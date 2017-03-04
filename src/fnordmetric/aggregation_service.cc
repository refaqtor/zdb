/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *   Copyright (c) 2016 Paul Asmuth, FnordCorp B.V. <paul@asmuth.com>
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <iostream>
#include <sstream>
#include "fnordmetric/aggregation_service.h"
#include "fnordmetric/util/fileutil.h"
#include "fnordmetric/util/logging.h"
#include "fnordmetric/util/time.h"
#include "fnordmetric/util/sha1.h"
#include "fnordmetric/statsd.h"

namespace fnordmetric {

ReturnCode AggregationService::startService(
    std::unique_ptr<AggregationService>* service) {
  service->reset(new AggregationService());
  return ReturnCode::success();
}

AggregationService::AggregationService() :
    aggregation_map_(new AggregationMap()),
    expiration_thread_shutdown_(false),
    expiration_thread_(std::thread(
        std::bind(&AggregationService::processExpirations, this))) {}

AggregationService::~AggregationService() {
  shutdown();

  if (expiration_thread_.joinable()) {
    expiration_thread_.join();
  }
}

ReturnCode AggregationService::applyConfig(const ConfigList* config) {
  std::unique_lock<std::mutex> lk(table_map_mutex_);
  auto table_map = table_map_.getTableMap();
  TableMapBuilder table_map_builder(table_map.get());

  for (auto& mc : config->getTableConfigs()) {
    table_map_builder.addTable(mc.first, mc.second);
  }

  table_map_.updateTableMap(table_map_builder.getTableMap());
  return ReturnCode::success();
}

ReturnCode AggregationService::insertSample(const Sample& smpl) {
  auto table_id = smpl.getMetricName(); // FIXME
  auto table = table_map_.getTableMap()->findTable(table_id);
  if (!table.get()) {
    return ReturnCode::errorf("ERUNTIME", "table not found: $0", table_id);
  }

  std::vector<std::string> labels; // FIXME

  std::unique_lock<std::mutex> lk(mutex_);
  auto now = MonotonicClock::now();
  auto slot = aggregation_map_->getSlot(
      smpl.getTime(),
      now + table->interval,
      table,
      labels);

  cv_.notify_all();
  return ReturnCode::success();
}

AggregationService::BatchInsertOptions::BatchInsertOptions() :
    metric_id_rewrite_enabled(false) {}

ReturnCode AggregationService::insertSamplesBatch(
    const char* data,
    size_t len,
    const BatchInsertOptions* opts /* = nullptr */) {
  std::string metric_id;
  std::string series_id;
  std::string value;
  LabelSet labels;

  char const* cur = data;
  char const* end = data + len;

  while (cur < end) {
    if (!parseStatsdSample(&cur, end, &metric_id, &series_id, &value)) {
      return ReturnCode::error("EPARSE", "invalid packet");
    }

    if (opts && opts->metric_id_rewrite_enabled) {
      try {
        metric_id = std::regex_replace(
            metric_id,
            opts->metric_id_rewrite_regex,
            opts->metric_id_rewrite_replace,
            std::regex_constants::match_default |
            std::regex_constants::format_no_copy |
            std::regex_constants::format_first_only);

      } catch (const std::exception& e) {
        return ReturnCode::errorf("ERUNTIME", "regex error: $0", e.what());
      }
    }

    auto now = WallClock::unixMicros();
    auto rc = insertSample(Sample(metric_id, value, now, {}));

    if (!rc.isSuccess()) {
      logWarning(
          "batch insert failed: $0; metric_id=$1 value=$2",
          rc.getMessage(),
          metric_id,
          value);
    }
  }

  return ReturnCode::success();
}

void AggregationService::shutdown() {
  std::unique_lock<std::mutex> lk(mutex_);
  expiration_thread_shutdown_ = true;
  cv_.notify_all();
}

void AggregationService::processExpirations() {
  std::unique_lock<std::mutex> lk(mutex_);
  while (!expiration_thread_shutdown_) {
    auto now = MonotonicClock::now();
    std::vector<std::unique_ptr<AggregationSlot>> expired;
    aggregation_map_->getExpiredSlots(now, &expired);

    if (!expired.empty()) {
      performInserts(std::move(expired));
    }

    auto next = aggregation_map_->getNextExpiration();
    if (next == 0) {
      cv_.wait(lk);
    } else {
      assert(next > now);
      cv_.wait_for(lk, std::chrono::microseconds(next - now));
    }
  }
}

void AggregationService::performInserts(
    std::vector<std::unique_ptr<AggregationSlot>> slots) {
  logInfo("metric-collectd", "Insert $0 slots", slots.size());
}

AggregationSlot* AggregationMap::getSlot(
    uint64_t timestamp,
    uint64_t expire_at,
    std::shared_ptr<TableConfig> table,
    const std::vector<std::string>& labels) {
  std::string slot_idv;
  slot_idv += std::to_string(timestamp) + "~";
  slot_idv += std::to_string((intptr_t) table.get());
  for (const auto& l : labels) {
    slot_idv += "~" + l;
  }

  auto slot_id = SHA1::compute(slot_idv);
  auto slots = slots_.equal_range(slot_id);
  for (auto iter = slots.first; iter != slots.second; ++iter) {
    if (iter->second->time == timestamp &&
        iter->second->table.get() == table.get() &&
        iter->second->labels == labels) {
      return iter->second;
    }
  }

  auto slot = new AggregationSlot;
  slot->slot_id = slot_id;
  slot->time = timestamp;
  slot->table = table;
  slot->labels = labels;

  slots_.emplace(slot_id, slot);

  auto iter = expiration_list_.begin();
  for (; iter->first > expire_at && iter != expiration_list_.end(); ++iter);
  expiration_list_.insert(
      iter,
      std::make_pair(expire_at, std::unique_ptr<AggregationSlot>(slot)));

  return slot;
}

void AggregationMap::getExpiredSlots(
    uint64_t expired_on,
    std::vector<std::unique_ptr<AggregationSlot>>* slots) {
  while (!expiration_list_.empty() && expiration_list_.back().first <= expired_on) {
    auto slot = std::move(expiration_list_.back().second);
    expiration_list_.pop_back();

    auto iter_range = slots_.equal_range(slot->slot_id);
    for (auto iter = iter_range.first; iter != iter_range.second; ++iter) {
      if (iter->second == slot.get()) {
        slots_.erase(iter);
        break;
      }
    }

    slots->emplace_back(std::move(slot));
  }
}

uint64_t AggregationMap::getNextExpiration() const {
  if (expiration_list_.empty()) {
    return 0;
  }

  return expiration_list_.back().first;
}

} // namsepace fnordmetric

