/*
 *  Copyright (c) 2019-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>

#include <folly/Range.h>

#include "eden/fs/config/FieldConverter.h"
#include "eden/fs/config/gen-cpp2/eden_config_types.h"

namespace facebook {
namespace eden {

class ConfigSettingBase;

/**
 * ConfigSettingManager is an interface to allow ConfigSettings to be
 * registered. We use it to track all the ConfigSettings in EdenConfig. It
 * allows us to limit the steps involved in adding new settings.
 */
class ConfigSettingManager {
 public:
  virtual ~ConfigSettingManager() {}
  virtual void registerConfiguration(ConfigSettingBase* configSetting) = 0;
};

/**
 *  ConfigSettingBase defines an interface that allows us to treat
 *  configuration settings generically. A ConfigSetting can have multiple
 *  values, one for each configuration source. ConfigSettingBase provides
 *  accessors (setters/getters) that take/return string values. Subclasses,
 *  can provide type based accessors.
 */
class ConfigSettingBase {
 public:
  ConfigSettingBase(folly::StringPiece key, ConfigSettingManager* csm)
      : key_(key) {
    if (csm) {
      csm->registerConfiguration(this);
    }
  }

  ConfigSettingBase(const ConfigSettingBase& source) = default;

  ConfigSettingBase(ConfigSettingBase&& source) = default;

  /**
   * Delete the assignment operator. Our approach is to support in subclasses
   * via 'copyFrom'.
   */
  ConfigSettingBase& operator=(const ConfigSettingBase& rhs) = delete;

  /**
   * Allow sub-classes to selectively support a polymorphic copy operation.
   * This is slightly more clear than having a polymorphic assignment operator.
   */
  virtual void copyFrom(const ConfigSettingBase& rhs) = 0;

  virtual ~ConfigSettingBase() {}
  /**
   * Parse and set the value for the provided ConfigSource.
   * @return Optional will have error message if the value was invalid.
   */
  FOLLY_NODISCARD virtual folly::Expected<folly::Unit, std::string>
  setStringValue(
      folly::StringPiece stringValue,
      const std::map<std::string, std::string>& attrMap,
      ConfigSource newSource) = 0;
  /**
   * Get the ConfigSource of the configuration setting. It is the highest
   * priority ConfigurationSource of all populated values.
   */
  virtual ConfigSource getSource() const = 0;
  /**
   * Get a string representation of the configuration setting.
   */
  virtual std::string getStringValue() const = 0;
  /**
   * Clear the configuration value (if present) for the passed ConfigSource.
   */
  virtual void clearValue(ConfigSource source) = 0;
  /**
   * Get the configuration key (used to identify) this setting. They key is
   * used to identify the entry in a configuration file. Example "core.edenDir"
   */
  virtual const std::string& getConfigKey() const {
    return key_;
  }

 protected:
  std::string key_;
};

/**
 * A Configuration setting is a piece of application configuration that can be
 * constructed by parsing a string. It retains values for various ConfigSources:
 * cli, user config, system config, and default. Access methods will return
 * values for the highest priority source.
 */
template <typename T, typename Converter = FieldConverter<T>>
class ConfigSetting : public ConfigSettingBase {
 public:
  ConfigSetting(
      folly::StringPiece key,
      T value,
      ConfigSettingManager* configSettingManager)
      : ConfigSettingBase(key, configSettingManager) {
    getSlot(ConfigSource::Default).emplace(std::move(value));
  }

  /**
   * Delete the assignment operator. We support copying via 'copyFrom'.
   */
  ConfigSetting<T>& operator=(const ConfigSetting<T>& rhs) = delete;

  ConfigSetting<T>& operator=(const ConfigSetting<T>&& rhs) = delete;

  /**
   * Support copying of ConfigSetting. We limit this to instance of
   * ConfigSetting.
   */
  void copyFrom(const ConfigSettingBase& rhs) override {
    auto rhsConfigSetting = dynamic_cast<const ConfigSetting<T>*>(&rhs);
    if (!rhsConfigSetting) {
      throw std::runtime_error("ConfigSetting copyFrom unknown type");
    }
    key_ = rhsConfigSetting->key_;
    configValueArray_ = rhsConfigSetting->configValueArray_;
  }

  /** Get the highest priority ConfigSource (we ignore unpopulated values).*/
  ConfigSource getSource() const override {
    return static_cast<ConfigSource>(getHighestPriorityIdx());
  }

  /** Get the highest priority value for this setting.*/
  const T& getValue() const {
    return getSlot(getSource()).value();
  }

  /** Get the string value for this setting. Intended for debug purposes. .*/
  std::string getStringValue() const override {
    return Converter{}.toDebugString(getValue());
  }

  /**
   * Set the value based on the passed string. The value is parsed using the
   * template's converter.
   * @return an error in the Optional if the operation failed.
   */
  folly::Expected<folly::Unit, std::string> setStringValue(
      folly::StringPiece stringValue,
      const std::map<std::string, std::string>& attrMap,
      ConfigSource newSource) override {
    if (newSource == ConfigSource::Default) {
      return folly::makeUnexpected<std::string>(
          "Convert ignored for default value");
    }
    Converter c;
    return c.fromString(stringValue, attrMap).then([&](T&& convertResult) {
      getSlot(newSource).emplace(std::move(convertResult));
    });
  }

  /**
   * Set the value with the identified source.
   */
  void setValue(T newVal, ConfigSource newSource, bool force = false) {
    if (force || newSource != ConfigSource::Default) {
      getSlot(newSource).emplace(std::move(newVal));
    }
  }

  /** Clear the value for the passed ConfigSource. The operation will be
   * ignored for ConfigSource::Default. */
  void clearValue(ConfigSource source) override {
    if (source != ConfigSource::Default && getSlot(source).has_value()) {
      getSlot(source).reset();
    }
  }

  virtual ~ConfigSetting() {}

 private:
  static constexpr size_t kConfigSourceLastIndex =
      static_cast<size_t>(apache::thrift::TEnumTraits<ConfigSource>::max());

  std::optional<T>& getSlot(ConfigSource source) {
    return configValueArray_[static_cast<size_t>(source)];
  }
  const std::optional<T>& getSlot(ConfigSource source) const {
    return configValueArray_[static_cast<size_t>(source)];
  }

  /**
   *  Get the index of the highest priority source that is populated.
   */
  size_t getHighestPriorityIdx() const {
    for (auto idx = kConfigSourceLastIndex;
         idx > static_cast<size_t>(ConfigSource::Default);
         --idx) {
      if (configValueArray_[idx].has_value()) {
        return idx;
      }
    }
    return static_cast<size_t>(ConfigSource::Default);
  }

  /**
   * Stores the values, indexed by ConfigSource (as int). Optional is used to
   * allow unpopulated entries. Default values should always be present.
   */
  std::array<std::optional<T>, kConfigSourceLastIndex + 1> configValueArray_;
};

} // namespace eden
} // namespace facebook
