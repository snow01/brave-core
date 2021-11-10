/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated_learning/tasks/sample_federated_task.h"

#include <string>

#include "base/bind.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "sql/meta_table.h"
#include "sql/recovery.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "ui/base/page_transition_types.h"

//DEBUG
#include <iostream>

namespace {

void BindSampleLogToStatement(const brave::SampleFederatedDataStore::SampleFederatedLog& sample_log,
                             sql::Statement* s) {
  //DCHECK(base::IsValidGUID(sample_log.id));
  s->BindString(0, sample_log.id);
  s->BindString(1, sample_log.sample_text);
  s->BindInt(2, sample_log.sample_number);
  s->BindString(3, sample_log.creation_date);
}

void DatabaseErrorCallback(sql::Database* db,
                           const base::FilePath& db_path,
                           int extended_error,
                           sql::Statement* stmt) {
  if (sql::Recovery::ShouldRecover(extended_error)) {
    // Prevent reentrant calls.
    db->reset_error_callback();

    // After this call, the |db| handle is poisoned so that future calls will
    // return errors until the handle is re-opened.
    sql::Recovery::RecoverDatabase(db, db_path);

    // The DLOG(FATAL) below is intended to draw immediate attention to errors
    // in newly-written code.  Database corruption is generally a result of OS
    // or hardware issues, not coding errors at the client level, so displaying
    // the error would probably lead to confusion.  The ignored call signals the
    // test-expectation framework that the error was handled.
    ignore_result(sql::Database::IsExpectedSqliteError(extended_error));
    return;
  }

  // The default handling is to assert on debug and to ignore on release.
  if (!sql::Database::IsExpectedSqliteError(extended_error))
    DLOG(FATAL) << db->GetErrorMessage();
}

}

namespace brave {

// SampleFederatedDataStore::SampleFederatedLog ---------------------------------------

SampleFederatedDataStore::SampleFederatedLog::SampleFederatedLog(const std::string& id,
                                                                 const std::string& sample_text,
                                                                 const int sample_number,
                                                                 const std::string& creation_date) 
    : id(id), 
      sample_text(sample_text), 
      sample_number(sample_number),
      creation_date(creation_date) {}

SampleFederatedDataStore::SampleFederatedLog::SampleFederatedLog(const SampleFederatedLog& other) = default;

SampleFederatedDataStore::SampleFederatedLog::~SampleFederatedLog() {}



// SampleFederatedDataStore -----------------------------------------------------

SampleFederatedDataStore::SampleFederatedDataStore(const base::FilePath& database_path)
    : db_({
        .exclusive_locking = true,
        .page_size = 4096,
        .cache_size = 500}),
    database_path_(database_path) {}

bool SampleFederatedDataStore::Init(const std::string& task_id, 
                                    const std::string& task_name) {
    task_id_ = task_id;
    task_name_ = task_name;

    db_.set_histogram_tag("SampleLog");

    // To recover from corruption.
    db_.set_error_callback(
        base::BindRepeating(&DatabaseErrorCallback, &db_, database_path_));

    // Attach the database to our index file.
    return db_.Open(database_path_) && EnsureTable();
}

bool SampleFederatedDataStore::AddSampleLog(const SampleFederatedLog& sample_log) {
    sql::Statement s(db_.GetUniqueStatement(
      base::StringPrintf("INSERT INTO %s (id, sample_text, sample_number, creation_date) "
      "VALUES (?,?,?,?)", task_name_.c_str()).c_str()));
    BindSampleLogToStatement(sample_log, &s);
    return s.Run();
}

bool SampleFederatedDataStore::DeleteAllSampleLogs() {
    if (!db_.Execute(
        base::StringPrintf("DELETE FROM %s", task_name_.c_str()).c_str()))
    return false;

    ignore_result(db_.Execute("VACUUM"));
    return true;
}

void SampleFederatedDataStore::LoadSampleLogs(GuidToSampleMap* sample_logs) {
    DCHECK(sample_logs);
    sql::Statement s(db_.GetUniqueStatement(
      base::StringPrintf("SELECT id, sample_text, sample_number, creation_date FROM %s",
                         task_name_.c_str()).c_str()));

    sample_logs->clear();
    while (s.Step()) {
        sample_logs->insert(std::make_pair(
            s.ColumnString(0),
            SampleFederatedLog(
                s.ColumnString(0),
                s.ColumnString(1),
                s.ColumnInt(2),
                s.ColumnString(3)
            )
        ));
    }
}

bool SampleFederatedDataStore::EnsureTable() {
    if (db_.DoesTableExist(task_name_)) return true;

    std::cerr << "Creating new table" << std::endl;
    sql::Transaction transaction(&db_);

    // meta_table_.Init(&db_, kCurrentVersionNumber, kCompatibleVersionNumber)
    return transaction.Begin() &&
            db_.Execute(
                base::StringPrintf("CREATE TABLE %s (id TEXT PRIMARY KEY, "
                "sample_text TEXT, sample_number INTEGER, "
                "creation_date TEXT)", task_name_.c_str()).c_str()) &&
            transaction.Commit();
}

SampleFederatedDataStore::~SampleFederatedDataStore() {
}   

} // namespace brave



