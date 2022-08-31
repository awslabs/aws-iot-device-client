// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "LimitedStreamBuffer.h"

#include <sstream>

using namespace std;
using namespace Aws::Iot::DeviceClient::Jobs;

void LimitedStreamBuffer::addString(const string &value)
{
    unique_lock<mutex> addLock(bufferLock);

    if (value.size() > contentsSizeLimit)
    {
        // Can't add a value that is greater than the buffer size
        buffer.clear();
        buffer.push_back(value.substr(value.size() - contentsSizeLimit, value.size()));
        contentsSize = contentsSizeLimit;
        return;
    }

    while (contentsSize + value.size() > contentsSizeLimit)
    {
        size_t frontSize = buffer.front().size();
        buffer.pop_front();
        contentsSize = contentsSize - frontSize;
    }

    buffer.push_back(value);
    contentsSize += value.size();
}

string LimitedStreamBuffer::toString()
{
    unique_lock<mutex> toStringLock(bufferLock);
    ostringstream output;
    for (string s : buffer)
    {
        output << s;
    }

    return output.str();
}
