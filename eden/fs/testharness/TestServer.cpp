/*
 *  Copyright (c) 2019-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "eden/fs/testharness/TestServer.h"

#include <folly/portability/GFlags.h>

#include "eden/fs/config/EdenConfig.h"
#include "eden/fs/fuse/privhelper/UserInfo.h"
#include "eden/fs/service/EdenServer.h"
#include "eden/fs/service/StartupLogger.h"
#include "eden/fs/testharness/FakePrivHelper.h"
#include "eden/fs/testharness/TempFile.h"

using std::make_shared;
using std::make_unique;
using std::unique_ptr;
using namespace facebook::eden::path_literals;

namespace facebook {
namespace eden {

TestServer::TestServer()
    : tmpDir_(makeTempDir()), server_(createServer(getTmpDir())) {
  auto prepareResult = server_->prepare(make_shared<ForegroundStartupLogger>());
  // We don't care about waiting for prepareResult: it just indicates when
  // preparation has fully completed, but the EdenServer can begin being used
  // immediately, before prepareResult completes.
  //
  // Maybe in the future it would be worth storing this future in a member
  // variable so our caller could extract if if they want to.  (It would allow
  // the caller to schedule additional work once the thrift server is fully up
  // and running, if the caller starts the thrift server.)
  (void)prepareResult;
}

TestServer::~TestServer() {}

AbsolutePath TestServer::getTmpDir() const {
  return AbsolutePath{tmpDir_.path().string()};
}

unique_ptr<EdenServer> TestServer::createServer(AbsolutePathPiece tmpDir) {
  auto edenDir = tmpDir + "eden"_pc;
  ensureDirectoryExists(edenDir);

  // Always use an in-memory local store during tests.
  // TODO: in the future we should build a better mechanism for controlling this
  // rather than having to update a command line flag.
  GFLAGS_NAMESPACE::SetCommandLineOptionWithMode(
      "local_storage_engine_unsafe",
      "memory",
      GFLAGS_NAMESPACE::SET_FLAG_IF_DEFAULT);

  auto userInfo = UserInfo::lookup();
  userInfo.setHomeDirectory(tmpDir + "home"_pc);
  auto config = make_shared<EdenConfig>(
      userInfo.getUsername(),
      userInfo.getUid(),
      userInfo.getHomeDirectory(),
      userInfo.getHomeDirectory() + ".edenrc"_pc,
      tmpDir + "etc"_pc,
      tmpDir + "etc/edenfs.rc"_relpath);
  auto privHelper = make_unique<FakePrivHelper>();
  config->setEdenDir(edenDir, ConfigSource::CommandLine);

  return make_unique<EdenServer>(userInfo, std::move(privHelper), config);
}

} // namespace eden
} // namespace facebook
