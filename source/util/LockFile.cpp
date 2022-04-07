// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include "LockFile.h"

using namespace Aws::Iot::DeviceClient::Util;

LockFile::LockFile(const std::string& filename)
    : filename(filename)
      , file(fopen(filename.c_str(), "wx"))
{
    if (!file)
    {
        throw std::runtime_error{"unable to open lockfile"};
    }
    flockfile(file);
}

LockFile::~LockFile()
{
    funlockfile(file);
    fclose(file);
    remove(filename.c_str());
}