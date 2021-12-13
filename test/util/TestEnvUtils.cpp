// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/config/Config.h"
#include "../../source/util/EnvUtils.h"
#include "../../source/util/FileUtils.h"
#include "../../source/util/StringUtils.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <cerrno>
#include <iterator>
#include <memory>
#include <string>

using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Util;

static const char PATH[] = "/usr/bin:/usr/local/bin";

static const char CWD[] = "/tmp";

static const std::string CONFIG_DIR = Config::ExpandDefaultConfigDir(true);

struct FakeOSInterface : public OSInterface
{
  public:
    char *getenv(const char *name) override
    {
        return getenv_retval.empty() ? nullptr : const_cast<char *>(getenv_retval.data());
    }

    int setenv(const char *name, const char *value, int overwrite) override
    {
        if (setenv_name == name)
        {
            setenv_value = value;
        }
        if (setenv_errno)
        {
            errno = setenv_errno;
        }
        return setenv_errno != 0 ? -1 : 0;
    }

    char *getcwd(char *buf, size_t size) override
    {
        if (!getcwd_retval.empty() && getcwd_retval.size() < size)
        {
            std::copy_n(std::begin(getcwd_retval), getcwd_retval.size(), buf);
            buf[getcwd_retval.size()] = '\0';
            return buf;
        }
        if (getcwd_errno)
        {
            errno = getcwd_errno;
        }
        return nullptr;
    }

    std::string getenv_retval{PATH};
    std::string setenv_name{"PATH"};
    std::string setenv_value;
    int setenv_errno{0};
    std::string getcwd_retval{CWD};
    int getcwd_errno{0};
};

struct FakeEnvUtils : public EnvUtils
{
  public:
    FakeEnvUtils(FakeOSInterface *os) : EnvUtils(OSInterfacePtr(os)) {}

    FakeOSInterface *getOS() { return dynamic_cast<FakeOSInterface *>(os.get()); }
};

TEST(EnvUtils, handleSetPath)
{
    FakeEnvUtils envUtils(new FakeOSInterface);

    // PATH is set in fixture, expect to append additional paths in AppendCwdToPath.
    std::ostringstream expected;
    expected << PATH << ':' << CONFIG_DIR << ':' << CONFIG_DIR << "/jobs" << ':' << CWD << ':' << CWD << "/jobs";

    ASSERT_EQ(0, envUtils.AppendCwdToPath());
    ASSERT_EQ(expected.str(), envUtils.getOS()->setenv_value);
}

TEST(EnvUtils, handleUnsetPath)
{
    FakeEnvUtils envUtils(new FakeOSInterface);
    envUtils.getOS()->getenv_retval.clear();

    // PATH is unset in fixture, expect the only paths are set in AppendCwdToPath.
    std::ostringstream expected;
    expected << CONFIG_DIR << ':' << CONFIG_DIR << "/jobs" << ':' << CWD << ':' << CWD << "/jobs";

    ASSERT_EQ(0, envUtils.AppendCwdToPath());
    ASSERT_EQ(expected.str(), envUtils.getOS()->setenv_value);
}

TEST(EnvUtils, handleGetcwdError)
{
    FakeEnvUtils envUtils(new FakeOSInterface);
    envUtils.getOS()->getcwd_retval.clear();
    envUtils.getOS()->getcwd_errno = EACCES;

    ASSERT_EQ(EACCES, envUtils.AppendCwdToPath());
}

TEST(EnvUtils, handleGetcwdExceedsMaxResize)
{
    FakeEnvUtils envUtils(new FakeOSInterface);
    envUtils.getOS()->getcwd_retval.clear();
    envUtils.getOS()->getcwd_errno = ERANGE;

    ASSERT_EQ(ENAMETOOLONG, envUtils.AppendCwdToPath());
}

TEST(EnvUtils, handleSetenvError)
{
    FakeEnvUtils envUtils(new FakeOSInterface);
    envUtils.getOS()->setenv_errno = ENOMEM;

    ASSERT_EQ(ENOMEM, envUtils.AppendCwdToPath());
}
