#pragma once

namespace Macros
{
    template <typename T>
    inline constexpr T const IF_ELSE(bool expression, T trueValue, T falseValue)
    {
        T results[] = {falseValue, trueValue};
        return results[expression];
    };
} // namespace Macros