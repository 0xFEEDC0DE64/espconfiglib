#pragma once

#include "configmanager.h"

#include "sdkconfig.h"
#define LOG_LOCAL_LEVEL CONFIG_LOG_LOCAL_LEVEL_CONFIG

// esp-idf includes
#include <esp_log.h>
#include <nvs_flash.h>

// local includes
#include "espchrono.h"

#define INSTANTIATE_CONFIGMANAGER_TEMPLATES(Type) \
    template esp_err_t ConfigManager<Type>::init(); \
    /* template bool ConfigManager<Type>::erase(); */ \
    template ConfigStatusReturnType ConfigManager<Type>::reset(); \
    template ConfigWrapperInterface *ConfigManager<Type>::findConfigByKey(std::string_view key);

namespace espconfig {
namespace {
constexpr const char * const TAG = "CONFIG";
} // namespace

template<typename ConfigContainer>
esp_err_t ConfigManager<ConfigContainer>::init()
{
    ESP_LOGD(TAG, "called");

    auto before = espchrono::millis_clock::now();
#ifndef CONFIG_NVS_ENCRYPTION
    {
        const auto result = nvs_flash_init_partition("nvs");
        ESP_LOG_LEVEL_LOCAL((result == ESP_OK ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "nvs_flash_init() for user returned: %s", esp_err_to_name(result));
        if (result != ESP_OK)
            return result;
    }

#ifdef CONFIG_SEPARATE_FACTORY_NVS_PARTITION
    {
        const auto result = nvs_flash_init_partition("nvsFac");
        ESP_LOG_LEVEL_LOCAL((result == ESP_OK ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "nvs_flash_init() for factory returned: %s", esp_err_to_name(result));
        if (result != ESP_OK)
            return result;
    }
#endif
#else
    const esp_partition_t *key_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, NULL);
    if (!key_part)
    {
        ESP_LOGE(TAG, "nvs_keys partition not found!");
        return ESP_ERR_NVS_PART_NOT_FOUND;
    }

    nvs_sec_cfg_t cfg;

    {
        const auto result = nvs_flash_read_security_cfg(key_part, &cfg);
        ESP_LOG_LEVEL_LOCAL((result == ESP_OK ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "nvs_flash_read_security_cfg() returned: %s", esp_err_to_name(result));
        if (result != ESP_OK)
        {
            if (result == ESP_ERR_NVS_KEYS_NOT_INITIALIZED)
            {
                const auto result = nvs_flash_generate_keys(key_part, &cfg);
                ESP_LOG_LEVEL_LOCAL((result == ESP_OK ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "nvs_flash_generate_keys() returned: %s", esp_err_to_name(result));
                if (result != ESP_OK)
                    return result;
            }
            else
                return result;
        }
    }

    {
        const auto result = nvs_flash_secure_init_partition("nvs", &cfg);
        ESP_LOG_LEVEL_LOCAL((result == ESP_OK ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "nvs_flash_secure_init() for user returned: %s", esp_err_to_name(result));
        if (result != ESP_OK)
            return result;
    }

#ifdef CONFIG_SEPARATE_FACTORY_NVS_PARTITION
    {
        const auto result = nvs_flash_secure_init_partition("nvsFac", &cfg);
        ESP_LOG_LEVEL_LOCAL((result == ESP_OK ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "nvs_flash_secure_init() for user returned: %s", esp_err_to_name(result));
        if (result != ESP_OK)
            return result;
    }
#endif
#endif

    {
        const auto result = nvs_open_from_partition("nvs", "recharge", NVS_READWRITE, &nvs_handle_user);
        ESP_LOG_LEVEL_LOCAL((result == ESP_OK ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "nvs_open() for user partition returned: %s", esp_err_to_name(result));
        if (result != ESP_OK)
            return result;
    }

#ifdef CONFIG_SEPARATE_FACTORY_NVS_PARTITION
    {
        const auto result = nvs_open_from_partition("nvsFac", "recharge", NVS_READWRITE, &nvs_handle_factory);
        ESP_LOG_LEVEL_LOCAL((result == ESP_OK ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "nvs_open() for factory partition returned: %s", esp_err_to_name(result));
        if (result != ESP_OK)
            return result;
    }
#endif

    auto after = espchrono::millis_clock::now();

    ESP_LOGI(TAG, "initializing NVS took %lldms", std::chrono::milliseconds{after-before}.count());

    before = espchrono::millis_clock::now();

    bool success = true;
    for (ConfigWrapperInterface &config : ConfigContainer::getAllConfigParams())
        if (const auto result = config.loadFromFlash(nvs_handle_user); !result)
        {
            ESP_LOGE(TAG, "config parameter %s failed to load: %.*s", config.nvsName(), result.error().size(), result.error().data());
            success = false;
        }

    after = espchrono::millis_clock::now();

    ESP_LOGI(TAG, "loading all config params took %lldms", std::chrono::milliseconds{after-before}.count());

    return success ? ESP_OK : ESP_FAIL;
}

//template<typename ConfigContainer>
//bool ConfigManager<ConfigContainer>::erase()
//{
//    ESP_LOGW(TAG, "called");

//    nvs_close(nvs_handle_user);
//    const auto err = nvs_flash_erase();
//    ESP_LOG_LEVEL_LOCAL((err == ESP_OK ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "nvs_flash_erase() returned: %s", esp_err_to_name(err));
//    if (err != ESP_OK)
//        return false;

//    return init();
//}

template<typename ConfigContainer>
ConfigStatusReturnType ConfigManager<ConfigContainer>::reset()
{
    ESP_LOGW(TAG, "called");

    std::string message;

    for (ConfigWrapperInterface &config : ConfigContainer::getAllConfigParams())
        if (const auto result = reset_config(config); !result)
        {
            if (!message.empty())
                message.append(", ");
            message.append(fmt::format("reset of {} failed: {}", config.nvsName(), result.error()));
        }

    if (!message.empty())
        return tl::make_unexpected(std::move(message));

    return {};
}

template<typename ConfigContainer>
ConfigWrapperInterface *ConfigManager<ConfigContainer>::findConfigByKey(std::string_view key)
{
    const auto configParams = ConfigContainer::getAllConfigParams();
    const auto iter = std::find_if(std::cbegin(configParams), std::cend(configParams),
                                   [&key](const ConfigWrapperInterface &config){ return key == config.nvsName(); });
    return iter != std::cend(configParams) ? &iter->get() : nullptr;
}

} // namespace espconfig