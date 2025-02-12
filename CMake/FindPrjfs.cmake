# Copyright (c) 2016-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

# Find PrjFS
#
# This package sets:
# Prjfs_FOUND - Whether PrjFS was found
# PRJFS_INCLUDE_DIR - The include directory for Prjfs
# PRJFS_LIBRARY - The Prjfs library

include(FindPackageHandleStandardArgs)

find_path(PRJFS_INCLUDE_DIR NAMES ProjectedFSLib.h PATHS "D:/edenwin64/prjfs")
find_library(PRJFS_LIBRARY NAMES ProjectedFSLib.lib PATHS "D:/edenwin64/prjfs")
find_package_handle_standard_args(
  Prjfs
  PRJFS_INCLUDE_DIR
  PRJFS_LIBRARY
)

if(Prjfs_FOUND)
  add_library(ProjectedFS INTERFACE)
  target_include_directories(ProjectedFS INTERFACE "${PRJFS_INCLUDE_DIR}")
  target_link_libraries(ProjectedFS INTERFACE "${PRJFS_LIBRARY}")
endif()
