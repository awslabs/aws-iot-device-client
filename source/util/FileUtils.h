

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
                    static std::string extractParentDirectory(std::string filePath);
                };
            } // namespace Util
        }     // namespace DeviceClient
    }         // namespace Iot
} // namespace Aws
#endif // AWS_IOT_DEVICE_CLIENT_FILEUTILS_H
