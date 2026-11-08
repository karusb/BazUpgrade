#include "sdk/bazupgrade.h"
#include "controllers/UpdateClient.hpp"
#include "util/ProcessId.hpp"
#include <cstdlib>
#include <cstring>
struct baz_client
{
    explicit baz_client(baz::ClientConfig config) : client(std::move(config))
    {
    }
    baz::UpdateClient client;
    baz::Cancellation cancellation;
    std::atomic_flag busy = ATOMIC_FLAG_INIT;
};
namespace
{
void Output(char **output, const baz::Json &value)
{
    if (!output)
        return;
    auto text = value.dump();
    *output = static_cast<char *>(std::malloc(text.size() + 1));
    if (!*output)
        throw std::bad_alloc();
    std::memcpy(*output, text.c_str(), text.size() + 1);
}
int Failure(char **output, const std::exception &error) noexcept
{
    try
    {
        Output(output, {{"status", "error"}, {"message", error.what()}});
    }
    catch (...)
    {
    }
    return 1;
}
} // namespace
extern "C"
{
    int baz_client_create(const char *path, baz_client **output, char **error)
    {
        if (error)
            *error = nullptr;
        if (!path || !output)
            return 1;
        *output = nullptr;
        try
        {
            *output = new baz_client(baz::ClientConfig::Load(baz::fs::u8path(path)));
            return 0;
        }
        catch (const std::exception &e)
        {
            return Failure(error, e);
        }
        catch (...)
        {
            return 1;
        }
    }
    int baz_client_run(baz_client *handle, const char *command, const char *argument, char **result)
    {
        if (result)
            *result = nullptr;
        if (!handle || !command || !result)
            return 1;
        if (handle->busy.test_and_set())
            return 2;
        struct Guard
        {
            std::atomic_flag &busy;
            ~Guard()
            {
                busy.clear();
            }
        } guard{handle->busy};
        try
        {
            handle->cancellation.requested = false;
            std::string action(command), parameter = argument ? argument : "";
            baz::Json value;
            if (action == "check")
                value = handle->client.Check(handle->cancellation);
            else if (action == "apply")
                value = handle->client.Apply(handle->cancellation,
                                             parameter.empty() ? 0 : baz::ProcessId(parameter));
            else if (action == "status")
                value = handle->client.Status();
            else if (action == "verify")
                value = handle->client.VerifyFile(baz::fs::u8path(parameter), handle->cancellation);
            else if (action == "rollback")
                value = handle->client.Rollback();
            else if (action == "reconcile")
            {
                baz::Require(parameter == "true" || parameter == "false", baz::ErrorCode::invalidInput,
                             "Reconciliation requires true or false");
                value = handle->client.Reconcile(parameter == "true");
            }
            else
                throw baz::Error(baz::ErrorCode::invalidInput, "Unknown SDK command");
            Output(result, value);
            return 0;
        }
        catch (const std::exception &e)
        {
            return Failure(result, e);
        }
        catch (...)
        {
            return 1;
        }
    }
    void baz_client_cancel(baz_client *handle)
    {
        if (handle)
            handle->cancellation.requested = true;
    }
    void baz_client_destroy(baz_client *handle)
    {
        delete handle;
    }
    void baz_string_free(char *value)
    {
        std::free(value);
    }
}
