#include "controllers/UpdateClient.hpp"
#include "interfaces/Version.hpp"
#include "util/ControlChannel.hpp"
#include "util/ProcessId.hpp"
#include "util/ToolCli.hpp"
#include <csignal>
#include <iostream>
namespace
{
baz::Cancellation cancellation;
extern "C" void Cancel(int)
{
    cancellation.requested.store(true);
}
} // namespace
int main(int argc, const char *argv[])
{
    using namespace baz;
    try
    {
        ToolCli cli(argc, argv, "TUF-based application update client");
        auto &command = cli.tagless(1, "command")
                            .constrain(std::deque<std::string>{"check", "apply", "status", "verify",
                                                               "reconcile", "rollback", "capabilities"});
        auto &config =
            cli.option("--config", "-c", "Trusted local client configuration").withMaxValueCount(1);
        auto &file = cli.option("--file", "", "Downloaded artifact to verify").withMaxValueCount(1);
        auto &pid = cli.option("--wait-pid", "", "Application PID to await", "0").withMaxValueCount(1);
        auto &installed = cli.option("--installed", "", "Observed native installation result")
                              .withMaxValueCount(1)
                              .constrain(std::deque<std::string>{"true", "false"});
        auto &notify = cli.flag("--notify-ready", "Emit readiness before waiting for application exit");
        auto &control = cli.flag("--control-stdin", "Accept cooperative cancellation on stdin");
        cli.flag(
               "--version", [](const BazPO::Option &) { throw CliExit{std::string(Version) + "\n"}; },
               "Print tool version")
            .prioritize();
        cli.parse();
        if (!command.exists())
        {
            std::cout << cli.Help();
            return 0;
        }
        auto action = command.valueAs<std::string>();
        for (const auto *option : {&config, &file, &pid, &installed})
            Require(!option->exists() || option->values().size() == 1, ErrorCode::invalidInput,
                    "A provided client option requires one explicit value");
        Require(action == "apply" || (!pid.exists() && !notify.exists()), ErrorCode::invalidInput,
                "Readiness and PID options require apply");
        Require(action == "verify" || !file.exists(), ErrorCode::invalidInput, "--file requires verify");
        Require(action == "reconcile" || !installed.exists(), ErrorCode::invalidInput,
                "--installed requires reconcile");
        if (action == "capabilities")
        {
            Require(!config.exists() && !control.exists(), ErrorCode::invalidInput,
                    "capabilities does not use client options");
            std::cout << Capabilities().dump() << '\n';
            return 0;
        }
        auto configuration = Required(config);
        if (action == "verify")
        {
            auto artifact = fs::u8path(Required(file));
            Require(!fs::is_symlink(fs::symlink_status(artifact)) && fs::is_regular_file(artifact),
                    ErrorCode::invalidInput, "--file requires an existing regular file");
        }
        if (action == "reconcile")
            Required(installed);
        std::signal(SIGINT, Cancel);
        std::signal(SIGTERM, Cancel);
#ifndef _WIN32
        std::signal(SIGPIPE, SIG_IGN);
#endif
        ControlChannel channel(cancellation, control.exists());
        UpdateClient client(ClientConfig::Load(fs::u8path(configuration)));
        Json result;
        if (action == "check")
            result = client.Check(cancellation);
        else if (action == "apply")
        {
            std::function<void()> ready;
            if (notify.exists())
                ready = [] { std::cout << "{\"event\":\"ready\"}" << std::endl; };
            result = client.Apply(cancellation, ProcessId(pid.valueAs<std::string>()), ready);
        }
        else if (action == "status")
            result = client.Status();
        else if (action == "rollback")
            result = client.Rollback();
        else if (action == "reconcile")
            result = client.Reconcile(installed.valueAs<std::string>() == "true");
        else
            result = client.VerifyFile(fs::u8path(file.valueAs<std::string>()), cancellation);
        std::cout << result.dump() << '\n';
        return 0;
    }
    catch (const CliExit &done)
    {
        std::cout << done.output;
        return 0;
    }
    catch (const Error &error)
    {
        std::cerr << Json({{"status", "error"},
                           {"code", static_cast<int>(error.code)},
                           {"message", error.what()}})
                         .dump()
                  << '\n';
        return 2 + static_cast<int>(error.code);
    }
    catch (const std::exception &error)
    {
        std::cerr << Json({{"status", "error"}, {"code", "invalidInput"}, {"message", error.what()}}).dump()
                  << '\n';
        return 2;
    }
}
