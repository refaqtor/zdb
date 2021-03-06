/**
 * Copyright (c) 2016, Paul Asmuth <paul@asmuth.com>
 * All rights reserved.
 * 
 * This file is part of the "libzdb" project. libzdb is free software licensed
 * under the 3-Clause BSD License (BSD-3-Clause).
 */
#include <assert.h>
#include "zdb.h"
#include "lock.h"
#include "database.h"

namespace zdb {

zdb_err_t table_add(
    database_ref db,
    const std::string& table_name) {
  assert(!!db);

  /* check that database is not readonly */
  if (db->readonly) {
    return ZDB_ERR_READONLY;
  }

  /* acquire write lock */
  lock_guard lk(&db->lock);
  lk.lock_write();

  /* check table name */
  if (table_name.empty()) {
    return ZDB_ERR_INVALID_ARGUMENT;
  }

  if (db->meta.tables.count(table_name)) {
    return ZDB_ERR_EXISTS;
  }

  /* add table */
  db->meta.dirty = true;
  db->meta.tables.emplace(table_name, table{});

  return ZDB_SUCCESS;
}

zdb_err_t column_add(
    database_ref db,
    const std::string& table_name,
    const std::string& column_name,
    zdb_type_t column_type,
    int* id /* = nullptr */) {
  assert(!!db);

  /* check that database is not readonly */
  if (db->readonly) {
    return ZDB_ERR_READONLY;
  }

  /* acquire write lock */
  lock_guard lk(&db->lock);
  lk.lock_write();

  /* find table */
  auto table_iter = db->meta.tables.find(table_name);
  if (table_iter == db->meta.tables.end()) {
    return ZDB_ERR_NOTFOUND;
  }

  auto& table = table_iter->second;

  /* check column name */
  for (const auto& c : table.columns) {
    if (c.name == column_name) {
      return ZDB_ERR_EXISTS;
    }
  }

  if (column_name.empty()) {
    return ZDB_ERR_INVALID_ARGUMENT;
  }

  /* create new column */
  column_info col;
  col.name = column_name;
  col.type = column_type;
  col.id = 0;

  /* find free column id */
  for (; col.id < table.columns.size(); ++col.id) {
    if (table.columns[col.id].id != col.id) {
      break;
    }
  }

  /* add column info to metadata */
  db->meta.dirty = true;
  table.columns.insert(table.columns.begin() + col.id, std::move(col));

  /* add columns to all row blocks */
  for (auto& row_block : table.row_map) {
    column_block col_block;
    col_block.present = false;
    col_block.page = nullptr;

    row_block.columns.insert(
        row_block.columns.begin() + col.id,
        std::move(col_block));
  }

  /* return id */
  if (id) {
    *id = col.id;
  }

  return ZDB_SUCCESS;
}

} // namespace zdb

