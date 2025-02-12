/*
 *  Copyright (c) 2016-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "eden/fs/tracing/EdenStats.h"

#include <folly/container/Array.h>
#include <chrono>
#include <memory>

#include "eden/fs/eden-config.h"

using namespace folly;
using namespace std::chrono;

namespace {
constexpr std::chrono::microseconds kMinValue{0};
constexpr std::chrono::microseconds kMaxValue{10000};
constexpr std::chrono::microseconds kBucketSize{1000};
} // namespace

namespace facebook {
namespace eden {

FuseThreadStats& EdenStats::getFuseStatsForCurrentThread() {
  return *threadLocalFuseStats_.get();
}

HgBackingStoreThreadStats& EdenStats::getHgBackingStoreStatsForCurrentThread() {
  return *threadLocalHgBackingStoreStats_.get();
}

HgImporterThreadStats& EdenStats::getHgImporterStatsForCurrentThread() {
  return *threadLocalHgImporterStats_.get();
}

void EdenStats::aggregate() {
  for (auto& stats : threadLocalFuseStats_.accessAllThreads()) {
    stats.aggregate();
  }
  for (auto& stats : threadLocalHgBackingStoreStats_.accessAllThreads()) {
    stats.aggregate();
  }
  for (auto& stats : threadLocalHgImporterStats_.accessAllThreads()) {
    stats.aggregate();
  }
}

std::shared_ptr<HgImporterThreadStats> getSharedHgImporterStatsForCurrentThread(
    std::shared_ptr<EdenStats> stats) {
  return std::shared_ptr<HgImporterThreadStats>(
      stats, &stats->getHgImporterStatsForCurrentThread());
}

EdenThreadStatsBase::EdenThreadStatsBase() {}

EdenThreadStatsBase::Histogram EdenThreadStatsBase::createHistogram(
    const std::string& name) {
  return Histogram{this,
                   name,
                   static_cast<size_t>(kBucketSize.count()),
                   kMinValue.count(),
                   kMaxValue.count(),
                   facebook::stats::COUNT,
                   50,
                   90,
                   99};
}

#if defined(EDEN_HAVE_STATS)
EdenThreadStatsBase::Timeseries EdenThreadStatsBase::createTimeseries(
    const std::string& name) {
  auto timeseries = Timeseries{this, name};
  timeseries.exportStat(facebook::stats::COUNT);
  return timeseries;
}
#endif

void FuseThreadStats::recordLatency(
    HistogramPtr item,
    std::chrono::microseconds elapsed,
    std::chrono::seconds now) {
  (void)now; // we don't use it in this code path
  (this->*item).addValue(elapsed.count());
}

} // namespace eden
} // namespace facebook
