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

#include <folly/ThreadLocal.h>
#include <memory>
#include "common/stats/ThreadLocalStats.h"
#include "eden/fs/eden-config.h"

namespace facebook {
namespace eden {

class FuseThreadStats;
class HgBackingStoreThreadStats;
class HgImporterThreadStats;

class EdenStats {
 public:
  /**
   * This function can be called on any thread.
   *
   * The returned object can be used only on the current thread.
   */
  FuseThreadStats& getFuseStatsForCurrentThread();

  /**
   * This function can be called on any thread.
   *
   * The returned object can be used only on the current thread.
   */
  HgBackingStoreThreadStats& getHgBackingStoreStatsForCurrentThread();

  /**
   * This function can be called on any thread.
   *
   * The returned object can be used only on the current thread.
   */
  HgImporterThreadStats& getHgImporterStatsForCurrentThread();

  /**
   * This function can be called on any thread.
   */
  void aggregate();

 private:
  class ThreadLocalTag {};

  folly::ThreadLocal<FuseThreadStats, ThreadLocalTag, void>
      threadLocalFuseStats_;
  folly::ThreadLocal<HgBackingStoreThreadStats, ThreadLocalTag, void>
      threadLocalHgBackingStoreStats_;
  folly::ThreadLocal<HgImporterThreadStats, ThreadLocalTag, void>
      threadLocalHgImporterStats_;
};

std::shared_ptr<HgImporterThreadStats> getSharedHgImporterStatsForCurrentThread(
    std::shared_ptr<EdenStats>);

/**
 * EdenThreadStatsBase is a base class for a group of thread-local stats
 * structures.
 *
 * Each EdenThreadStatsBase object should only be used from a single thread. The
 * EdenStats object should be used to maintain one EdenThreadStatsBase object
 * for each thread that needs to access/update the stats.
 */
class EdenThreadStatsBase : public facebook::stats::ThreadLocalStatsT<
                                facebook::stats::TLStatsThreadSafe> {
 public:
  using Histogram = TLHistogram;
#if defined(EDEN_HAVE_STATS)
  using Timeseries = TLTimeseries;
#endif

  explicit EdenThreadStatsBase();

 protected:
  Histogram createHistogram(const std::string& name);
#if defined(EDEN_HAVE_STATS)
  Timeseries createTimeseries(const std::string& name);
#endif
};

class FuseThreadStats : public EdenThreadStatsBase {
 public:
  // We track latency in units of microseconds, hence the _us suffix
  // in the histogram names below.

  Histogram lookup{createHistogram("fuse.lookup_us")};
  Histogram forget{createHistogram("fuse.forget_us")};
  Histogram getattr{createHistogram("fuse.getattr_us")};
  Histogram setattr{createHistogram("fuse.setattr_us")};
  Histogram readlink{createHistogram("fuse.readlink_us")};
  Histogram mknod{createHistogram("fuse.mknod_us")};
  Histogram mkdir{createHistogram("fuse.mkdir_us")};
  Histogram unlink{createHistogram("fuse.unlink_us")};
  Histogram rmdir{createHistogram("fuse.rmdir_us")};
  Histogram symlink{createHistogram("fuse.symlink_us")};
  Histogram rename{createHistogram("fuse.rename_us")};
  Histogram link{createHistogram("fuse.link_us")};
  Histogram open{createHistogram("fuse.open_us")};
  Histogram read{createHistogram("fuse.read_us")};
  Histogram write{createHistogram("fuse.write_us")};
  Histogram flush{createHistogram("fuse.flush_us")};
  Histogram release{createHistogram("fuse.release_us")};
  Histogram fsync{createHistogram("fuse.fsync_us")};
  Histogram opendir{createHistogram("fuse.opendir_us")};
  Histogram readdir{createHistogram("fuse.readdir_us")};
  Histogram releasedir{createHistogram("fuse.releasedir_us")};
  Histogram fsyncdir{createHistogram("fuse.fsyncdir_us")};
  Histogram statfs{createHistogram("fuse.statfs_us")};
  Histogram setxattr{createHistogram("fuse.setxattr_us")};
  Histogram getxattr{createHistogram("fuse.getxattr_us")};
  Histogram listxattr{createHistogram("fuse.listxattr_us")};
  Histogram removexattr{createHistogram("fuse.removexattr_us")};
  Histogram access{createHistogram("fuse.access_us")};
  Histogram create{createHistogram("fuse.create_us")};
  Histogram bmap{createHistogram("fuse.bmap_us")};
  Histogram ioctl{createHistogram("fuse.ioctl_us")};
  Histogram poll{createHistogram("fuse.poll_us")};
  Histogram forgetmulti{createHistogram("fuse.forgetmulti_us")};

  // Since we can potentially finish a request in a different
  // thread from the one used to initiate it, we use HistogramPtr
  // as a helper for referencing the pointer-to-member that we
  // want to update at the end of the request.
  using HistogramPtr = Histogram FuseThreadStats::*;

  /** Record a the latency for an operation.
   * item is the pointer-to-member for one of the histograms defined
   * above.
   * elapsed is the duration of the operation, measured in microseconds.
   * now is the current steady clock value in seconds.
   * (Once we open source the common stats code we can eliminate the
   * now parameter from this method). */
  void recordLatency(
      HistogramPtr item,
      std::chrono::microseconds elapsed,
      std::chrono::seconds now);
};

/**
 * @see HgBackingStore
 */
class HgBackingStoreThreadStats : public EdenThreadStatsBase {
 public:
  Histogram hgBackingStoreGetTree{createHistogram("store.hg.get_tree")};
  Histogram mononokeBackingStoreGetTree{
      createHistogram("store.mononoke.get_tree")};
  Histogram mononokeBackingStoreGetBlob{
      createHistogram("store.mononoke.get_blob")};
};

/**
 * @see HgImporter
 * @see HgBackingStore
 */
class HgImporterThreadStats : public EdenThreadStatsBase {
 public:
  Histogram hgBackingStoreGetBlob{createHistogram("store.hg.get_file")};

#if defined(EDEN_HAVE_STATS)
  Timeseries catFile{createTimeseries("hg_importer.cat_file")};
  Timeseries fetchTree{createTimeseries("hg_importer.fetch_tree")};
  Timeseries manifest{createTimeseries("hg_importer.manifest")};
  Timeseries manifestNodeForCommit{
      createTimeseries("hg_importer.manifest_node_for_commit")};
  Timeseries prefetchFiles{createTimeseries("hg_importer.prefetch_files")};
#endif
};

} // namespace eden
} // namespace facebook
