// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <Hobgoblin/Utility/Base64.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <string>
#include <vector>

namespace jbatnozic {
namespace hobgoblin {
namespace util {

TEST(Base64Test, EncodeThenDecodeVariableLengthBuffer) {
    static constexpr std::size_t MAX_BUFFER_SIZE = 4096;

    std::vector<std::uint8_t> sourceBuffer;
    sourceBuffer.reserve(MAX_BUFFER_SIZE * sizeof(std::uint32_t));

    std::string encodeBuffer;
    encodeBuffer.reserve(MAX_BUFFER_SIZE * sizeof(std::uint32_t) * 2);

    std::vector<std::uint8_t> decodeBuffer;
    decodeBuffer.reserve(MAX_BUFFER_SIZE * sizeof(std::uint32_t));

    for (std::size_t bufferSize = 1; bufferSize < MAX_BUFFER_SIZE; bufferSize += 30) {
        SCOPED_TRACE("Buffer size = " + std::to_string(bufferSize) + " (x4)");

        // Prepare source buffer
        sourceBuffer.resize(bufferSize * sizeof(std::uint32_t));
        for (std::size_t i = 0; i < bufferSize; i += 1) {
            reinterpret_cast<std::uint32_t*>(sourceBuffer.data())[i] = static_cast<std::uint32_t>(i);
        }

        // Prepare encode buffer
        const auto encodeSizePrediction =
            GetRecommendedOutputBufferSizeForBase64Encode(stopz(sourceBuffer.size()));
        encodeBuffer.resize(pztos(encodeSizePrediction));

        // Encode
        const auto encodeSizeActual =
            Base64Encode(sourceBuffer.data(),
                         stopz(sourceBuffer.size()),
                         encodeBuffer.data(),
                         stopz(encodeBuffer.size()));

        ASSERT_LE(encodeSizeActual, encodeSizePrediction);
        EXPECT_LE(encodeSizePrediction - encodeSizeActual, 4);

        encodeBuffer.resize(pztos(encodeSizeActual));

        // Decode
        const auto decodeSizePrediction =
            GetRecommendedOutputBufferSizeForBase64Decode(stopz(encodeBuffer.size()));
        decodeBuffer.resize(pztos(decodeSizePrediction));

        const auto decodeSizeActual =
            Base64Decode(encodeBuffer.data(),
                         stopz(encodeBuffer.size()),
                         decodeBuffer.data(),
                         stopz(decodeBuffer.size()));

        ASSERT_LE(decodeSizeActual, decodeSizePrediction);
        EXPECT_LE(decodeSizePrediction - decodeSizeActual, 4);

        decodeBuffer.resize(pztos(decodeSizeActual));

        // Compare
        ASSERT_EQ(sourceBuffer.size(), decodeBuffer.size());
        EXPECT_THAT(decodeBuffer, testing::Eq(sourceBuffer));
    }
}

} // namespace util
} // namespace hobgoblin
} // namespace jbatnozic
