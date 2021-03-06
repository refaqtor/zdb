/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2017 Paul Asmuth <paul@asmuth.com>
 *
 * FnordTable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "unittest.h"
#include "fnordmetric/aggregation_service.h"
#include "fnordmetric/util/time.h"

using namespace fnordmetric;

UNIT_TEST(AggregationMapTest);

static AggregationSlot* getSlotOrDie(
    AggregationMap* map,
    uint64_t timestamp,
    uint64_t expire_at,
    std::shared_ptr<TableConfig> table,
    const std::vector<std::string>& labels) {
  AggregationSlot* slot;
  auto rc = map->getSlot(timestamp, expire_at, table, labels, &slot);
  if (!rc.isSuccess()) {
    throw std::runtime_error(rc.getMessage());
  }

  return slot;
}

TEST_CASE(AggregationMapTest, TestAggregationSlotLookup, [] () {
  AggregationMap map;

  auto ts = WallClock::unixMicros();
  auto test_tbl = std::make_shared<TableConfig>("test_tbl");
  test_tbl->interval = 10 * kMicrosPerSecond;
  test_tbl->labels.emplace_back(LabelConfig("test"));

  auto slot_a = getSlotOrDie(
      &map,
      ts,
      ts + 10 * kMicrosPerSecond,
      test_tbl,
      { "blah" });

  EXPECT(slot_a != nullptr);

  auto slot_b = getSlotOrDie(
      &map,
      ts + 10 * kMicrosPerSecond,
      ts + 20 * kMicrosPerSecond,
      test_tbl,
      { "blah" });

  EXPECT(slot_b != nullptr);
  EXPECT(slot_b != slot_a);

  auto slot_c = getSlotOrDie(
      &map,
      ts,
      ts + 10 * kMicrosPerSecond,
      test_tbl,
      { "blah" });

  EXPECT(slot_c != nullptr);
  EXPECT(slot_c != slot_b);
  EXPECT(slot_c == slot_a);

  auto slot_d = getSlotOrDie(
      &map,
      ts,
      ts + 10 * kMicrosPerSecond,
      test_tbl,
      { "blahx" });

  EXPECT(slot_d != nullptr);
  EXPECT(slot_d != slot_a);
  EXPECT(slot_d != slot_b);

  auto slot_e = getSlotOrDie(
      &map,
      ts,
      ts + 10 * kMicrosPerSecond,
      test_tbl,
      {});

  EXPECT(slot_e != nullptr);
  EXPECT(slot_e != slot_a);
  EXPECT(slot_e != slot_b);
  EXPECT(slot_e != slot_d);

  auto slot_f = getSlotOrDie(
      &map,
      ts + 10 * kMicrosPerSecond,
      ts + 20 * kMicrosPerSecond,
      test_tbl,
      {});

  EXPECT(slot_f != nullptr);
  EXPECT(slot_f != slot_a);
  EXPECT(slot_f != slot_b);
  EXPECT(slot_f != slot_d);
  EXPECT(slot_f != slot_e);

  auto slot_g = getSlotOrDie(
      &map,
      ts,
      ts + 10 * kMicrosPerSecond,
      test_tbl,
      {});

  EXPECT(slot_g != nullptr);
  EXPECT(slot_g != slot_a);
  EXPECT(slot_g != slot_b);
  EXPECT(slot_g != slot_d);
  EXPECT(slot_g == slot_e);
  EXPECT(slot_g != slot_f);
});

TEST_CASE(AggregationMapTest, TestExpiration, [] () {
  AggregationMap map;

  auto ts = WallClock::unixMicros();
  auto test_tbl = std::make_shared<TableConfig>("test_tbl");
  test_tbl->interval = 10 * kMicrosPerSecond;
  test_tbl->labels.emplace_back(LabelConfig("slot"));

  getSlotOrDie(
      &map,
      ts + 30 * kMicrosPerSecond,
      ts + 40 * kMicrosPerSecond,
      test_tbl,
      { "4" });

  getSlotOrDie(
      &map,
      ts + 40 * kMicrosPerSecond,
      ts + 50 * kMicrosPerSecond,
      test_tbl,
      { "5" });

  getSlotOrDie(
      &map,
      ts + 50 * kMicrosPerSecond,
      ts + 60 * kMicrosPerSecond,
      test_tbl,
      { "6" });

  getSlotOrDie(
      &map,
      ts + 20 * kMicrosPerSecond,
      ts + 30 * kMicrosPerSecond,
      test_tbl,
      { "3" });

  getSlotOrDie(
      &map,
      ts,
      ts + 10 * kMicrosPerSecond,
      test_tbl,
      { "1" });

  getSlotOrDie(
      &map,
      ts + 10 * kMicrosPerSecond,
      ts + 20 * kMicrosPerSecond,
      test_tbl,
      { "2" });

  std::vector<std::unique_ptr<AggregationSlot>> expired;
  map.getExpiredSlots(ts, &expired);
  EXPECT(expired.size() == 0);

  map.getExpiredSlots(ts + 10 * kMicrosPerSecond, &expired);
  EXPECT(expired.size() == 1);
  EXPECT(expired[0]->labels.size() == 1);
  EXPECT(expired[0]->labels[0] == "1");
  expired.clear();

  map.getExpiredSlots(ts + 11 * kMicrosPerSecond, &expired);
  EXPECT(expired.size() == 0);

  map.getExpiredSlots(ts + 35 * kMicrosPerSecond, &expired);
  EXPECT(expired[0]->labels.size() == 1);
  EXPECT(expired[0]->labels[0] == "2");
  EXPECT(expired[1]->labels.size() == 1);
  EXPECT(expired[1]->labels[0] == "3");
  EXPECT(expired.size() == 2);
  expired.clear();

  map.getExpiredSlots(ts + 60 * kMicrosPerSecond, &expired);
  EXPECT(expired[0]->labels.size() == 1);
  EXPECT(expired[0]->labels[0] == "4");
  EXPECT(expired[1]->labels.size() == 1);
  EXPECT(expired[1]->labels[0] == "5");
  EXPECT(expired[2]->labels.size() == 1);
  EXPECT(expired[2]->labels[0]== "6");
  EXPECT(expired.size() == 3);
  expired.clear();

  map.getExpiredSlots(ts + 60 * kMicrosPerSecond, &expired);
  EXPECT(expired.size() == 0);

  getSlotOrDie(
      &map,
      ts + 30 * kMicrosPerSecond,
      ts + 40 * kMicrosPerSecond,
      test_tbl,
      { "4" });

  getSlotOrDie(
      &map,
      ts + 40 * kMicrosPerSecond,
      ts + 50 * kMicrosPerSecond,
      test_tbl,
      { "5" });

  getSlotOrDie(
      &map,
      ts + 50 * kMicrosPerSecond,
      ts + 60 * kMicrosPerSecond,
      test_tbl,
      { "6" });

  getSlotOrDie(
      &map,
      ts + 20 * kMicrosPerSecond,
      ts + 30 * kMicrosPerSecond,
      test_tbl,
      { "3" });

  getSlotOrDie(
      &map,
      ts,
      ts + 10 * kMicrosPerSecond,
      test_tbl,
      { "1" });

  getSlotOrDie(
      &map,
      ts + 10 * kMicrosPerSecond,
      ts + 20 * kMicrosPerSecond,
      test_tbl,
      { "2" });

  map.getExpiredSlots(ts, &expired);
  EXPECT(expired.size() == 0);

  map.getExpiredSlots(ts + 10 * kMicrosPerSecond, &expired);
  EXPECT(expired.size() == 1);
  EXPECT(expired[0]->labels.size() == 1);
  EXPECT(expired[0]->labels[0] == "1");
  expired.clear();

  map.getExpiredSlots(ts + 11 * kMicrosPerSecond, &expired);
  EXPECT(expired.size() == 0);

  map.getExpiredSlots(ts + 35 * kMicrosPerSecond, &expired);
  EXPECT(expired[0]->labels.size() == 1);
  EXPECT(expired[0]->labels[0] == "2");
  EXPECT(expired[1]->labels.size() == 1);
  EXPECT(expired[1]->labels[0] == "3");
  EXPECT(expired.size() == 2);
  expired.clear();

  map.getExpiredSlots(ts + 60 * kMicrosPerSecond, &expired);
  EXPECT(expired[0]->labels.size() == 1);
  EXPECT(expired[0]->labels[0] == "4");
  EXPECT(expired[1]->labels.size() == 1);
  EXPECT(expired[1]->labels[0] == "5");
  EXPECT(expired[2]->labels.size() == 1);
  EXPECT(expired[2]->labels[0] == "6");
  EXPECT(expired.size() == 3);
});

