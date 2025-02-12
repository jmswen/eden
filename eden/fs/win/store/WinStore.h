/*
 *  Copyright (c) 2016-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once
#include <memory>
#include <string>
#include <vector>
#include "eden/fs/store/ObjectStore.h"
#include "eden/fs/utils/PathFuncs.h"
#include "folly/futures/Future.h"

namespace facebook {
namespace eden {

class ObjectStore;
class Tree;
class Blob;
class BlobMetadata;
class EdenMount;

struct FileMetadata {
  //
  // File name : final component
  //
  std::wstring name;

  //
  // isDirectory will be set only for the directories
  // For files it will be ignored
  //
  bool isDirectory{false};

  //
  // File size. For directories it will ignored
  //
  size_t size{0};

  FileMetadata(std::wstring name, bool isDir, size_t size)
      : name(name), isDirectory(isDir), size(size) {}

  FileMetadata() {}
};

class WinStore {
 public:
  WinStore(const EdenMount* mount);
  ~WinStore();

  //
  // getAllEntries() doesn't guarantee the order of the entries. Caller should
  // sort to get the desired order.
  //
  bool getAllEntries(
      const std::wstring& path,
      std::vector<FileMetadata>& entryList) const;
  bool getFileMetadata(const std::wstring& path, FileMetadata& fileMetadata)
      const;
  std::optional<const folly::IOBuf&> getFileContents(
      const std::wstring& path) const;

  std::shared_ptr<const Tree> getTree(const std::wstring& path) const;
  std::shared_ptr<const Blob> getBlob(const std::wstring& path) const;

 private:
  std::shared_ptr<const Tree> getTree(const RelativePathPiece& relPath) const;

  // Store a pointer to EdenMount. WinStore doesn't own or maintain the lifetime
  // of Mount. Instead, at this point, WinStore will be owned by the mount in
  // some direct or indirect way.
  const EdenMount* mount_;

  // Forbidden copy constructor and assignment operator
  WinStore(WinStore const&) = delete;
  WinStore& operator=(WinStore const&) = delete;
};
} // namespace eden
} // namespace facebook
