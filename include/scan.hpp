#pragma once

#include "parse.hpp"
#include "types.hpp"

namespace stdx
{
template <typename... Ts, std::size_t... Is>
std::expected<details::scan_result<Ts...>, details::scan_error>
scan_impl(const std::vector<std::string_view> &format_parts,
          const std::vector<std::string_view> &input_parts,
          std::index_sequence<Is...>)
{
    std::tuple<std::remove_cv_t<Ts>...> parsed_values;
    std::expected<void, details::scan_error> status{};

    (
        [&]
        {
            if (status.has_value())
            {
                auto parsed = details::parse_value_with_format<Ts>(input_parts[Is], format_parts[Is]);

                if (!parsed.has_value())
                {
                    status = std::unexpected(parsed.error());
                    return;
                }

                std::get<Is>(parsed_values) = std::move(parsed.value());
            }
        }(),
        ...);

    if (!status.has_value())
        return std::unexpected(status.error());

    return details::scan_result<Ts...>{std::move(parsed_values)};
}

template <typename... Ts>
std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input, std::string_view format)
{
    auto parsed_sources = details::parse_sources<Ts...>(input, format);

    if (!parsed_sources)
        return std::unexpected(parsed_sources.error());

    const auto &[format_parts, input_parts] = parsed_sources.value();

    return scan_impl<Ts...>(format_parts, input_parts, std::index_sequence_for<Ts...>{});
}

}  // namespace stdx
