// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_ENVUTILS_H
#define AWS_IOT_DEVICE_CLIENT_ENVUTILS_H

#include <memory>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Util
            {
                /**
                 * \brief OSInterface presents an interface to the operating system.
                 */
                struct OSInterface
                {
                    virtual ~OSInterface() = default;
                    virtual char *getenv(const char *name) = 0;
                    virtual int setenv(const char *name, const char *value, int overwrite) = 0;
                    virtual char *getcwd(char *buf, size_t size) = 0;
                };

                /**
                 * \brief OSInterfacePosix is an operating system interface using posix system calls.
                 */
                struct OSInterfacePosix : public OSInterface
                {
                    /**
                     * \brief Get value of an environment variable.
                     * @param name Environment variable name
                     * @return Pointer to environment variable value or nullptr
                     */
                    char *getenv(const char *name) override;

                    /**
                     * \brief Add or change environment variable.
                     * @param name Environment variable name
                     * @param value Environment variable value
                     * @param overwrite Replace environment variable value when non-zero
                     * @return 0 on success or non-zero on error
                     */
                    int setenv(const char *name, const char *value, int overwrite) override;

                    /**
                     * \brief Get the absolute pathname of the current working directory.
                     * @param buf Array of characters in which to store returned working directory
                     * @param size Size in bytes of the array pointed to by buf
                     * @return Pointer to buffer passed as input or nullptr if length of pathname exceeds size of buffer
                     */
                    char *getcwd(char *buf, size_t size) override;
                };

                /**
                 * \brief Utility functions for managing environment variables.
                 */
                struct EnvUtils
                {
                  private:
                    static constexpr char TAG[] = "EnvUtils.cpp";

                  public:
                    using OSInterfacePtr = std::unique_ptr<OSInterface>;

                    EnvUtils() : os(new OSInterfacePosix) {}

                    explicit EnvUtils(OSInterfacePtr os) : os(std::move(os)) {}

                    /**
                     * \brief Append the current working directory to PATH environment variable.
                     * @return 0 on success or non-zero on error
                     */
                    int AppendCwdToPath() const;

                  protected:
                    OSInterfacePtr os;
                };
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
#endif // AWS_IOT_DEVICE_CLIENT_ENVUTILS_H
