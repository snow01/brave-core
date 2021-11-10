/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_SAMPLE_FEDERATED_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_SAMPLE_FEDERATED_TASK_H_

#include <map>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "sql/database.h"
#include "sql/meta_table.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace brave {

class SampleFederatedDataStore {
 public:
    // Log Definition --------------------------------------------------------
    struct SampleFederatedLog {
        SampleFederatedLog(const std::string& id,
                           const std::string& sample_text,
                           const int sample_number,
                           const std::string& creation_date);
        SampleFederatedLog();
        SampleFederatedLog(const SampleFederatedLog& other);
        ~SampleFederatedLog();

        std::string id;
        std::string sample_text;
        int sample_number;
        std::string creation_date;
    };

    typedef std::vector<std::string> SampleIDs;
    typedef std::map<std::string, SampleFederatedLog> GuidToSampleMap;

    explicit SampleFederatedDataStore(const base::FilePath& database_path);

    SampleFederatedDataStore(const SampleFederatedDataStore&) = delete;
    SampleFederatedDataStore& operator=(const SampleFederatedDataStore&) = delete;

    bool Init(const std::string& task_id, 
              const std::string& task_name); // TODO: add retention policy 

    bool AddSampleLog(const SampleFederatedLog& sample_log);

    void LoadSampleLogs(GuidToSampleMap* sample_logs);

    bool DeleteAllSampleLogs();

 private:
    bool EnsureTable();  
    //bool CheckDataConformity();

    virtual ~SampleFederatedDataStore();

    sql::Database db_;
    base::FilePath database_path_;

    std::string task_id_;
    std::string task_name_;
    sql::MetaTable meta_table_;
};

} // namespace brave

#endif // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_SAMPLE_FEDERATED_TASK_H_

 