#include <gtest/gtest.h>

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "parse.hpp"
#include "scan.hpp"

namespace
{
template <typename T>
void AssertSuccess(const std::expected<T, stdx::details::scan_error> &result)
{
    ASSERT_TRUE(result.has_value()) << result.error().message;
}

template <typename T>
void AssertFailure(const std::expected<T, stdx::details::scan_error> &result)
{
    ASSERT_FALSE(result.has_value());
    ASSERT_FALSE(result.error().message.empty());
}

void AssertErrorContains(const std::string &actual, std::string_view expected_substring)
{
    ASSERT_NE(actual.find(expected_substring), std::string::npos) << "Actual error message: " << actual;
}

template <typename Expected, typename Actual>
void AssertSameType()
{
    static_assert(std::is_same_v<Expected, Actual>, "Types are different");
}

template <typename Expected, typename Actual>
void AssertValueAndType(const Actual &actual, const Expected &expected)
{
    static_assert(std::is_same_v<Expected, Actual>, "Types are different");
    EXPECT_EQ(actual, expected);
}

template <typename Expected, typename Actual>
void AssertFloatingValueAndType(const Actual &actual, const Expected &expected)
{
    static_assert(std::is_same_v<Expected, Actual>, "Types are different");

    if constexpr (std::is_same_v<Expected, float>)
        EXPECT_FLOAT_EQ(actual, expected);
    else
        EXPECT_DOUBLE_EQ(actual, expected);
}
}  // namespace

// parse_sources success tests
TEST(ParseSourcesSuccessTests, ParsesSinglePlaceholderWithoutLiterals)
{
    auto result = stdx::details::parse_sources<std::int32_t>("123", "{%d}");

    AssertSuccess(result);

    const auto &[format_parts, input_parts] = result.value();

    ASSERT_EQ(format_parts.size(), 1);
    ASSERT_EQ(input_parts.size(), 1);

    EXPECT_EQ(format_parts[0], "%d");
    EXPECT_EQ(input_parts[0], "123");
}

TEST(ParseSourcesSuccessTests, ParsesSinglePlaceholderWithLeadingLiteral)
{
    auto result = stdx::details::parse_sources<std::int32_t>("id=42", "id={%d}");

    AssertSuccess(result);

    const auto &[format_parts, input_parts] = result.value();

    ASSERT_EQ(format_parts.size(), 1);
    ASSERT_EQ(input_parts.size(), 1);

    EXPECT_EQ(format_parts[0], "%d");
    EXPECT_EQ(input_parts[0], "42");
}

TEST(ParseSourcesSuccessTests, ParsesSinglePlaceholderWithTrailingLiteral)
{
    auto result = stdx::details::parse_sources<std::int32_t>("42!", "{%d}!");

    AssertSuccess(result);

    const auto &[format_parts, input_parts] = result.value();

    ASSERT_EQ(format_parts.size(), 1);
    ASSERT_EQ(input_parts.size(), 1);

    EXPECT_EQ(format_parts[0], "%d");
    EXPECT_EQ(input_parts[0], "42");
}

TEST(ParseSourcesSuccessTests, ParsesSeveralPlaceholdersSeparatedByLiterals)
{
    auto result = stdx::details::parse_sources<std::int32_t, std::string, double>("id=42 name=Bob score=92.5",
                                                                                  "id={%d} name={%s} score={%f}");

    AssertSuccess(result);

    const auto &[format_parts, input_parts] = result.value();

    ASSERT_EQ(format_parts.size(), 3);
    ASSERT_EQ(input_parts.size(), 3);

    EXPECT_EQ(format_parts[0], "%d");
    EXPECT_EQ(format_parts[1], "%s");
    EXPECT_EQ(format_parts[2], "%f");

    EXPECT_EQ(input_parts[0], "42");
    EXPECT_EQ(input_parts[1], "Bob");
    EXPECT_EQ(input_parts[2], "92.5");
}

TEST(ParseSourcesSuccessTests, ParsesPlaceholderInTheMiddleOfLiterals)
{
    auto result = stdx::details::parse_sources<std::string>("Hello [world]!", "Hello [{%s}]!");

    AssertSuccess(result);

    const auto &[format_parts, input_parts] = result.value();

    ASSERT_EQ(format_parts.size(), 1);
    ASSERT_EQ(input_parts.size(), 1);

    EXPECT_EQ(format_parts[0], "%s");
    EXPECT_EQ(input_parts[0], "world");
}

TEST(ParseSourcesSuccessTests, PreservesEmptyCapturedSubstring)
{
    auto result = stdx::details::parse_sources<std::string>("prefix--suffix", "prefix-{%s}-suffix");

    AssertSuccess(result);

    const auto &[format_parts, input_parts] = result.value();

    ASSERT_EQ(format_parts.size(), 1);
    ASSERT_EQ(input_parts.size(), 1);

    EXPECT_EQ(format_parts[0], "%s");
    EXPECT_EQ(input_parts[0], "");
}

// parse_sources error tests
TEST(ParseSourcesErrorTests, FailsOnUnmatchedClosingBrace)
{
    auto result = stdx::details::parse_sources<std::int32_t>("123", "%d}");

    AssertFailure(result);
    EXPECT_EQ(result.error().message, "Parsing error: Unmatched '}' in format string");
}

TEST(ParseSourcesErrorTests, FailsOnUnmatchedOpeningBrace)
{
    auto result = stdx::details::parse_sources<std::int32_t>("123", "{%d");

    AssertFailure(result);
    EXPECT_EQ(result.error().message, "Parsing error: Unmatched '{' in format string");
}

TEST(ParseSourcesErrorTests, FailsOnAdjacentPlaceholders)
{
    auto result = stdx::details::parse_sources<std::int32_t, std::int32_t>("123456", "{%d}{%d}");

    AssertFailure(result);
    EXPECT_EQ(result.error().message, "Parsing error: Adjacent placeholders are not allowed");
}

TEST(ParseSourcesErrorTests, FailsWhenLiteralPartDoesNotMatch)
{
    auto result = stdx::details::parse_sources<std::int32_t>("idx=42", "id={%d}");

    AssertFailure(result);
    EXPECT_EQ(result.error().message, "Parsing error: Unformatted text in input and format string are different");
}

TEST(ParseSourcesErrorTests, FailsWhenInputContainsExtraCharacters)
{
    auto result = stdx::details::parse_sources<std::int32_t>("42!!!!", "{%d}!");

    AssertFailure(result);
    EXPECT_EQ(result.error().message, "Parsing error: Input contains extra characters");
}

TEST(ParseSourcesErrorTests, FailsWhenPlaceholderCountDoesNotMatchTemplateArguments)
{
    auto result = stdx::details::parse_sources<std::int32_t, std::string>("42", "{%d}");

    AssertFailure(result);
    EXPECT_EQ(result.error().message, "Parsing error: Placeholder count does not match number of template arguments");
}

// parse_value_with_format error tests
TEST(ParseValueWithFormatErrorTests, FailsOnInvalidFormatString)
{
    auto result = stdx::details::parse_value_with_format<std::int32_t>("123", "d");

    AssertFailure(result);
    AssertErrorContains(result.error().message, "invalid format");
    AssertErrorContains(result.error().message, "d");
}

TEST(ParseValueWithFormatErrorTests, FailsOnFormatMismatch)
{
    auto result = stdx::details::parse_value_with_format<std::uint32_t>("123", "%d");

    AssertFailure(result);
    AssertErrorContains(result.error().message, "conversion specifier mismatch");
    AssertErrorContains(result.error().message, "uint32_t");
    AssertErrorContains(result.error().message, "expected '%u'");
    AssertErrorContains(result.error().message, "got '%d'");
}

TEST(ParseValueWithFormatErrorTests, FailsOnSignedInvalidArgument)
{
    auto result = stdx::details::parse_value_with_format<std::int32_t>("abc", "%d");

    AssertFailure(result);
    AssertErrorContains(result.error().message, "failed to parse value");
    AssertErrorContains(result.error().message, "abc");
    AssertErrorContains(result.error().message, "int32_t");
    AssertErrorContains(result.error().message, "invalid_argument");
}

TEST(ParseValueWithFormatErrorTests, FailsOnUnsignedInvalidArgument)
{
    auto result = stdx::details::parse_value_with_format<std::uint32_t>("xyz", "%u");

    AssertFailure(result);
    AssertErrorContains(result.error().message, "failed to parse value");
    AssertErrorContains(result.error().message, "xyz");
    AssertErrorContains(result.error().message, "uint32_t");
    AssertErrorContains(result.error().message, "invalid_argument");
}

TEST(ParseValueWithFormatErrorTests, FailsOnFloatingInvalidArgument)
{
    auto result = stdx::details::parse_value_with_format<double>("oops", "%f");

    AssertFailure(result);
    AssertErrorContains(result.error().message, "failed to parse value");
    AssertErrorContains(result.error().message, "oops");
    AssertErrorContains(result.error().message, "double");
    AssertErrorContains(result.error().message, "invalid_argument");
}

TEST(ParseValueWithFormatErrorTests, FailsOnTrailingCharactersInInteger)
{
    auto result = stdx::details::parse_value_with_format<std::int32_t>("123abc", "%d");

    AssertFailure(result);
    AssertErrorContains(result.error().message, "input contains trailing characters");
    AssertErrorContains(result.error().message, "123abc");
    AssertErrorContains(result.error().message, "int32_t");
}

TEST(ParseValueWithFormatErrorTests, FailsOnTrailingCharactersInFloat)
{
    auto result = stdx::details::parse_value_with_format<float>("3.14x", "%f");

    AssertFailure(result);
    AssertErrorContains(result.error().message, "input contains trailing characters");
    AssertErrorContains(result.error().message, "3.14x");
    AssertErrorContains(result.error().message, "float");
}

TEST(ParseValueWithFormatErrorTests, FailsOnSignedOutOfRange)
{
    auto result = stdx::details::parse_value_with_format<std::int8_t>("1000", "%d");

    AssertFailure(result);
    AssertErrorContains(result.error().message, "result_out_of_range");
    AssertErrorContains(result.error().message, "1000");
    AssertErrorContains(result.error().message, "int8_t");
}

TEST(ParseValueWithFormatErrorTests, FailsOnUnsignedOutOfRange)
{
    auto result = stdx::details::parse_value_with_format<std::uint8_t>("1000", "%u");

    AssertFailure(result);
    AssertErrorContains(result.error().message, "result_out_of_range");
    AssertErrorContains(result.error().message, "1000");
    AssertErrorContains(result.error().message, "uint8_t");
}

// parse_value_with_format success tests
TEST(ParseValueWithFormatSuccessTests, ParsesSignedInteger)
{
    auto result = stdx::details::parse_value_with_format<std::int32_t>("-42", "%d");

    AssertSuccess(result);
    AssertValueAndType<std::int32_t>(result.value(), static_cast<std::int32_t>(-42));
}

TEST(ParseValueWithFormatSuccessTests, ParsesUnsignedInteger)
{
    auto result = stdx::details::parse_value_with_format<std::uint64_t>("42", "%u");

    AssertSuccess(result);
    AssertValueAndType<std::uint64_t>(result.value(), static_cast<std::uint64_t>(42));
}

TEST(ParseValueWithFormatSuccessTests, ParsesFloat)
{
    auto result = stdx::details::parse_value_with_format<float>("3.5", "%f");

    AssertSuccess(result);
    AssertFloatingValueAndType<float>(result.value(), 3.5f);
}

TEST(ParseValueWithFormatSuccessTests, ParsesDouble)
{
    auto result = stdx::details::parse_value_with_format<double>("2.25", "%f");

    AssertSuccess(result);
    AssertFloatingValueAndType<double>(result.value(), 2.25);
}

TEST(ParseValueWithFormatSuccessTests, ParsesStdString)
{
    auto result = stdx::details::parse_value_with_format<std::string>("hello", "%s");

    AssertSuccess(result);
    AssertValueAndType<std::string>(result.value(), std::string("hello"));
}

TEST(ParseValueWithFormatSuccessTests, ParsesStdStringView)
{
    auto result = stdx::details::parse_value_with_format<std::string_view>("hello", "%s");

    AssertSuccess(result);
    AssertValueAndType<std::string_view>(result.value(), std::string_view("hello"));
}

TEST(ParseValueWithFormatSuccessTests, ParsesByTypeWhenFormatIsEmpty)
{
    auto result = stdx::details::parse_value_with_format<std::int16_t>("123", "");

    AssertSuccess(result);
    AssertValueAndType<std::int16_t>(result.value(), static_cast<std::int16_t>(123));
}

// scan success tests
TEST(ScanSuccessTests, ScansSingleInteger)
{
    auto result = stdx::scan<std::int32_t>("123", "{%d}");

    AssertSuccess(result);

    auto values = result.value().values();
    using tuple_type = decltype(values);

    AssertSameType<std::tuple<std::int32_t>, tuple_type>();
    AssertValueAndType<std::int32_t>(std::get<0>(values), static_cast<std::int32_t>(123));
}

TEST(ScanSuccessTests, ScansIntegerAndString)
{
    auto result = stdx::scan<std::int32_t, std::string>("id=42 name=Bob", "id={%d} name={%s}");

    AssertSuccess(result);

    auto values = result.value().values();
    using tuple_type = decltype(values);

    AssertSameType<std::tuple<std::int32_t, std::string>, tuple_type>();
    AssertValueAndType<std::int32_t>(std::get<0>(values), static_cast<std::int32_t>(42));
    AssertValueAndType<std::string>(std::get<1>(values), std::string("Bob"));
}

TEST(ScanSuccessTests, ScansIntegerStringAndDouble)
{
    auto result =
        stdx::scan<std::int32_t, std::string, double>("id=42 name=Bob score=92.5", "id={%d} name={%s} score={%f}");

    AssertSuccess(result);

    auto values = result.value().values();
    using tuple_type = decltype(values);

    AssertSameType<std::tuple<std::int32_t, std::string, double>, tuple_type>();
    AssertValueAndType<std::int32_t>(std::get<0>(values), static_cast<std::int32_t>(42));
    AssertValueAndType<std::string>(std::get<1>(values), std::string("Bob"));
    AssertFloatingValueAndType<double>(std::get<2>(values), 92.5);
}

TEST(ScanSuccessTests, ScansUnsignedAndFloat)
{
    auto result = stdx::scan<std::uint16_t, float>("count=655 ratio=1.25", "count={%u} ratio={%f}");

    AssertSuccess(result);

    auto values = result.value().values();
    using tuple_type = decltype(values);

    AssertSameType<std::tuple<std::uint16_t, float>, tuple_type>();
    AssertValueAndType<std::uint16_t>(std::get<0>(values), static_cast<std::uint16_t>(655));
    AssertFloatingValueAndType<float>(std::get<1>(values), 1.25f);
}

TEST(ScanSuccessTests, ScansStringView)
{
    auto result = stdx::scan<std::string_view>("name=world", "name={%s}");

    AssertSuccess(result);

    auto values = result.value().values();
    using tuple_type = decltype(values);

    AssertSameType<std::tuple<std::string_view>, tuple_type>();
    AssertValueAndType<std::string_view>(std::get<0>(values), std::string_view("world"));
}

TEST(ScanSuccessTests, ScansCvQualifiedTypes)
{
    auto result = stdx::scan<const std::int32_t, const std::string, volatile double>("id=42 name=Bob score=92.5",
                                                                                     "id={%d} name={%s} score={%f}");

    AssertSuccess(result);

    auto values = result.value().values();
    using tuple_type = decltype(values);

    AssertSameType<std::tuple<const std::int32_t, const std::string, volatile double>, tuple_type>();
    AssertSameType<const std::int32_t, std::tuple_element_t<0, tuple_type>>();
    AssertSameType<const std::string, std::tuple_element_t<1, tuple_type>>();
    AssertSameType<volatile double, std::tuple_element_t<2, tuple_type>>();

    EXPECT_EQ(std::get<0>(values), 42);
    EXPECT_EQ(std::get<1>(values), "Bob");
    EXPECT_DOUBLE_EQ(std::get<2>(values), 92.5);
}

TEST(ScanSuccessTests, ScansEmptyStringForStringPlaceholder)
{
    auto result = stdx::scan<std::string>("prefix--suffix", "prefix-{%s}-suffix");

    AssertSuccess(result);

    auto values = result.value().values();
    using tuple_type = decltype(values);

    AssertSameType<std::tuple<std::string>, tuple_type>();
    AssertValueAndType<std::string>(std::get<0>(values), std::string(""));
}

// scan error tests
TEST(ScanErrorTests, ParseValueError)
{
    auto result = stdx::scan<std::int32_t, std::string>("id=abc name=Bob", "id={%d} name={%s}");

    AssertFailure(result);
    AssertErrorContains(result.error().message, "failed to parse value");
    AssertErrorContains(result.error().message, "abc");
    AssertErrorContains(result.error().message, "int32_t");
}

TEST(ScanErrorTests, FormatMismatchError)
{
    auto result = stdx::scan<std::uint32_t>("123", "{%d}");

    AssertFailure(result);
    AssertErrorContains(result.error().message, "conversion specifier mismatch");
    AssertErrorContains(result.error().message, "expected '%u'");
    AssertErrorContains(result.error().message, "got '%d'");
}

TEST(ScanErrorTests, OutOfRangeError)
{
    auto result = stdx::scan<std::uint8_t>("1000", "{%u}");

    AssertFailure(result);
    AssertErrorContains(result.error().message, "result_out_of_range");
    AssertErrorContains(result.error().message, "uint8_t");
}

TEST(ScanErrorTests, ParseSourcesError)
{
    auto result = stdx::scan<std::int32_t, std::string>("id=42", "id={%d}");

    AssertFailure(result);
    EXPECT_EQ(result.error().message, "Parsing error: Placeholder count does not match number of template arguments");
}

TEST(ScanErrorTests, ReferenceTypeError)
{
    auto result = stdx::scan<std::int32_t &>("id=42", "id={%d}");

    AssertFailure(result);
    EXPECT_EQ(result.error().message, "Scan error: unsupported type in template arguments");
}