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

    baz::Json RunUpdateCommand(const std::string &action, const std::string &parameter)
    {
        if (action == "check")
            return client.CheckForUpdate(cancellation);
        else if (action == "apply")
            return client.ApplyUpdate(cancellation, parameter.empty() ? 0 : baz::ProcessId(parameter));
        else if (action == "status")
            return client.ReadInstallationState();
        else if (action == "verify")
            return client.VerifyDownloadedFile(baz::fs::u8path(parameter), cancellation);
        else if (action == "rollback")
            return client.RollbackPortableInstallation();
        else if (action == "reconcile")
        {
            baz::Require(parameter == "true" || parameter == "false", baz::ErrorCode::invalidInput,
                         "Reconciliation requires true or false");
            return client.ReconcileInstallationOutcome(parameter == "true");
        }
        else
            throw baz::Error(baz::ErrorCode::invalidInput, "Unknown SDK command");
    }

    baz::UpdateClient client;
    baz::Cancellation cancellation;
    std::atomic_flag busy = ATOMIC_FLAG_INIT;
};

namespace
{
void AllocateJsonResult(char **output, const baz::Json &value)
{
    if (!output)
        return;

    auto text = value.dump();
    *output = static_cast<char *>(std::malloc(text.size() + 1));
    if (!*output)
        throw std::bad_alloc();

    std::memcpy(*output, text.c_str(), text.size() + 1);
}

int ReportOperationError(char **output, const std::exception &error) noexcept
{
    try
    {
        AllocateJsonResult(output, {{"status", "error"}, {"message", error.what()}});
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
            return ReportOperationError(error, e);
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
            const auto value = handle->RunUpdateCommand(command, argument ? argument : "");

            AllocateJsonResult(result, value);

            return 0;
        }
        catch (const std::exception &e)
        {
            return ReportOperationError(result, e);
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
