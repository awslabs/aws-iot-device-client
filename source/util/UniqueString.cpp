// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "UniqueString.h"

#include <iostream>
#include <random>
#include <sstream>

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;

string UniqueString::GetRandomToken(size_t len)
{
    size_t length = len > UniqueString::MAX_CLIENT_TOKEN_SIZE ? UniqueString::MAX_CLIENT_TOKEN_SIZE : len;
    static auto &chrs = "0123456789"
                        "abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    ostringstream stream;
    while (length--)
    {
        stream << chrs[pick(rg)];
    }

    return stream.str();
}