/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/ThreadLocal.h>
#include <chrono>
#include <memory>

#include "eden/fs/config/CachedParsedFileMonitor.h"
#include "eden/fs/config/ReloadableConfig.h"
#include "eden/fs/tracing/EdenStats.h"
#ifdef _WIN32
#include "eden/fs/win/utils/Stub.h" // @manual
#include "eden/fs/win/utils/UserInfo.h" // @manual
#else
#include "eden/fs/fuse/privhelper/UserInfo.h"
#endif
#include "eden/fs/model/git/GitIgnoreFileParser.h"
#include "eden/fs/utils/PathFuncs.h"

namespace facebook {
namespace eden {

class Clock;
class EdenConfig;
class FaultInjector;
class PrivHelper;
class ProcessNameCache;
class TopLevelIgnores;
class UnboundedQueueExecutor;

/**
 * ServerState contains state shared across multiple mounts.
 *
 * This is normally owned by the main EdenServer object.  However unit tests
 * also create ServerState objects without an EdenServer.
 */
class ServerState {
 public:
  ServerState(
      UserInfo userInfo,
      std::shared_ptr<PrivHelper> privHelper,
      std::shared_ptr<UnboundedQueueExecutor> threadPool,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<ProcessNameCache> processNameCache,
      std::shared_ptr<const EdenConfig> edenConfig);
  ~ServerState();

  /**
   * Set the path to the server's thrift socket.
   *
   * This is called by EdenServer once it has initialized the thrift server.
   */
  void setSocketPath(AbsolutePathPiece path) {
    socketPath_ = path.copy();
  }

  /**
   * Get the path to the server's thrift socket.
   *
   * This is used by the EdenMount to populate the `.eden/socket` special file.
   */
  const AbsolutePath& getSocketPath() const {
    return socketPath_;
  }

  /**
   * Get the EdenStats object that tracks process-wide (rather than per-mount)
   * statistics.
   */
  EdenStats& getStats() {
    return edenStats_;
  }

  ReloadableConfig& getReloadableConfig() {
    return config_;
  }
  const ReloadableConfig& getReloadableConfig() const {
    return config_;
  }

  /**
   * Get the EdenConfig data.
   *
   * The config data may be reloaded from disk depending on the value of the
   * reload parameter.
   */
  std::shared_ptr<const EdenConfig> getEdenConfig(
      ConfigReloadBehavior reload = ConfigReloadBehavior::AutoReload) {
    return config_.getEdenConfig(reload);
  }

  /**
   * Get the TopLevelIgnores. It is based on the system and user git ignore
   * files.
   */
  std::unique_ptr<TopLevelIgnores> getTopLevelIgnores();

  /**
   * Get the UserInfo object describing the user running this edenfs process.
   */
  const UserInfo& getUserInfo() const {
    return userInfo_;
  }

  /**
   * Get the PrivHelper object used to perform operations that require
   * elevated privileges.
   */
  PrivHelper* getPrivHelper() {
    return privHelper_.get();
  }

  /**
   * Get the thread pool.
   *
   * Adding new tasks to this thread pool executor will never block.
   */
  const std::shared_ptr<UnboundedQueueExecutor>& getThreadPool() const {
    return threadPool_;
  }

  /**
   * Get the Clock.
   */
  const std::shared_ptr<Clock>& getClock() const {
    return clock_;
  }

  const std::shared_ptr<ProcessNameCache>& getProcessNameCache() const {
    return processNameCache_;
  }

  FaultInjector& getFaultInjector() {
    return *faultInjector_;
  }

 private:
  AbsolutePath socketPath_;
  UserInfo userInfo_;
  EdenStats edenStats_;
  std::shared_ptr<PrivHelper> privHelper_;
  std::shared_ptr<UnboundedQueueExecutor> threadPool_;
  std::shared_ptr<Clock> clock_;
  std::shared_ptr<ProcessNameCache> processNameCache_;
  std::unique_ptr<FaultInjector> const faultInjector_;

  ReloadableConfig config_;
  folly::Synchronized<CachedParsedFileMonitor<GitIgnoreFileParser>>
      userIgnoreFileMonitor_;
  folly::Synchronized<CachedParsedFileMonitor<GitIgnoreFileParser>>
      systemIgnoreFileMonitor_;
};
} // namespace eden
} // namespace facebook
