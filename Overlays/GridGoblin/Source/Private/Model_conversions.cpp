// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Model/Chunk.hpp>
#include <GridGoblin/Private/Model_conversions.hpp>

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/Utility/Base64.hpp>
#include <Hobgoblin/Utility/Stream.hpp>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <chrono>
#include <optional>
#include <string>
#include <type_traits>

namespace jbatnozic {
namespace gridgoblin {
namespace detail {

namespace json = rapidjson;

struct ReusableConversionBuffers {
    hg::util::BufferStream stream; //!< For extension serialization
    std::string            string;
};

hg::NeverNull<ReusableConversionBuffers*> NewReusableConversionBuffers() {
    return new ReusableConversionBuffers;
}

void DeleteReusableConversionBuffers(ReusableConversionBuffers* aBuffers) {
    delete aBuffers;
}

namespace {
constexpr auto LOG_ID = "GridGoblin";

const std::string TAG_EMPTY     = "empty";
const std::string TAG_BINSTREAM = "binary_stream";

#define ENSURE_JSON_CONTAINS(_value_, _member_name_, _member_type_)                                 \
    do {                                                                                            \
        if (!(_value_).HasMember(_member_name_) || !(_value_)[_member_name_].Is##_member_type_()) { \
            HG_THROW_TRACED(JsonParseError, 0, "No array member '{}' found.", _member_name_);       \
        }                                                                                           \
    } while (false)

template <class taIntegral>
taIntegral GetIntMember(const json::Value& aJsonValue, const char* aMemberName) {
    static_assert(std::is_integral<taIntegral>::value, "GetIntMember only works with integral types.");

    ENSURE_JSON_CONTAINS(aJsonValue, aMemberName, Int);

    return static_cast<taIntegral>(aJsonValue[aMemberName].GetInt());
}

using ExtensionKind = std::optional<ChunkExtensionInterface::SerializationMethod>;

const std::string& ExtensionKindToString(const ExtensionKind& aExtensionKind) {
    if (!aExtensionKind.has_value() || *aExtensionKind == ExtensionKind::value_type::NONE) {
        return TAG_EMPTY;
    }

    switch (*aExtensionKind) {
    case ExtensionKind::value_type::BINARY_STREAM:
        return TAG_BINSTREAM;
        break;

    default:
        HG_UNREACHABLE("Invalid value for ExtensionKind ({}).", (int)*aExtensionKind);
    }
}

ExtensionKind StringToExtensionKind(const std::string& aString) {
    /*--*/ if (aString == TAG_EMPTY) {
        return std::nullopt;
    } else if (aString == TAG_BINSTREAM) {
        return {ExtensionKind::value_type::BINARY_STREAM};
    } else {
        HG_THROW_TRACED(JsonParseError, 0, "Unsupported string: '{}'.", aString);
    }
}

void Base64Encode(
    /*  in */ const ChunkExtensionInterface&               aChunkExtension,
    /*  in */ ChunkExtensionInterface::SerializationMethod aPreferredSerializationMethod,
    /* out */ std::string&                                 aBase64EncodeBuffer,
    /*  in */ hg::util::BufferStream&                      aReusableBufStream)
//
{
    aReusableBufStream.clear();
    aBase64EncodeBuffer.clear();

    switch (aPreferredSerializationMethod) {
    case ChunkExtensionInterface::SerializationMethod::BINARY_STREAM:
        {
            aChunkExtension.serialize(aReusableBufStream);

            const auto encodeSizePrediction = hg::util::GetRecommendedOutputBufferSizeForBase64Encode(
                aReusableBufStream.getDataSize());

            aBase64EncodeBuffer.resize(hg::pztos(encodeSizePrediction));

            const auto encodeSizeActual = hg::util::Base64Encode(aReusableBufStream.getData(),
                                                                 aReusableBufStream.getDataSize(),
                                                                 aBase64EncodeBuffer.data(),
                                                                 hg::stopz(aBase64EncodeBuffer.size()));

            aBase64EncodeBuffer.resize(hg::pztos(encodeSizeActual));
        }
        break;

    default:
        HG_UNREACHABLE("Invalid value for ChunkExtensionInterface::SerializationMethod ({}).",
                       (int)aPreferredSerializationMethod);
    }
}

void Base64Decode(
    /* out */ ChunkExtensionInterface&                     aChunkExtension,
    /*  in */ ChunkExtensionInterface::SerializationMethod aPreferredSerializationMethod,
    /*  in */ const char*                                  aBase64EncodedString,
    /*  in */ hg::PZInteger                                aBase64EncodedStringLength,
    /*  in */ std::string&                                 aReusableString)
//
{
    switch (aPreferredSerializationMethod) {
    case ChunkExtensionInterface::SerializationMethod::BINARY_STREAM:
        {
            const auto decodeSizePrediction =
                hg::util::GetRecommendedOutputBufferSizeForBase64Decode(aBase64EncodedStringLength);

            aReusableString.clear();
            aReusableString.resize(hg::pztos(decodeSizePrediction));

            const auto decodeSizeActual = hg::util::Base64Decode(aBase64EncodedString,
                                                                 aBase64EncodedStringLength,
                                                                 aReusableString.data(),
                                                                 hg::stopz(aReusableString.size()));

            hg::util::ViewStream vstream{aReusableString.data(), decodeSizeActual};

            aChunkExtension.deserialize(vstream);
        }
        break;

    default:
        HG_UNREACHABLE("Invalid value for ChunkExtensionInterface::SerializationMethod ({}).",
                       (int)aPreferredSerializationMethod);
    }
}

//! Named based on the 'elvis operator' (?:) from other languages
#define ELVIS(_lhs_, _rhs_) ((_lhs_) ? (_lhs_) : (_rhs_))
} // namespace

// MARK: Cell <-> JSON

json::Value CellToJson(const CellModel& aCell, json::Document& aDocument) {
    const auto cellFlags = aCell.getFlags();

    auto& allocator = aDocument.GetAllocator();

    json::Value value;
    value.SetObject();

    value.AddMember("flags", json::Value{cellFlags}.Move(), allocator);

    if (cellFlags & CellModel::FLOOR_INITIALIZED) {
        const auto& floor = aCell.getFloor();

        json::Value jsonFloor;
        jsonFloor.SetObject();

        jsonFloor.AddMember("spriteId", json::Value{floor.spriteId}.Move(), allocator);

        value.AddMember("floor", jsonFloor, allocator);
    }

    if (cellFlags & CellModel::WALL_INITIALIZED) {
        const auto& wall = aCell.getWall();

        json::Value jsonWall;
        jsonWall.SetObject();

        jsonWall.AddMember("spriteId", json::Value{wall.spriteId}.Move(), allocator);
        jsonWall.AddMember("spriteId_reduced", json::Value{wall.spriteId_reduced}.Move(), allocator);
        jsonWall.AddMember("shape", json::Value{ShapeToString(wall.shape), allocator}.Move(), allocator);

        value.AddMember("wall", jsonWall, allocator);
    }

    return value;
}

CellModel JsonToCell(const json::Value& aJson) {
    CellModel cell;

    const auto flags = GetIntMember<decltype(cell.getFlags())>(aJson, "flags");

    if (flags & CellModel::FLOOR_INITIALIZED) {
        if (!aJson.HasMember("floor") || !aJson["floor"].IsObject()) {
            HG_THROW_TRACED(JsonParseError, 0, "No object member '{}' found.", "floor");
        }

        CellModel::Floor floor;
        floor.spriteId = GetIntMember<decltype(floor.spriteId)>(aJson["floor"], "spriteId");

        cell.setFloor(floor);
    }

    if (flags & CellModel::WALL_INITIALIZED) {
        if (!aJson.HasMember("wall") || !aJson["wall"].IsObject()) {
            HG_THROW_TRACED(JsonParseError, 0, "No object member '{}' found.", "wall");
        }

        CellModel::Wall wall;
        wall.spriteId = GetIntMember<decltype(wall.spriteId)>(aJson["wall"], "spriteId");
        wall.spriteId_reduced =
            GetIntMember<decltype(wall.spriteId_reduced)>(aJson["wall"], "spriteId_reduced");
        wall.shape = StringToShape(aJson["wall"]["shape"].GetString());

        cell.setWall(wall);
    }

    return cell;
}

// MARK: Chunk <-> JSON

json::Document ChunkToJson(const Chunk& aChunk, ReusableConversionBuffers* aReusableConversionBuffers) {
    const auto chunkWidth  = aChunk.getCellCountX();
    const auto chunkHeight = aChunk.getCellCountY();

    json::Document doc;
    auto&          allocator = doc.GetAllocator();
    doc.SetObject();

    // Add dimensions
    doc.AddMember("width", json::Value{chunkWidth}.Move(), allocator);
    doc.AddMember("height", json::Value{chunkHeight}.Move(), allocator);

    // Add cells
    json::Value cellArray{json::kArrayType};
    cellArray.Reserve(chunkWidth * chunkHeight, allocator);
    for (hg::PZInteger y = 0; y < chunkHeight; y += 1) {
        for (hg::PZInteger x = 0; x < chunkWidth; x += 1) {
            auto value = CellToJson(aChunk.getCellAtUnchecked(x, y), doc);
            cellArray.PushBack(value, allocator);
        }
    }
    doc.AddMember("cells", cellArray, allocator);

    // Add extension
    const auto* extension = aChunk.getExtension();
    if (extension) {
        const auto  method = extension->getPreferredSerializationMethod();
        const auto& kind   = ExtensionKindToString(method);

        doc.AddMember("extension_kind", json::Value{kind, allocator}.Move(), allocator);

        if (method != ChunkExtensionInterface::SerializationMethod::NONE) {
            if (aReusableConversionBuffers != nullptr) {
                Base64Encode(*extension,
                             method,
                             aReusableConversionBuffers->string,
                             aReusableConversionBuffers->stream);

                doc.AddMember("extension_data",
                              json::Value{aReusableConversionBuffers->string, allocator},
                              allocator);
            } else {
                ReusableConversionBuffers defaultBuffers;
                Base64Encode(*extension, method, defaultBuffers.string, defaultBuffers.stream);

                doc.AddMember("extension_data",
                              json::Value{defaultBuffers.string, allocator},
                              allocator);
            }
        }
    } else {
        const auto& kind = ExtensionKindToString(std::nullopt);
        doc.AddMember("extension_kind", json::Value{kind, allocator}.Move(), allocator);
    }

    return doc;
}

Chunk JsonToChunk(const json::Document&        aJsonDocument,
                  const ChunkExtensionFactory& aChunkExtensionFactory,
                  ReusableConversionBuffers*   aReusableConversionBuffers) {
    // Read dimensions
    const auto chunkWidth  = GetIntMember<hg::PZInteger>(aJsonDocument, "width");
    const auto chunkHeight = GetIntMember<hg::PZInteger>(aJsonDocument, "height");

    Chunk chunk{chunkWidth, chunkHeight};

    // Read cells
    ENSURE_JSON_CONTAINS(aJsonDocument, "cells", Array);

    const auto& array = aJsonDocument["cells"];
    if (array.Size() != hg::pztos(chunkWidth * chunkHeight)) {
        HG_THROW_TRACED(JsonParseError,
                        0,
                        "Cell array is {} (expected {}).",
                        array.Size(),
                        chunkWidth * chunkHeight);
    }

    auto* valuePtr = array.Begin();
    for (hg::PZInteger y = 0; y < chunkHeight; y += 1) {
        for (hg::PZInteger x = 0; x < chunkWidth; x += 1) {
            chunk.getCellAtUnchecked(x, y) = CellModel{JsonToCell(*valuePtr)};
            valuePtr++;
        }
    }

    // Read extension
    ENSURE_JSON_CONTAINS(aJsonDocument, "extension_kind", String);

    auto extension = aChunkExtensionFactory ? aChunkExtensionFactory(chunk) : nullptr;

    const auto kind = StringToExtensionKind(aJsonDocument["extension_kind"].GetString());
    if (kind.has_value()) {
        if (extension != nullptr) {
            ENSURE_JSON_CONTAINS(aJsonDocument, "extension_data", String);

            const auto& value               = aJsonDocument["extension_data"];
            const auto* encodedExtString    = value.GetString();
            const auto  encodedExtStringLen = value.GetStringLength();

            if (aReusableConversionBuffers != nullptr) {
                Base64Decode(*extension,
                             *kind,
                             encodedExtString,
                             encodedExtStringLen,
                             aReusableConversionBuffers->string);
            } else {
                ReusableConversionBuffers defaultBuffers;
                Base64Decode(*extension,
                             *kind,
                             encodedExtString,
                             encodedExtStringLen,
                             defaultBuffers.string);
            }
        } else {
            HG_LOG_WARN(
                LOG_ID,
                "Found serialized data for chunk extension but failed to instantiate the extension.");
        }
    }

    chunk.setExtension(std::move(extension));

    return chunk;
}

std::string ChunkToJsonString(const Chunk&               aChunk,
                              ReusableConversionBuffers* aReusableConversionBuffers) {
#if HG_BUILD_TYPE == HG_DEBUG
#define JsonWriter json::PrettyWriter
#else
#define JsonWriter json::Writer
#endif

    const auto doc = ChunkToJson(aChunk, aReusableConversionBuffers);

    json::StringBuffer             stringbuf;
    JsonWriter<json::StringBuffer> writer(stringbuf);
    doc.Accept(writer);

    return stringbuf.GetString();

#undef JsonWriter
}

Chunk JsonStringToChunk(std::string                  aJsonString,
                        const ChunkExtensionFactory& aChunkExtensionFactory,
                        ReusableConversionBuffers*   aReusableConversionBuffers) {
    const auto start = std::chrono::steady_clock::now();

    json::Document doc;
    doc.ParseInsitu(aJsonString.data());

    auto result = JsonToChunk(doc, aChunkExtensionFactory, aReusableConversionBuffers);

    const auto end = std::chrono::steady_clock::now();
    const auto us  = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    HG_LOG_INFO(LOG_ID, "JsonStringToChunk took {}ms.", us.count() / 1000.0);

    return result;
}

} // namespace detail
} // namespace gridgoblin
} // namespace jbatnozic
