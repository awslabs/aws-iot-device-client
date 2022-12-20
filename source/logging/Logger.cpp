// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace Aws::Iot::DeviceClient;
using namespace std;
using namespace std::chrono;

constexpr char TIMESTAMP_FORMAT[] =
    "%Y-%m-%dT%H:%M:%S."; // ISO 8601 "2011-10-08T07:07:09.178Z", ms will be calculated last

void LogUtil::generateTimestamp(
    std::chrono::time_point<std::chrono::system_clock> t,
    size_t bufferSize,
    char *timeBuffer)
{
    auto ms = duration_cast<milliseconds>(t.time_since_epoch()) % 1000;
    auto timer = system_clock::to_time_t(t);
    struct tm buf;
    std::tm bt = *gmtime_r(&timer, &buf);

    std::ostringstream time_stream;
    time_stream << std::put_time(&bt, TIMESTAMP_FORMAT);
    time_stream << std::setfill('0') << std::setw(3) << ms.count();
    time_stream << "Z";

    const string timestamp = time_stream.str();
    timestamp.copy(timeBuffer, bufferSize);
    timeBuffer[bufferSize - 1] = '\0';
}
