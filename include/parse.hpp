#pragma once

#include <charconv>
#include <concepts>
#include <cstdint>
#include <expected>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include "types.hpp"

namespace stdx::details
{

namespace traits
{
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T>
struct is_supported_base_type : std::false_type
{
};

template <>
struct is_supported_base_type<std::int8_t> : std::true_type
{
};
template <>
struct is_supported_base_type<std::int16_t> : std::true_type
{
};
template <>
struct is_supported_base_type<std::int32_t> : std::true_type
{
};
template <>
struct is_supported_base_type<std::int64_t> : std::true_type
{
};
template <>
struct is_supported_base_type<std::uint8_t> : std::true_type
{
};
template <>
struct is_supported_base_type<std::uint16_t> : std::true_type
{
};
template <>
struct is_supported_base_type<std::uint32_t> : std::true_type
{
};
template <>
struct is_supported_base_type<std::uint64_t> : std::true_type
{
};
template <>
struct is_supported_base_type<float> : std::true_type
{
};
template <>
struct is_supported_base_type<double> : std::true_type
{
};
template <>
struct is_supported_base_type<std::string> : std::true_type
{
};
template <>
struct is_supported_base_type<std::string_view> : std::true_type
{
};

template <typename T>
inline constexpr bool is_supported_type_v = !std::is_reference_v<T> && is_supported_base_type<remove_cvref_t<T>>::value;

template <typename T>
inline constexpr bool is_signed_integer_v =
    std::same_as<remove_cvref_t<T>, std::int8_t> || std::same_as<remove_cvref_t<T>, std::int16_t> ||
    std::same_as<remove_cvref_t<T>, std::int32_t> || std::same_as<remove_cvref_t<T>, std::int64_t>;

template <typename T>
inline constexpr bool is_unsigned_integer_v =
    std::same_as<remove_cvref_t<T>, std::uint8_t> || std::same_as<remove_cvref_t<T>, std::uint16_t> ||
    std::same_as<remove_cvref_t<T>, std::uint32_t> || std::same_as<remove_cvref_t<T>, std::uint64_t>;

template <typename T>
inline constexpr bool is_floating_point_v =
    std::same_as<remove_cvref_t<T>, float> || std::same_as<remove_cvref_t<T>, double>;

template <typename T>
inline constexpr bool is_string_like_v =
    std::same_as<remove_cvref_t<T>, std::string> || std::same_as<remove_cvref_t<T>, std::string_view>;
}  // namespace traits

template <typename T>
concept scan_supported_type = traits::is_supported_type_v<T>;

template <typename T>
concept signed_integer_scan_type = scan_supported_type<T> && traits::is_signed_integer_v<T>;

template <typename T>
concept unsigned_integer_scan_type = scan_supported_type<T> && traits::is_unsigned_integer_v<T>;

template <typename T>
concept floating_scan_type = scan_supported_type<T> && traits::is_floating_point_v<T>;

template <typename T>
concept string_scan_type = scan_supported_type<T> && traits::is_string_like_v<T>;

template <typename T>
using base_t = std::remove_cv_t<T>;

template <typename T>
constexpr std::string_view type_name()
{
    using U = traits::remove_cvref_t<T>;

    if constexpr (std::same_as<U, std::int8_t>)
        return "int8_t";

    if constexpr (std::same_as<U, std::int16_t>)
        return "int16_t";

    if constexpr (std::same_as<U, std::int32_t>)
        return "int32_t";

    if constexpr (std::same_as<U, std::int64_t>)
        return "int64_t";

    if constexpr (std::same_as<U, std::uint8_t>)
        return "uint8_t";

    if constexpr (std::same_as<U, std::uint16_t>)
        return "uint16_t";

    if constexpr (std::same_as<U, std::uint32_t>)
        return "uint32_t";

    if constexpr (std::same_as<U, std::uint64_t>)
        return "uint64_t";

    if constexpr (std::same_as<U, float>)
        return "float";

    if constexpr (std::same_as<U, double>)
        return "double";

    if constexpr (std::same_as<U, std::string>)
        return "std::string";

    if constexpr (std::same_as<U, std::string_view>)
        return "std::string_view";

    return "unknown";
}

inline std::string errc_to_string(std::errc ec)
{
    if (ec == std::errc::invalid_argument)
        return "invalid_argument";

    if (ec == std::errc::result_out_of_range)
        return "result_out_of_range";

    return std::make_error_code(ec).message();
}

template <typename T>
std::expected<T, scan_error> make_error(std::string message)
{
    std::string full;
    full.reserve(sizeof("Conversion error: ") - 1 + message.size());

    full += "Conversion error: ";
    full += message;

    return std::unexpected(scan_error{std::move(full)});
}

template <typename T>
std::string make_context_prefix(std::string_view input)
{
    std::string result;
    result.reserve(64 + input.size());

    result += "failed to parse value '";
    result += input;
    result += "' as ";
    result += type_name<T>();
    result += ": ";

    return result;
}

template <typename T>
std::string make_from_chars_error_message(std::string_view input, std::errc ec)
{
    std::string msg = make_context_prefix<T>(input);

    msg += "from_chars returned ";
    msg += errc_to_string(ec);

    return msg;
}

template <typename T>
std::string make_trailing_chars_error_message(std::string_view input)
{
    std::string msg = make_context_prefix<T>(input);

    msg += "input contains trailing characters";

    return msg;
}

template <typename T>
std::string make_range_error_message(std::string_view input, std::string_view lower, std::string_view upper)
{
    std::string msg = make_context_prefix<T>(input);

    msg += "value is out of range [";
    msg += lower;
    msg += ", ";
    msg += upper;
    msg += "]";

    return msg;
}

template <signed_integer_scan_type T>
std::expected<T, scan_error> parse_signed_integral(std::string_view input)
{
    std::int64_t parsed{};
    const char *first = input.data();
    const char *last = input.data() + input.size();

    const auto [ptr, ec] = std::from_chars(first, last, parsed);

    if (ec != std::errc{})
        return make_error<T>(make_from_chars_error_message<T>(input, ec));

    if (ptr != last)
        return make_error<T>(make_trailing_chars_error_message<T>(input));

    constexpr auto min_v = std::numeric_limits<base_t<T>>::min();
    constexpr auto max_v = std::numeric_limits<base_t<T>>::max();

    if (parsed < static_cast<std::int64_t>(min_v) || parsed > static_cast<std::int64_t>(max_v))
    {
        return make_error<T>(make_range_error_message<T>(
            input, std::to_string(static_cast<long long>(min_v)), std::to_string(static_cast<long long>(max_v))));
    }

    return static_cast<base_t<T>>(parsed);
}

template <unsigned_integer_scan_type T>
std::expected<T, scan_error> parse_unsigned_integral(std::string_view input)
{
    std::uint64_t parsed{};
    const char *first = input.data();
    const char *last = input.data() + input.size();

    const auto [ptr, ec] = std::from_chars(first, last, parsed);

    if (ec != std::errc{})
        return make_error<T>(make_from_chars_error_message<T>(input, ec));

    if (ptr != last)
        return make_error<T>(make_trailing_chars_error_message<T>(input));

    constexpr auto max_v = std::numeric_limits<base_t<T>>::max();

    if (parsed > static_cast<std::uint64_t>(max_v))
    {
        return make_error<T>(
            make_range_error_message<T>(input, "0", std::to_string(static_cast<unsigned long long>(max_v))));
    }

    return static_cast<base_t<T>>(parsed);
}

template <floating_scan_type T>
std::expected<T, scan_error> parse_floating(std::string_view input)
{
    base_t<T> parsed{};

    const char *first = input.data();
    const char *last = input.data() + input.size();

    const auto [ptr, ec] = std::from_chars(first, last, parsed);

    if (ec != std::errc{})
        return make_error<T>(make_from_chars_error_message<T>(input, ec));

    if (ptr != last)
        return make_error<T>(make_trailing_chars_error_message<T>(input));

    return parsed;
}

template <string_scan_type T>
std::expected<T, scan_error> parse_string_like(std::string_view input)
{
    if constexpr (std::same_as<base_t<T>, std::string_view>)
        return input;

    return std::string(input);
}

template <scan_supported_type T>
constexpr std::string_view expected_format()
{
    if constexpr (traits::is_signed_integer_v<T>)
        return "%d";

    if constexpr (traits::is_unsigned_integer_v<T>)
        return "%u";

    if constexpr (traits::is_floating_point_v<T>)
        return "%f";

    return "%s";
}

template <scan_supported_type T>
std::string make_format_mismatch_message(std::string_view input, std::string_view fmt)
{
    std::string msg;
    msg.reserve(128 + input.size() + fmt.size());

    msg += "conversion specifier mismatch for value '";
    msg += input;
    msg += "' and type ";
    msg += type_name<T>();
    msg += ": expected '";
    msg += expected_format<T>();
    msg += "', got '";
    msg += fmt;
    msg += "'";

    return msg;
}

// Семейство функций parse_value
template <signed_integer_scan_type T>
std::expected<T, scan_error> parse_value(std::string_view input)
{
    return parse_signed_integral<T>(input);
}

template <unsigned_integer_scan_type T>
std::expected<T, scan_error> parse_value(std::string_view input)
{
    return parse_unsigned_integral<T>(input);
}

template <floating_scan_type T>
std::expected<T, scan_error> parse_value(std::string_view input)
{
    return parse_floating<T>(input);
}

template <string_scan_type T>
std::expected<T, scan_error> parse_value(std::string_view input)
{
    return parse_string_like<T>(input);
}

// Функция для парсинга значения с учетом спецификатора формата
template <scan_supported_type T>
std::expected<T, scan_error> parse_value_with_format(std::string_view input, std::string_view fmt)
{
    if (!fmt.empty())
    {
        if (!(fmt == "%d" || fmt == "%u" || fmt == "%f" || fmt == "%s"))
        {
            return make_error<T>("invalid format '" + std::string(fmt) +
                                 "': expected one of %d, %u, %f, %s or empty format");
        }

        const std::string_view expected = expected_format<T>();

        if (fmt != expected)
            return make_error<T>(make_format_mismatch_message<T>(input, fmt));
    }

    return parse_value<T>(input);
}

// Функция для проверки корректности входных данных и выделения из обеих строк интересующих данных для парсинга
template <typename... Ts>
std::expected<std::pair<std::vector<std::string_view>, std::vector<std::string_view>>, scan_error>
parse_sources(std::string_view input, std::string_view format)
{
    std::vector<std::string_view> format_parts;
    std::vector<std::string_view> input_parts;

    std::size_t start = 0;
    bool prev_was_placeholder = false;

    auto consume_literal = [&](std::string_view literal, bool split_before_literal) -> std::expected<void, scan_error>
    {
        if (split_before_literal)
        {
            auto pos = input.find(literal);

            if (pos == std::string_view::npos)
            {
                return std::unexpected(
                    scan_error{"Parsing error: Unformatted text in input and format string are different"});
            }

            input_parts.emplace_back(input.substr(0, pos));
            input.remove_prefix(pos);
        }

        if (!input.starts_with(literal))
        {
            return std::unexpected(
                scan_error{"Parsing error: Unformatted text in input and format string are different"});
        }

        input.remove_prefix(literal.size());
        return {};
    };

    while (true)
    {
        std::size_t open = format.find('{', start);
        std::size_t close = format.find('}', start);

        if (close != std::string_view::npos && (open == std::string_view::npos || close < open))
            return std::unexpected(scan_error{"Parsing error: Unmatched '}' in format string"});

        if (open == std::string_view::npos)
            break;

        close = format.find('}', open + 1);

        if (close == std::string_view::npos)
            return std::unexpected(scan_error{"Parsing error: Unmatched '{' in format string"});

        if (open > start)
        {
            std::string_view between = format.substr(start, open - start);

            auto result = consume_literal(between, start != 0);

            if (!result)
                return std::unexpected(result.error());

            prev_was_placeholder = false;
        }
        else
        {
            if (prev_was_placeholder)
                return std::unexpected(scan_error{"Parsing error: Adjacent placeholders are not allowed"});
        }

        format_parts.push_back(format.substr(open + 1, close - open - 1));
        start = close + 1;
        prev_was_placeholder = true;
    }

    if (start < format.size())
    {
        std::string_view remaining_format = format.substr(start);

        auto result = consume_literal(remaining_format, !format_parts.empty());

        if (!result)
            return std::unexpected(result.error());

        if (!input.empty())
            return std::unexpected(scan_error{"Parsing error: Input contains extra characters"});
    }
    else
    {
        input_parts.emplace_back(input);
    }

    if (format_parts.size() != sizeof...(Ts))
    {
        return std::unexpected(
            scan_error{"Parsing error: Placeholder count does not match number of template arguments"});
    }

    return std::pair{format_parts, input_parts};
}
}  // namespace stdx::details