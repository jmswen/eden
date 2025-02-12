/*
 *  Copyright (c) 2019-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "eden/fs/service/EdenInit.h"

#include <boost/filesystem.hpp>
#include <folly/portability/GFlags.h>

#include "eden/fs/config/EdenConfig.h"
#include "eden/fs/fuse/privhelper/UserInfo.h"
#include "eden/fs/utils/PathFuncs.h"

using folly::StringPiece;

DEFINE_string(configPath, "", "The path of the ~/.edenrc config file");
DEFINE_string(edenDir, "", "The path to the .eden directory");
DEFINE_string(
    etcEdenDir,
    "/etc/eden",
    "The directory holding all system configuration files");

namespace {
using namespace facebook::eden;

constexpr StringPiece kDefaultUserConfigFile{".edenrc"};
constexpr StringPiece kEdenfsConfigFile{"edenfs.rc"};

void findEdenDir(EdenConfig& config) {
  // Get the initial path to the Eden directory.
  // We use the --edenDir flag if set, otherwise the value loaded from the
  // config file.
  boost::filesystem::path boostPath(
      FLAGS_edenDir.empty() ? config.getEdenDir().value() : FLAGS_edenDir);

  try {
    // Ensure that the directory exists, and then canonicalize its name with
    // realpath().  Using realpath() requires that the directory exist.
    boost::filesystem::create_directories(boostPath);
    auto resolvedDir = facebook::eden::realpath(boostPath.string());

    // Updating the value in the config using ConfigSource::CommandLine also
    // makes sure that any future updates to the config file do not affect the
    // value we use.  Once we start we want to always use a fixed location for
    // the eden directory.
    config.setEdenDir(resolvedDir, ConfigSource::CommandLine);
  } catch (const std::exception& ex) {
    throw ArgumentError(
        "error creating ", boostPath.string(), ": ", folly::exceptionStr(ex));
  }
}

} // namespace

namespace facebook {
namespace eden {

std::unique_ptr<EdenConfig> getEdenConfig(UserInfo& identity) {
  // normalizeBestEffort() to try resolving symlinks in these paths but don't
  // fail if they don't exist.
  AbsolutePath systemConfigDir;
  try {
    systemConfigDir = normalizeBestEffort(FLAGS_etcEdenDir);
  } catch (const std::exception& ex) {
    throw ArgumentError(
        "invalid flag value: ",
        FLAGS_etcEdenDir,
        ": ",
        folly::exceptionStr(ex));
  }
  const auto systemConfigPath =
      systemConfigDir + PathComponentPiece{kEdenfsConfigFile};

  const std::string configPathStr = FLAGS_configPath;
  AbsolutePath userConfigPath;
  if (configPathStr.empty()) {
    userConfigPath = identity.getHomeDirectory() +
        PathComponentPiece{kDefaultUserConfigFile};
  } else {
    try {
      userConfigPath = normalizeBestEffort(configPathStr);
    } catch (const std::exception& ex) {
      throw ArgumentError(
          "invalid flag value: ",
          FLAGS_configPath,
          ": ",
          folly::exceptionStr(ex));
    }
  }
  // Create the default EdenConfig. Next, update with command line arguments.
  // Command line arguments will take precedence over config file settings.
  auto edenConfig = std::make_unique<EdenConfig>(
      identity.getUsername(),
      identity.getUid(),
      identity.getHomeDirectory(),
      userConfigPath,
      systemConfigDir,
      systemConfigPath);

  // Load system and user configurations
  edenConfig->loadSystemConfig();
  edenConfig->loadUserConfig();

  // Determine the location of the Eden state directory, and update this value
  // in the EdenConfig object.  This also creates the directory if it does not
  // exist.
  findEdenDir(*edenConfig);

  return edenConfig;
}

} // namespace eden
} // namespace facebook
