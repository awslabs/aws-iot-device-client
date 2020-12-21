

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

                    static int getFilePermissions(const std::string &filePath);
                };
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
#endif // AWS_IOT_DEVICE_CLIENT_FILEUTILS_H
