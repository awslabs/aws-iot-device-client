// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef AWS_IOT_DEVICE_CLIENT_FILEUTILS_H
#define AWS_IOT_DEVICE_CLIENT_FILEUTILS_H

#include <aws/common/byte_buf.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

namespace Aws
{
    namespace Iot
    {
        namespace DeviceClient
        {
            namespace Util
            {
                /**
                 * \brief Utility functions for operations related to files
                 */
                class FileUtils
                {
                  private:
                    static constexpr char TAG[] = "FileUtils.cpp";

                  public:
                    /**
                     * \brief Creates each of the directories in the provided path if they do not exist
                     * @param path the full path to assess
                     * @return 0 upon success, some other number indicating an error otherwise
                     */
                    static int Mkdirs(const std::string &path);

                    /**
                     * \brief Given a path to a file, attempts to extract the parent directory
                     * @param filePath a path to a file
                     * @return the parent directory of the file
                     */
                    static std::string ExtractParentDirectory(const std::string &filePath);

                    /**
                     * \brief Given a path to a file, attempts to extract the absolute path.
                     * @param filePath a path to a file
                     * @return the expanded path of the file.
                     * @throws wordexp_fail_error upon wordexp failure
                     */
                    static std::string ExtractExpandedPath(const std::string &filePath);

                    /**
                     * \brief Stores string value in given file
                     * @param value string value to be stored in given file
                     * @param filePath a path to a file
                     * @return true on success
                     */
                    static bool StoreValueInFile(const std::string &value, const std::string &filePath);

                    /**
                     * \brief Reads data of size into buffer
                     * @param pathToFile Path to a file
                     * @param data Buffer to contain data
                     * @param size Size of data to read
                     * @return 0 on success
                     */
                    static int ReadFromFile(const std::string &pathToFile, aws_byte_buf *data, size_t size);

                    /**
                     * \brief Write data into file in append mode
                     * @param pathToFile Path to a file
                     * @param data Buffer containing data
                     * @return 0 on success
                     */
                    static int WriteToFile(const std::string &pathToFile, const aws_byte_buf *data);

                    /**
                     * \brief Returns an integer representing the permissions of the specified file.
                     *
                     * @param filePath a path to a file
                     * @return an integer representing the file permissions.
                     */
                    static int GetFilePermissions(const std::string &path);

                    /**
                     * \brief Validates ownership permissions on the given file/dir
                     *
                     * @param path a path to a file/dir
                     * @return returns true if user have the ownership permissions to access given file/dir
                     */
                    static bool ValidateFileOwnershipPermissions(const std::string &path);

                    /**
                     * \brief Returns true if permissions set for given file/dir are correct
                     *
                     * @param path a path to a file/dir
                     * @param filePermissions correct permission for a given file
                     * @param fatalError a boolean parameter to decide to log error message or warning message . Default
                     * value is true
                     * @return an boolean value representing if permissions set on given file/dir are correct or not
                     */
                    static bool ValidateFilePermissions(
                        const std::string &path,
                        const int filePermissions,
                        bool fatalError = true);

                    /**
                     * Converts a file permissions mask into a human readable format:
                     *
                     * This function will return a 3-digit integer representing the permissions set by the mask.
                     * Each digit in the returned value will range from 0-7. The first digit is user, the second digit
                     * is group, and the third digit is world (everyone). The digit is determined by adding the
                     * permissions found from the following categories:
                     *
                     * 4 - has read privileges
                     * 2 - has write privileges
                     * 1 - has execute privileges
                     *
                     * @param mask the permissions mask
                     * @return an integer representing a human readable format of the permissions mask
                     */
                    static int PermissionsMaskToInt(mode_t mask);

                    /**
                     * \brief Returns the size of the file in bytes
                     * @param filePath the path to the file
                     * @return the size of the file in bytes
                     */
                    static size_t GetFileSize(const std::string &filePath);

                    /**
                     * Attempts to create the provided directory with the given permissions
                     * @param dirPath the path to the directory
                     * @param permissions the permission mask that should be applied to the directory
                     * @return true if the directory was successfully created with the given permissions, false
                     * otherwise
                     */
                    static bool CreateDirectoryWithPermissions(const char *dirPath, mode_t permissions);

                    /**
                     * \brief Check if the path exists and refers to a directory
                     * @param dirPath the path to the directory
                     * @return true if the path exists and is a directory, false otherwise
                     */
                    static bool DirectoryExists(const std::string &dirPath);

                    /**
                     * \brief Create an empty file with the given permissions
                     * @param filename the path to the file
                     * @param permissions the permission mask that should be applied to the file
                     * @return true if the file was successfully created with the given permissions, false otherwise
                     */
                    static bool CreateEmptyFileWithPermissions(const std::string &filename, mode_t permissions);

                    /**
                     * \brief Check if the given filename exists
                     *
                     * @param filename File Path to the Filename to check
                     * @return True if the file exists. False otherwise.
                     */
                    static bool FileExists(const std::string &filename);

                    /**
                     * \brief Check if the given filepath is valid
                     *
                     * @param filePath FilePath to check
                     * @return True if the filepath is valid. False otherwise.
                     */
                    static bool IsValidFilePath(const std::string &filePath);
                };

                struct wordexp_fail_error : std::runtime_error
                {
                    using std::runtime_error::runtime_error;
                };
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
#endif // AWS_IOT_DEVICE_CLIENT_FILEUTILS_H
