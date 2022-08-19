#pragma once

#include "sdkconfig.h"

#pragma push_macro("LOG_LOCAL_LEVEL")
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL CONFIG_LOG_LOCAL_LEVEL_CONFIG

// system includes
#include <utility>
#include <cassert>

// esp-idf includes
#include <esp_log.h>

// 3rdparty lib includes
#include <fmt/core.h>

// local includes
#include "configwrapper.h"
#include "cpputils.h"

#define INSTANTIATE_CONFIGWRAPPER_TEMPLATES(TYPE) \
    namespace espconfig { \
    template ConfigStatusReturnType ConfigWrapper<TYPE>::write(nvs_handle_t nvsHandle, value_t value); \
    template<> const char *ConfigWrapper<TYPE>::type() const { return #TYPE; } \
    template std::string ConfigWrapper<TYPE>::valueAsString() const; \
    template std::string ConfigWrapper<TYPE>::defaultAsString() const; \
    /* template TYPE ConfigWrapper<TYPE>::defaultValue() const; */ \
    template ConfigStatusReturnType ConfigWrapper<TYPE>::loadFromFlash(nvs_handle_t nvsHandle); \
    template ConfigStatusReturnType ConfigWrapper<TYPE>::reset(nvs_handle_t nvsHandle); \
    template ConfigStatusReturnType ConfigWrapper<TYPE>::forceReset(nvs_handle_t nvsHandle); \
    /* template ConfigConstraintReturnType ConfigWrapper<TYPE>::checkValue(value_t value) const; */ \
    template ConfigStatusReturnType ConfigWrapper<TYPE>::writeToFlash(nvs_handle_t nvsHandle, value_t value); \
    } // namespace espconfig

namespace espconfig {
namespace {
constexpr const char * const TAG = "CONFIG";
} // namespace

template<typename T>
ConfigStatusReturnType ConfigWrapper<T>::write(nvs_handle_t nvsHandle, value_t value)
{
    CONFIGWRAPPER_TOSTRING_USINGS

    ESP_LOGD(TAG, "%s %s", nvsName(), toString(value).c_str());

    if (!m_loaded)
        ESP_LOGE(TAG, "%s has not been loaded yet!", nvsName());

    if (const auto result = checkValue(value); !result)
    {
        ESP_LOGW(TAG, "%s cannot be set to %s: constraint not met: %s", nvsName(), toString(value).c_str(), result.error().c_str());
        return result;
    }

    if (m_value == value)
        return {};

    return writeToFlash(nvsHandle, value);
}

template<typename T>
std::string ConfigWrapper<T>::valueAsString() const
{
    CONFIGWRAPPER_TOSTRING_USINGS

    if (m_touched)
        return toString(m_value);
    else
        return "--not-touched-- " + toString(m_value);
}

template<typename T>
std::string ConfigWrapper<T>::defaultAsString() const
{
    CONFIGWRAPPER_TOSTRING_USINGS

    return toString(defaultValue());
}

template<typename T>
ConfigStatusReturnType ConfigWrapper<T>::loadFromFlash(nvs_handle_t nvsHandle)
{
    //ESP_LOGD(TAG, "%s", nvsName());

    const auto result = nvs_get(nvsHandle, nvsName(), &m_value);

    ESP_LOG_LEVEL_LOCAL((cpputils::is_in(result, ESP_OK, ESP_ERR_NVS_NOT_FOUND) ? ESP_LOG_DEBUG : ESP_LOG_ERROR), TAG, "%s nvs_get() returned: %s", nvsName(), esp_err_to_name(result));

    if (result == ESP_OK)
    {
        if (const auto result = checkValue(m_value); !result)
        {
            ESP_LOGE(TAG, "%s constraint not met for value in flash: %s", nvsName(), result.error().c_str());
            return forceReset(nvsHandle);
        }

        m_loaded = true;
        m_touched = true;
        return {};
    }
    else if (result == ESP_ERR_NVS_NOT_FOUND)
    {
        m_loaded = true;
        m_touched = false;
        m_value = defaultValue();
        if (const auto result = checkValue(m_value); !result)
            ESP_LOGE(TAG, "%s constraint not met for value from default: %s", nvsName(), result.error().c_str());

        return {};
    }
    else
    {
        return forceReset(nvsHandle);
    }
}

template<typename T>
ConfigStatusReturnType ConfigWrapper<T>::reset(nvs_handle_t nvsHandle)
{
    ESP_LOGD(TAG, "%s", nvsName());

    if (!allowReset())
        return {};

    return forceReset(nvsHandle);
}

template<typename T>
ConfigStatusReturnType ConfigWrapper<T>::forceReset(nvs_handle_t nvsHandle)
{
    ESP_LOGD(TAG, "%s", nvsName());

    auto result = nvs_erase_key(nvsHandle, nvsName());

    ESP_LOG_LEVEL_LOCAL((cpputils::is_in(result, ESP_OK, ESP_ERR_NVS_NOT_FOUND) ? ESP_LOG_DEBUG : ESP_LOG_ERROR), TAG, "%s nvs_erase_key() returned: %s", nvsName(), esp_err_to_name(result));

    if (result == ESP_ERR_NVS_NOT_FOUND)
    {
        if (m_touched)
            ESP_LOGE(TAG, "%s for touched and not found?!", nvsName());
        result = ESP_OK;
    }

    if (result == ESP_OK || !m_loaded)
    {
        if (!m_loaded /* && result == ESP_OK */)
            m_loaded = true;
        m_touched = false;
        m_value = defaultValue();
    }

    if (result != ESP_OK)
        return tl::make_unexpected(std::string{"nvs_erase_key() failed with "} + esp_err_to_name(result));

    return {};
}

template<typename T>
ConfigStatusReturnType ConfigWrapper<T>::writeToFlash(nvs_handle_t nvsHandle, value_t value)
{
    CONFIGWRAPPER_TOSTRING_USINGS

    ESP_LOGD(TAG, "%s %s", nvsName(), toString(value).c_str());

    const auto result = nvs_set(nvsHandle, nvsName(), value);

    ESP_LOG_LEVEL_LOCAL((result == ESP_OK ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "%s %s nvs_set() returned %s", nvsName(), toString(value).c_str(), esp_err_to_name(result));

    m_value = value;
    m_touched = true;

    if (result != ESP_OK)
        return tl::make_unexpected(std::string{"nvs_set() failed with "} + esp_err_to_name(result));
    return {};
}

} // namespace espconfig

#pragma pop_macro("LOG_LOCAL_LEVEL")
