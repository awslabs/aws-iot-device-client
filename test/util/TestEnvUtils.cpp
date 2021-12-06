// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../../source/util/EnvUtils.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <cerrno>
#include <iterator>
#include <memory>
#include <string>

using namespace Aws::Iot::DeviceClient::Util;

static const char DEFAULT_PATH[] = "/usr/bin:/usr/local/bin";

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
        return setenv_retval;
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

    std::string getenv_retval{"/usr/bin:/usr/local/bin"};
    std::string setenv_name{"PATH"};
    std::string setenv_value;
    int setenv_retval{0};
    std::string getcwd_retval{"/tmp"};
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
    envUtils.getOS()->getcwd_retval = "/tmp";

    ASSERT_EQ(0, envUtils.AppendCwdToPath());
    ASSERT_EQ("/usr/bin:/usr/local/bin:/tmp:/tmp/jobs", envUtils.getOS()->setenv_value);
}

TEST(EnvUtils, handleUnsetPath)
{
    FakeEnvUtils envUtils(new FakeOSInterface);
    envUtils.getOS()->getenv_retval.clear();
    envUtils.getOS()->getcwd_retval = "/tmp";

    ASSERT_EQ(0, envUtils.AppendCwdToPath());
    ASSERT_EQ("/tmp:/tmp/jobs", envUtils.getOS()->setenv_value);
}

TEST(EnvUtils, handleGetcwdError)
{
    FakeEnvUtils envUtils(new FakeOSInterface);
    envUtils.getOS()->getcwd_retval.clear();
    envUtils.getOS()->getcwd_errno = 1;

    ASSERT_EQ(1, envUtils.AppendCwdToPath());
}

TEST(EnvUtils, handleSetenvError)
{
    FakeEnvUtils envUtils(new FakeOSInterface);
    envUtils.getOS()->setenv_retval = 1;

    ASSERT_EQ(1, envUtils.AppendCwdToPath());
}
