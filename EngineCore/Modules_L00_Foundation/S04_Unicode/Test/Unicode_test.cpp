// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

// clang-format off

#include <Hobgoblin/Unicode.hpp>

#include <SFML/System/String.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>

#include <fmt/ostream.h>

#include <iostream>

namespace hg = jbatnozic::hobgoblin;

TEST(HGUnicodeTest, TestSerbianLatin) {
    static constexpr int32_t  EXPECTED_LENGTH = 14;
    static constexpr char16_t EXPECTED_CONTENTS[] = {
        0x0160, 0x0107, 0x010d, 0x0020, 0x0061, 0x0073, 0x0064,
        0x0066, 0x0020, 0x0111, 0x0161, 0x017e, 0x002e, 0x007e
    };

    const auto str = HG_UNISTR("Šćč asdf đšž.~");
    ASSERT_EQ(str.length(), EXPECTED_LENGTH);
    const char16_t* buffer = str.getBuffer();
    
    for (int32_t i = 0; i < EXPECTED_LENGTH; i += 1) {
        EXPECT_EQ((int)buffer[i], (int)EXPECTED_CONTENTS[i]);
        EXPECT_EQ((int)str[i],    (int)EXPECTED_CONTENTS[i]);
    }

    // icu::UnicodeString is NOT null-terminated!
    // EXPECT_EQ(buffer[EXPECTED_LENGTH], 0);
}

TEST(HGUnicodeTest, TestSerbianCyrillic) {
    static constexpr int32_t  EXPECTED_LENGTH = 16;
    static constexpr char16_t EXPECTED_CONTENTS[] = {
        0x0069, 0x0074, 0x0020, 0x0438, 0x0437, 0x0020, 0x0432, 0x0445,
        0x0430, 0x0442, 0x0020, 0x0438, 0x0442, 0x0020, 0x0069, 0x0073
    };

    const auto str = HG_UNISTR("it из вхат ит is");
    ASSERT_EQ(str.length(), EXPECTED_LENGTH);
    const char16_t* buffer = str.getBuffer();

    for (int32_t i = 0; i < EXPECTED_LENGTH; i += 1) {
        EXPECT_EQ((int)buffer[i], (int)EXPECTED_CONTENTS[i]);
        EXPECT_EQ((int)str[i],    (int)EXPECTED_CONTENTS[i]);
    }
}

TEST(HGUnicodeTest, TestWeirdAlphabets) {
    static constexpr int32_t  EXPECTED_LENGTH = 5;
    static constexpr char16_t EXPECTED_CONTENTS[] = {
        0x0061, 0x002d, 0x4e2d, 0x002d, 0x042f
    };

    const auto str = HG_UNISTR("a-中-Я");
    ASSERT_EQ(str.length(), EXPECTED_LENGTH);
    const char16_t* buffer = str.getBuffer();

    for (int32_t i = 0; i < EXPECTED_LENGTH; i += 1) {
        EXPECT_EQ((int)buffer[i], (int)EXPECTED_CONTENTS[i]);
        EXPECT_EQ((int)str[i],    (int)EXPECTED_CONTENTS[i]);
    }
}

TEST(HGUnicodeTest, UniStrConvTest_Utf8StrString) {
    const auto unistr = HG_UNISTR("!@#$%^&*()šđčć");
    const auto utf8str = hg::UniStrConv(hg::TO_UTF8_STD_STRING, unistr);
    EXPECT_EQ(unistr, hg::UniStrConv(hg::FROM_UTF8_STD_STRING, utf8str));
}

namespace {
std::string SlurpFileContents(std::filesystem::path aPath) {
    std::ifstream fileStream{aPath, std::ios::in | std::ios::binary};
    std::string fileContents{std::istreambuf_iterator<char>{fileStream},
                             std::istreambuf_iterator<char>()};
    return fileContents;
}
} // namespace

TEST(HGUnicodeTest, TestConversionToPath) {
    const auto basePath     = hg::UnicodeString{HG_TEST_ASSET_DIR};
    const auto relativePath = HG_UNISTR("čćdšzžj/гроњћчвељ.txt");
    const auto path         = hg::UniStrConv(hg::TO_STD_PATH, basePath + '/' + relativePath);

    ASSERT_TRUE(std::filesystem::exists(path));

    const auto fileContents = SlurpFileContents(path);

    EXPECT_EQ(fileContents, "TEST");
}

TEST(HGUnicodeTest, TestCharsetDetection) {
    struct TestCase {
        hg::UnicodeString relativeFilePath;
        hg::Charset       expectedCharset;
        std::int32_t      expectedConfidence;
    };

    const TestCase TEST_CASES[] = {
        {
            HG_UNILIT("ChuckNorris/ISO-8859-1.txt"),
            hg::Charset::ISO_8859_1,
            23
        },
        {
            HG_UNILIT("ChuckNorris/UTF-8.txt"),
            hg::Charset::ISO_8859_1, // With no BOM and no non-ASCII characters, there is no way to
            23                       // distinguish between UTF-8 and ISO-8859-1.
        },
        {
            HG_UNILIT("ChuckNorris/UTF-8_wUnicodeChars.txt"),
            hg::Charset::UTF_8,
            80
        },
        {
            HG_UNILIT("ChuckNorris/UTF-8_BOM.txt"),
            hg::Charset::UTF_8,
            100
        },
        {
            HG_UNILIT("ChuckNorris/UTF-8_BOM_wUnicodeChars.txt"),
            hg::Charset::UTF_8,
            100
        },
        {
            HG_UNILIT("ChuckNorris/UTF-16_BE_BOM.txt"),
            hg::Charset::UTF_16_BE,
            100
        },
        {
            HG_UNILIT("ChuckNorris/UTF-16_BE_BOM_wUnicodeChars.txt"),
            hg::Charset::UTF_16_BE,
            100
        },
        {
            HG_UNILIT("ChuckNorris/UTF-16_LE_BOM.txt"),
            hg::Charset::UTF_16_LE,
            100
        },
        {
            HG_UNILIT("ChuckNorris/UTF-16_LE_BOM_wUnicodeChars.txt"),
            hg::Charset::UTF_16_LE,
            100
        }
    };

    for (const auto& testCase : TEST_CASES) {
        const auto basePath     = hg::UnicodeString{HG_TEST_ASSET_DIR};
        const auto path         = hg::UniStrConv(hg::TO_STD_PATH, basePath + '/' + testCase.relativeFilePath);

        ASSERT_TRUE(std::filesystem::exists(path));

        const auto fileContents = SlurpFileContents(path);

        const auto detectionResult = hg::CharsetDetector(fileContents.c_str(), 
                                                         static_cast<std::int32_t>(fileContents.size())).detect();

        EXPECT_EQ(detectionResult.charset, testCase.expectedCharset)
            << "for file: " << hg::UniStrConv(hg::TO_ASCII_STD_STRING, testCase.relativeFilePath);

        EXPECT_EQ(detectionResult.confidence, testCase.expectedConfidence)
            << "for file: " << hg::UniStrConv(hg::TO_ASCII_STD_STRING, testCase.relativeFilePath);
    }
}

// TODO(Test newline processing)

namespace tou16formattable_test {
struct MustNotCopyCanNotMove_U16Formattable {
    #define Class MustNotCopyCanNotMove_U16Formattable

    MustNotCopyCanNotMove_U16Formattable(int aValue) : value{aValue} {}

    MustNotCopyCanNotMove_U16Formattable(const Class& aOther) {
        ADD_FAILURE() << "Unexpected copy (ctor) happened.";
    }

    MustNotCopyCanNotMove_U16Formattable(Class&&) = delete;

    Class& operator=(const Class& aOther) {
        if (&aOther != this) {
            ADD_FAILURE() << "Unexpected copy (operator) happened.";
        }
        return *this;
    }

    Class& operator=(Class&&) = delete;

    int value;

    #undef Class
};

struct MustNotCopyCanNotMove_StdStreamable {
    #define Class MustNotCopyCanNotMove_StdStreamable

    MustNotCopyCanNotMove_StdStreamable(int aValue) : value{aValue} {}

    MustNotCopyCanNotMove_StdStreamable(const Class& aOther) {
        ADD_FAILURE() << "Unexpected copy (ctor) happened.";
    }

    MustNotCopyCanNotMove_StdStreamable(Class&&) = delete;

    Class& operator=(const Class& aOther) {
        if (&aOther != this) {
            ADD_FAILURE() << "Unexpected copy (operator) happened.";
        }
        return *this;
    }

    Class& operator=(Class&&) = delete;

    friend std::ostream& operator<<(std::ostream& aOS, const Class& aObj) {
        return (aOS << "tou16formattable_test::MustNotCopyCanNotMove_StdStreamable(" << aObj.value << ")");
    }

    int value;

    #undef Class
};

struct MustNotCopyCanNotMove_StdTostringable {
    #define Class MustNotCopyCanNotMove_StdTostringable

    MustNotCopyCanNotMove_StdTostringable(int aValue) : value{aValue} {}

    MustNotCopyCanNotMove_StdTostringable(const Class& aOther) {
        ADD_FAILURE() << "Unexpected copy (ctor) happened.";
    }

    MustNotCopyCanNotMove_StdTostringable(Class&&) = delete;

    Class& operator=(const Class& aOther) {
        if (&aOther != this) {
            ADD_FAILURE() << "Unexpected copy (operator) happened.";
        }
        return *this;
    }

    Class& operator=(Class&&) = delete;

    int value;

    #undef Class
};

std::string to_string(const MustNotCopyCanNotMove_StdTostringable& aObj) {
    return "tou16formattable_test::MustNotCopyCanNotMove_StdTostringable(" +
           std::to_string(aObj.value) + 
           ')';
}

} // namespace tou16formattable_test

namespace fmt {

template <>
struct formatter<tou16formattable_test::MustNotCopyCanNotMove_U16Formattable, char16_t> 
    : ::fmt::formatter<std::u16string, char16_t> 
{
    template <class taContext>
    auto format(const tou16formattable_test::MustNotCopyCanNotMove_U16Formattable& aArg,
                taContext& aContext) const {
        std::u16string u16str{u"tou16formattable_test::MustNotCopyCanNotMove_U16Formattable("};
        const auto valstr = std::to_string(aArg.value);
        for (const char c : valstr) {
            u16str.push_back((char16_t)c);
        }
        u16str.push_back((char16_t)')');

        return ::fmt::formatter<std::u16string, char16_t>::format(u16str, aContext);
    }
};

} // namespace fmt

TEST(HGUnicodeTest, ToU16Formattable_UCoalesce_Test) {
    using hg::detail::ToU16Formattable;
    {
        SCOPED_TRACE("object is u16 formattable (primitive) - const reference to original object returned");

        const int obj{3};
        decltype(auto) formattable = ToU16Formattable(obj);
        static_assert(std::is_same<decltype(formattable), const int&>::value, "wrong type");

        EXPECT_EQ(&obj, &formattable);
        EXPECT_EQ(hg::UCoalesce(formattable), HG_UNISTR("3"));
    }
    {
        SCOPED_TRACE("object is u16 formattable (class) - const reference to original object returned");

        tou16formattable_test::MustNotCopyCanNotMove_U16Formattable obj{5};
        decltype(auto) formattable = ToU16Formattable(obj);
        static_assert(std::is_same<decltype(formattable), 
                                   tou16formattable_test::MustNotCopyCanNotMove_U16Formattable&>::value,
                      "wrong type");

        EXPECT_EQ(&obj, &formattable);
        EXPECT_EQ(hg::UCoalesce(formattable),
                    HG_UNISTR("tou16formattable_test::MustNotCopyCanNotMove_U16Formattable(5)"));
    }
#if 0 // Don't do this, it's dangerous and not supported
    {
        SCOPED_TRACE("object is u16 formattable but is a temporary - return it by reference");

        decltype(auto) formattable = ToU16Formattable(tou16formattable_test::MustNotCopyCanNotMove_U16Formattable{23});
    }
#endif
    {
        SCOPED_TRACE("object is not u16 formattable but is tostringable - UnicodeString returned");

        const tou16formattable_test::MustNotCopyCanNotMove_StdTostringable obj{66};
        decltype(auto) formattable = ToU16Formattable(obj);
        static_assert(std::is_same<decltype(formattable), hg::UnicodeString>::value, "wrong type");

        EXPECT_EQ(hg::UCoalesce(formattable),
                  HG_UNISTR("tou16formattable_test::MustNotCopyCanNotMove_StdTostringable(66)"));
    }
    {
        SCOPED_TRACE("object is not u16 formattable but is streamable - UnicodeString returned");

        const tou16formattable_test::MustNotCopyCanNotMove_StdStreamable obj{100};
        decltype(auto) formattable = ToU16Formattable(obj);
        static_assert(std::is_same<decltype(formattable), hg::UnicodeString>::value, "wrong type");

        EXPECT_EQ(hg::UCoalesce(formattable),
                  HG_UNISTR("tou16formattable_test::MustNotCopyCanNotMove_StdStreamable(100)"));
    }
    {
        SCOPED_TRACE("object is a UTF-16 string literal - char array returned");

        decltype(auto) formattable = ToU16Formattable(HG_UNILIT("utf-16 string literal å"));
        static_assert(std::is_same<decltype(formattable), const char16_t (&)[24]>::value, "wrong type");

        EXPECT_EQ(hg::UCoalesce(formattable), HG_UNISTR("utf-16 string literal å"));
    }
    {
        SCOPED_TRACE("object is an ascii string literal - UnicodeString returned");

        decltype(auto) formattable = ToU16Formattable("ascii string literal");
        static_assert(std::is_same<decltype(formattable), hg::UnicodeString>::value, "wrong type");

        EXPECT_EQ(hg::UCoalesce(formattable), HG_UNISTR("ascii string literal"));
    }
    {
        SCOPED_TRACE("object is std::string - UnicodeString returned");

        decltype(auto) formattable = ToU16Formattable(std::string{"std::string"});
        static_assert(std::is_same<decltype(formattable), hg::UnicodeString>::value, "wrong type");

        EXPECT_EQ(hg::UCoalesce(formattable), HG_UNISTR("std::string"));
    }
}

namespace {

struct CustomFormattedType {
    char c = 0;
    int  i = 0;
};

std::ostream& operator<<(std::ostream& aOStream, const CustomFormattedType& aCFT) {
    return (aOStream << aCFT.c << ' ' << aCFT.i);
}

} // namespace

namespace dummy {
struct CustomFormattedType2 {};

hg::UnicodeString ToUnicodeString(const CustomFormattedType2&) {
    return HG_UNISTR("CustomFormattedType2");
}

std::string to_string(const CustomFormattedType2&) {
    ADD_FAILURE();
    return {};
}
}

TEST(HGUnicodeTest, UFormatTest) {
    {
        const auto expectedString  = HG_UNISTR("The answer is 42.");
        const auto formattedString = hg::UFormat(HG_UNIFMT("The answer is {}."), 42);

        EXPECT_EQ(expectedString, formattedString);
    }
    {
        const auto expectedString  = HG_UNISTR("The answer is 42.");
        const auto formattedString = hg::UFormat(HG_UNIFMT("The {1} is {0}."), 42, HG_UNILIT("answer"));

        EXPECT_EQ(expectedString, formattedString);
    }
    {
        const auto expectedString  = HG_UNISTR("The string 'šđžčć' contains unicode characters such as ž.");
        const auto formattedString = hg::UFormat(HG_UNIFMT("The string '{}' contains unicode characters such as ž."),
                                                 HG_UNILIT("šđžčć"));

        EXPECT_EQ(expectedString, formattedString);
    }
    {
        const auto expectedString  = HG_UNISTR("The string 'šđžčć' contains unicode characters such as ž.");
        const auto formattedString = hg::UFormat(HG_UNIFMT("The string '{}' contains unicode characters such as ž."),
                                                 HG_UNISTR("šđžčć"));

        EXPECT_EQ(expectedString, formattedString);
    }
    {
        const auto expectedString  = HG_UNISTR("The CPU is i 7.");
        const auto formattedString = hg::UFormat(HG_UNIFMT("The CPU is {}."), CustomFormattedType{'i', 7});

        EXPECT_EQ(expectedString, formattedString);
    }
    {
        const auto expectedString  = HG_UNISTR("_ CustomFormattedType2 _");
        const auto formattedString = hg::UFormat(HG_UNIFMT("_ {} _"), dummy::CustomFormattedType2{});
        ToUnicodeString(dummy::CustomFormattedType2{});

        EXPECT_EQ(expectedString, formattedString);
    }
}

TEST(HGUnicodeTest, SfStringConversionTest_Simple) {
    const auto expectedSfStr  = sf::String{"Terminator 2"};
    const auto expectedUniStr = HG_UNISTR("Terminator 2");

    {
        SCOPED_TRACE("HG to SFML");

        sf::String sfStr;
        hg::detail::StoreUStringInSfString(expectedUniStr, &sfStr);
        EXPECT_EQ(sfStr, expectedSfStr);
    }
    {
        SCOPED_TRACE("SFML to HG");

        hg::UnicodeString uniStr;
        hg::detail::LoadUStringFromSfString(uniStr, &expectedSfStr);
        EXPECT_EQ(uniStr, expectedUniStr);
    }
}

static sf::String CreateSfString(const char32_t* aChars, int aCharCount) {
    sf::String result;
    for (int i = 0; i < aCharCount; i += 1) {
        if (i == aCharCount - 1 && aChars[i] == '\0') {
            break;
        }
        result.insert(result.getSize(), static_cast<sf::Uint32>(aChars[i]));
    }
    return result;
}

TEST(HGUnicodeTest, SfStringConversionTest_Complex) {   
    const char32_t utf32Chars[] = U"_ asdf 1234 ćšđž _";

    const auto expectedSfStr  = CreateSfString(utf32Chars, sizeof(utf32Chars) / sizeof(utf32Chars[0]));
    const auto expectedUniStr = HG_UNISTR("_ asdf 1234 ćšđž _");

    {
        SCOPED_TRACE("HG to SFML");

        sf::String sfStr;
        hg::detail::StoreUStringInSfString(expectedUniStr, &sfStr);
        EXPECT_EQ(sfStr, expectedSfStr);
    }
    {
        SCOPED_TRACE("SFML to HG");

        hg::UnicodeString uniStr;
        hg::detail::LoadUStringFromSfString(uniStr, &expectedSfStr);
        EXPECT_EQ(uniStr, expectedUniStr);
    }
}

TEST(HGUnicodeTest, SfStringConversionTest_EmptyString) {   
    const auto expectedSfStr  = CreateSfString(nullptr, 0);
    const auto expectedUniStr = HG_UNISTR("");

    {
        SCOPED_TRACE("HG to SFML");

        sf::String sfStr;
        hg::detail::StoreUStringInSfString(expectedUniStr, &sfStr);
        EXPECT_EQ(sfStr, expectedSfStr);
    }
    {
        SCOPED_TRACE("SFML to HG");

        hg::UnicodeString uniStr;
        hg::detail::LoadUStringFromSfString(uniStr, &expectedSfStr);
        EXPECT_EQ(uniStr, expectedUniStr);
    }
}

// clang-format on

TEST(HGUnicodeTest, ValidURegexTest) {
    hg::URegex        regex{HG_UNISTR("(([a-zA-Z])[a-zA-Z0-9]*)")};
    hg::UMatchResults results;

    ASSERT_FALSE(hg::RegexMatch(HG_UNISTR("5ChunkNorris0123"), regex, results));
    ASSERT_FALSE(hg::RegexMatch(HG_UNISTR("kChunkNorris_123"), regex, results));

    ASSERT_TRUE(hg::RegexMatch(HG_UNISTR("kChunkNorris0123"), regex, results));
    EXPECT_EQ(results.getGroupCount(), 2);
    EXPECT_EQ(results[0], HG_UNISTR("kChunkNorris0123"));
    EXPECT_EQ(results[1], HG_UNISTR("kChunkNorris0123"));
    EXPECT_EQ(results[2], HG_UNISTR("k"));

    EXPECT_THROW(results[3], hg::URegexError);
}

TEST(HGUnicodeTest, InvalidURegexTest) {
    EXPECT_THROW(hg::URegex{HG_UNISTR("?*([)")}, hg::URegexError);
}
