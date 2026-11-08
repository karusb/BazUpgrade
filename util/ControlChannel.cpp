#include "util/ControlChannel.hpp"
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#else
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace baz
{
ControlChannel::ControlChannel(Cancellation &cancellation, bool enabled)
{
    if (!enabled)
        return;
    reader = std::thread(
        [this, &cancellation]
        {
            std::string pending;
            while (!stopping.load())
            {
                char bytes[64];
                std::size_t count = 0;
#ifdef _WIN32
                HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
                if (input == INVALID_HANDLE_VALUE || GetFileType(input) != FILE_TYPE_PIPE)
                    return;
                DWORD available = 0, received = 0;
                if (!PeekNamedPipe(input, nullptr, 0, nullptr, &available, nullptr))
                    return;
                if (!available)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                if (!::ReadFile(input, bytes, static_cast<DWORD>(sizeof(bytes)), &received, nullptr))
                    return;
                count = received;
#else
                struct stat kind
                {
                };
                if (fstat(STDIN_FILENO, &kind) != 0 || (!S_ISFIFO(kind.st_mode) && !S_ISSOCK(kind.st_mode)))
                    return;
                pollfd input{STDIN_FILENO, POLLIN, 0};
                auto ready = poll(&input, 1, 100);
                if (ready < 0)
                    return;
                if (!ready)
                    continue;
                auto received = read(STDIN_FILENO, bytes, sizeof(bytes));
                if (received <= 0)
                    return;
                count = static_cast<std::size_t>(received);
#endif
                if (!count)
                    return;
                for (std::size_t i = 0; i < count; ++i)
                {
                    if (bytes[i] == '\n')
                    {
                        if (pending == "cancel")
                            cancellation.requested = true;
                        pending.clear();
                    }
                    else if (pending.size() < 128)
                        pending += bytes[i];
                    else
                        return;
                }
            }
        });
}
ControlChannel::~ControlChannel()
{
    stopping = true;
    if (reader.joinable())
        reader.join();
}
} // namespace baz
