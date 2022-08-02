#pragma once

// system includes
#include <string>

// 3rdparty lib includes
#include <tl/expected.hpp>
#include <fmt/core.h>

// local includes
#include "configwrapper.h"
#include "esphttpdutils.h"

namespace espconfig {
template<int MAX_LENGTH>
ConfigConstraintReturnType StringMaxSize(const std::string &str)
{
    if (str.size() > MAX_LENGTH)
        return tl::make_unexpected(fmt::format("String length  {} exceeds maximum {}", str.size(), MAX_LENGTH));
    return {};
}

template<int MIN_LENGTH, int MAX_LENGTH>
ConfigConstraintReturnType StringMinMaxSize(const std::string &str)
{
    if (str.size() < MIN_LENGTH || str.size() > MAX_LENGTH)
        return tl::make_unexpected(fmt::format("String length  {} exceeds range {} to {}", str.size(), MIN_LENGTH, MAX_LENGTH));
    return {};
}

inline ConfigConstraintReturnType StringEmpty(const std::string &str)
{
    if (!str.empty())
        return tl::make_unexpected("String has to be empty");
    return {};
}

inline ConfigConstraintReturnType StringValidUrl(const std::string &str)
{
    return esphttpdutils::urlverify(str);
}

template<ConfigWrapper<std::string>::ConstraintCallback callback0, ConfigWrapper<std::string>::ConstraintCallback callback1>
ConfigConstraintReturnType StringOr(const std::string &str)
{
    const auto result0 = callback0(str);
    if (result0)
        return {};
    const auto result1 = callback1(str);
    if (result1)
        return {};
    return tl::make_unexpected(fmt::format("None of the following 2 constraints succeded: {} | {}", result0.error(), result1.error()));
}

template<ConfigWrapper<std::string>::ConstraintCallback callback0, ConfigWrapper<std::string>::ConstraintCallback callback1>
ConfigConstraintReturnType StringAnd(const std::string &str)
{
    if (const auto result = callback0(str); !result)
        return tl::make_unexpected(result.error());
    if (const auto result = callback1(str); !result)
        return tl::make_unexpected(result.error());
    return {};
}

template<typename T, T ... ALLOWED_VALUES>
ConfigConstraintReturnType OneOf(typename ConfigWrapper<T>::value_t val)
{
    if (!((ALLOWED_VALUES == val) || ...))
        return tl::make_unexpected("Value not one of the allowed ones");
    return {};
}

template<typename T, T MIN_VALUE>
ConfigConstraintReturnType MinValue(typename ConfigWrapper<T>::value_t val)
{
    if (val < MIN_VALUE)
        return tl::make_unexpected(fmt::format("Value {} exceeds minimum {}", val, MIN_VALUE));
    return {};
}

template<typename T, T MAX_VALUE>
ConfigConstraintReturnType MaxValue(typename ConfigWrapper<T>::value_t val)
{
    if (val > MAX_VALUE)
        return tl::make_unexpected(fmt::format("Value {} exceeds maximum {}", val, MAX_VALUE));
    return {};
}

template<typename T, T MIN_VALUE, T MAX_VALUE>
ConfigConstraintReturnType MinMaxValue(typename ConfigWrapper<T>::value_t val)
{
    if (val < MIN_VALUE || val > MAX_VALUE)
        return tl::make_unexpected(fmt::format("Value {} exceeds range {} to {}", val, MIN_VALUE, MAX_VALUE));
    return {};
}

template<typename T>
ConfigConstraintReturnType MinMaxValue(typename ConfigWrapper<T>::value_t val, T MIN_VALUE, T MAX_VALUE)
{
    if (val < MIN_VALUE || val > MAX_VALUE)
        return tl::make_unexpected(fmt::format("Value {} exceeds range {} to {}", val, MIN_VALUE, MAX_VALUE));
    return {};
}

template<typename T, T MIN_VALUE, T MAX_VALUE>
ConfigConstraintReturnType MinMaxOrZeroValue(typename ConfigWrapper<T>::value_t val)
{
    if (val != 0 && (val < MIN_VALUE || val > MAX_VALUE))
        return tl::make_unexpected(fmt::format("Value {} exceeds constraint 0 or range {} to {}", val, MIN_VALUE, MAX_VALUE));
    return {};
}
} // namespace espconfig
