#ifndef AWS_IOT_DEVICE_CLIENT_FILEUTILS_H
#define AWS_IOT_DEVICE_CLIENT_FILEUTILS_H

#include <string>

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
                    static int mkdirs(const char *path);

                    /**
                     * \brief Given a path to a file, attempts to extract the parent directory
                     * @param filePath a path to a file
                     * @return the parent directory of the file
                     */
                    static std::string extractParentDirectory(const std::string &filePath);

                    /**
                     * \brief Stores string value in given file
                     * @param value string value to be stored in given file
                     * @param filePath a path to a file
                     * @return true on success
                     */
                    static bool StoreValueInFile(std::string value, std::string filePath);

                    /**
                     * \brief Returns an integer representing the permissions of the specified file.
                     *
                     * @param filePath a path to a file
                     * @return an integer representing the file permissions.
                     */
                    static int getFilePermissions(const std::string &filePath);

                    /**
                     * Converts a file permissions mask into a human readable format
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
                    static int permissionsMaskToInt(mode_t mask);

                    /**
                     * \brief Returns the size of the file in bytes
                     * @param filePath the path to the file
                     * @return the size of the file in bytes
                     */
                    static size_t getFileSize(const std::string &filePath);

                    /**
                     * Attempts to create the provided directory with the given permissions
                     * @param dirPath the path to the directory
                     * @param permissions the permission mask that should be applied to the directory
                     * @return true if the directory was successfully created with the given permissions, false
                     * otherwise
                     */
                    static bool createDirectoryWithPermissions(const char *dirPath, mode_t permissions);
                };
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
#endif // AWS_IOT_DEVICE_CLIENT_FILEUTILS_H
