#pragma once

#include "sdkconfig.h"

// esp-idf includes
#include <nvs.h>

// local includes
#include "configwrapper.h"

namespace espconfig {

template<typename ConfigContainer>
class ConfigManager : public ConfigContainer
{
public:
    using ConfigContainer::ConfigContainer;

    nvs_handle_t nvs_handle_user{};
#ifdef CONFIG_SEPARATE_FACTORY_NVS_PARTITION
    nvs_handle_t nvs_handle_factory{};
#endif

    esp_err_t init(const char *ns);

    esp_err_t loadFromFlash();

    //bool erase();
    ConfigStatusReturnType reset();

    ConfigWrapperInterface *findConfigByKey(std::string_view key);

    template<typename T>
    ConfigStatusReturnType write_config(ConfigWrapper<T> &config, typename ConfigWrapper<T>::value_t value);

    inline ConfigStatusReturnType reset_config(ConfigWrapperInterface &config);

    inline ConfigStatusReturnType force_reset_config(ConfigWrapperInterface &config);
};

template<typename ConfigContainer> template<typename T>
ConfigStatusReturnType ConfigManager<ConfigContainer>::write_config(ConfigWrapper<T> &config, typename ConfigWrapper<T>::value_t value)
{
    return config.write(nvs_handle_user, value);
}

template<typename ConfigContainer>
inline ConfigStatusReturnType ConfigManager<ConfigContainer>::reset_config(ConfigWrapperInterface &config)
{
    return config.reset(nvs_handle_user);
}

template<typename ConfigContainer>
inline ConfigStatusReturnType ConfigManager<ConfigContainer>::force_reset_config(ConfigWrapperInterface &config)
{
    return config.forceReset(nvs_handle_user);
}

} // namespace espconfig
