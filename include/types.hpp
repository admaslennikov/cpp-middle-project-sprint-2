#pragma once

#include <string>
#include <tuple>

namespace stdx::details
{
// Класс для хранения ошибки неуспешного сканирования
struct scan_error
{
    std::string message;
};

// Шаблонный класс для хранения результатов успешного сканирования
template <typename... Ts>
struct scan_result
{
    scan_result() = default;

    explicit scan_result(std::tuple<Ts...> values) : m_values(std::move(values)) {}

    [[nodiscard]]
    auto values() const -> std::tuple<Ts...>
    {
        return m_values;
    }

private:
    std::tuple<Ts...> m_values;
};
}  // namespace stdx::details
