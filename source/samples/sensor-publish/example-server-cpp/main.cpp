// Serve lines from a file over a Unix Domain Socket.
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

// Wrapper for system call errors.
class syscall_error : public std::runtime_error
{

  public:
    syscall_error(const std::string &msg, int errnum) : std::runtime_error(msg), errno_(errnum)
    {
        std::ostringstream oss;
        oss << msg << ": " << std::strerror(errno_) << " (" << errno_ << ")";
        what_ = oss.str();
    }

    virtual const char *what() const noexcept { return what_.c_str(); }

    std::string what_;
    int errno_;
};

// getenv reads value from environment and converts to any type.
template <typename T> T getenv(const std::string &name, const T &default_value)
{
    auto psv = std::getenv(name.c_str());
    if (psv)
    {
        T val;
        std::istringstream iss;
        iss.str(psv);
        iss >> val;
        return val;
    }
    return default_value;
}

// _s converts the null terminated string to std::string.
std::string _s(const char *ps)
{
    return std::string(ps);
}

// read the lines of the file into a buffer.
std::vector<std::string> read(const std::string &filename, const std::string &delim, bool ignore_if_empty = true)
{
    std::vector<std::string> lines;

    std::ifstream ifs(filename);
    if (!ifs)
    {
        throw syscall_error("Error opening file", errno);
    }

    std::string line;
    while (std::getline(ifs, line))
    {
        if (ignore_if_empty && line.empty())
        {
            continue;
        }
        lines.push_back(line.append(delim));
    }

    if (ifs.bad())
    {
        throw syscall_error("Error reading file", errno);
    }

    if (lines.empty())
    {
        throw std::runtime_error("Error empty file");
    }

    return lines;
}

int main(int argc, char *argv[])
{
    std::string filename = getenv("FILENAME", _s("/src/.marcoaz/sample-sensor-data.txt"));
    std::string path = getenv("SUN_PATH", _s("/tmp/sensors/my-sensor-server"));
    std::string delim = getenv("DELIM", _s("\n"));
    unsigned int delay_ms = getenv("DELAY_MS", 1000U);
    bool repeat_file = getenv("REPEAT_FILE", true);
    bool check_peercred = getenv("CHECK_PEERCRED", true);

    // Ignore SIGPIPE and handle errors when remote closes on write.
    signal(SIGPIPE, SIG_IGN);

    if (check_peercred)
    {
        std::cout << "server uid:" << getuid() << "\n";
    }

    // Read the lines of the file into memory.
    std::vector<std::string> lines = read(filename, delim);

    // Create socket.
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        throw syscall_error("Error creating socket", errno);
    }

    // Delete path.
    unlink(path.c_str());

    // Check length of path.
    struct sockaddr_un addr;
    if (path.size() > sizeof(addr.sun_path) - 1)
    {
        throw std::runtime_error("Socket path too long");
    }

    // Initialize address of server.
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    // Set umask so that pathname socket is created with user and group writeable eg 660.
    umask(S_IXUSR | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);

    // Bind the socket to the address.
    const struct sockaddr *paddr = reinterpret_cast<struct sockaddr *>(&addr);
    if (bind(sockfd, paddr, sizeof(addr)) == -1)
    {
        throw syscall_error("Error binding socket", errno);
    }

    // Listen for client connections.
    if (listen(sockfd, SOMAXCONN) == -1)
    {
        throw syscall_error("Error listening on socket", errno);
    }

    while (1)
    {
        // Accept new client connections.
        int clientfd = accept(sockfd, nullptr, nullptr);
        if (clientfd == -1)
        {
            throw syscall_error("Error accepting client", errno);
        }

        // Check socket peer credential.
        if (check_peercred)
        {
            struct ucred peercred;
            socklen_t size_cred = sizeof(peercred);
            std::memset(&peercred, 0, size_cred);

            if (getsockopt(clientfd, SOL_SOCKET, SO_PEERCRED, &peercred, &size_cred) == -1)
            {
                throw syscall_error("Error reading peer credential", errno);
            }

            std::cout << "pid:" << peercred.pid << " uid:" << peercred.uid << " gid:" << peercred.gid << "\n";

            if (peercred.uid != getuid())
            {
                std::cerr << "client connection rejected, peer and server uid do not match\n";
                close(clientfd);
                continue;
            }
        }

        bool closed = false;

        // Write file until client closes connection.
        while (!closed)
        {
            for (const auto &line : lines)
            {
                // Send one line to the client.
                std::string::size_type nbytes = 0;
                while (nbytes != line.size())
                {
                    ssize_t count = write(clientfd, &line[nbytes], line.size() - nbytes);
                    if (count == -1)
                    {
                        if (errno == EPIPE)
                        {
                            closed = true;
                            break;
                        }
                        // Ignore signal interruption.
                        else if (errno != EINTR)
                        {
                            throw syscall_error("Error writing client socket", errno);
                        }
                        continue;
                    }
                    nbytes += count;
                }
                if (closed)
                {
                    break;
                }

                std::cout << "sent: " << line;

                // Sleep before sending the next line to client.
                if (delay_ms > 0U)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                }
            }

            std::cout << (closed ? "EPIPE" : "EOF") << "\n";
            if (!repeat_file)
            {
                break;
            }
        }

        // Close server connection with client.
        close(clientfd);
    }

    exit(EXIT_SUCCESS);
}
