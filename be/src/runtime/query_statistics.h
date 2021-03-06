// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef DORIS_BE_EXEC_QUERY_STATISTICS_H
#define DORIS_BE_EXEC_QUERY_STATISTICS_H

#include <mutex>

#include "gen_cpp/data.pb.h"
#include "util/spinlock.h"

namespace doris {

class QueryStatisticsRecvr;
 
// This is responsible for collecting query statistics, usually it consists of 
// two parts, one is current fragment or plan's statistics, the other is sub fragment
// or plan's statistics and QueryStatisticsRecvr is responsible for collecting it.
class QueryStatistics {
public:

    QueryStatistics() : scan_rows(0), scan_bytes(0) {
    }

    void merge(const QueryStatistics& other) {
        scan_rows += other.scan_rows;
        scan_bytes += other.scan_bytes;
    }

    void add_scan_rows(int64_t scan_rows) {
        this->scan_rows += scan_rows;
    }

    void add_scan_bytes(int64_t scan_bytes) {
        this->scan_bytes += scan_bytes;
    }

    void merge(QueryStatisticsRecvr* recvr);

    void clear() {
        scan_rows = 0;
        scan_bytes = 0;
    }

    void to_pb(PQueryStatistics* statistics) {
        DCHECK(statistics != nullptr);
        statistics->set_scan_rows(scan_rows);
        statistics->set_scan_bytes(scan_bytes);
    }

    void merge_pb(const PQueryStatistics& statistics) {
        scan_rows += statistics.scan_rows();
        scan_bytes += statistics.scan_bytes();
    }

private:

    int64_t scan_rows;
    int64_t scan_bytes;
};

// It is used for collecting sub plan query statistics in DataStreamRecvr.
class QueryStatisticsRecvr {
public:

    ~QueryStatisticsRecvr();

    void insert(const PQueryStatistics& statistics, int sender_id);

private:

friend class QueryStatistics;

    void merge(QueryStatistics* statistics) {
        std::lock_guard<SpinLock> l(_lock);
        for (auto& pair : _query_statistics) {
            statistics->merge(*(pair.second));
        }
    }
 
    std::map<int, QueryStatistics*> _query_statistics;
    SpinLock _lock;
};

}

#endif
