#include "controllers/PortableInstaller.hpp"
#include "interfaces/Version.hpp"
#include "publishing/RepositoryPublisher.hpp"
#include "publishing/WindowsSigning.hpp"
#include "util/ToolCli.hpp"
#include <iostream>
int main(int argc, const char *argv[])
{
    using namespace baz;
    try
    {
        ToolCli cli(argc, argv, "Signed repository lifecycle and native compatibility feeds");
        auto &command =
            cli.tagless(1, "command")
                .constrain(std::deque<std::string>{"init", "pack", "publish", "refresh", "restore-metadata",
                                                   "rotate-root", "export-electron", "export-appinstaller",
                                                   "create-test-certificate", "sign-windows"});
        auto &repo = cli.option("--repo", "", "Public repository directory");
        auto &keys = cli.option("--keys", "", "Protected signing-state directory outside the web root");
        auto &directory = cli.option("--directory", "", "Portable application source directory");
        auto &output = cli.option("--output", "", "Archive or native feed output");
        auto &artifact = cli.option("--artifact", "", "Completed release artifact");
        auto &target = cli.option("--target", "", "Logical target path");
        auto &descriptor = cli.option("--descriptor", "", "Signed application descriptor JSON");
        auto &baseUrl = cli.option("--base-url", "", "Public HTTPS repository base URL");
        auto &unsignedFeed =
            cli.option("--unsigned-native-feed", "", "Explicit unsigned Electron feed acknowledgement")
                .constrain(std::deque<std::string>{"true"});
        auto &thumbprint =
            cli.option("--certificate-thumbprint", "", "Explicit CurrentUser/My signing identity");
        auto &signTool = cli.option("--sign-tool", "", "Absolute trusted Windows SDK signtool executable");
        auto &timestamp = cli.option("--timestamp-url", "", "Optional HTTPS RFC 3161 timestamp service");
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
        bool pack = action == "pack", publish = action == "publish";
        bool certificate = action == "create-test-certificate", signing = action == "sign-windows";
        bool electron = action == "export-electron", feed = electron || action == "export-appinstaller";
        Require(pack || !directory.exists(), ErrorCode::invalidInput, "--directory requires pack");
        Require(pack || feed || certificate || !output.exists(), ErrorCode::invalidInput,
                "--output requires pack, certificate or feed export");
        Require(publish || !descriptor.exists(), ErrorCode::invalidInput, "--descriptor requires publish");
        Require(publish || signing || !artifact.exists(), ErrorCode::invalidInput,
                "--artifact requires publish or sign-windows");
        Require(signing || (!thumbprint.exists() && !signTool.exists() && !timestamp.exists()),
                ErrorCode::invalidInput, "Signing options require sign-windows");
        Require(publish || feed || !target.exists(), ErrorCode::invalidInput,
                "--target requires publish or export");
        Require(feed || !baseUrl.exists(), ErrorCode::invalidInput, "--base-url requires feed export");
        Require(electron || !unsignedFeed.exists(), ErrorCode::invalidInput,
                "Unsigned acknowledgement requires Electron export");
        Json result;
        if (certificate || signing)
        {
            Require(!repo.exists() && !keys.exists(), ErrorCode::invalidInput,
                    "Native signing does not use TUF repository keys");
#ifdef _WIN32
            if (certificate)
                result = CreateTestCertificate(fs::u8path(Required(output)));
            else
                result =
                    SignWindowsArtifact(fs::u8path(Required(artifact)), Required(thumbprint),
                                        fs::u8path(Required(signTool)), timestamp.valueAs<std::string>());
#else
            throw Error(ErrorCode::unsupported, "Native Windows signing tools require Windows");
#endif
        }
        else if (pack)
        {
            Require(!repo.exists() && !keys.exists(), ErrorCode::invalidInput,
                    "pack does not use repository signing state");
            PackDirectory(fs::u8path(Required(directory)), fs::u8path(Required(output)));
            result = {{"status", "packed"}};
        }
        else
        {
            auto root = Required(repo), privateState = Required(keys);
            if (publish)
            {
                Required(artifact);
                Required(target);
                Required(descriptor);
            }
            if (feed)
            {
                Required(target);
                Required(baseUrl);
                Required(output);
            }
            if (electron)
                Required(unsignedFeed);
            RepositoryPublisher publisher(fs::u8path(root), fs::u8path(privateState));
            if (action == "init")
                result = publisher.Initialize();
            else if (publish)
                result = publisher.Publish(fs::u8path(artifact.valueAs<std::string>()),
                                           target.valueAs<std::string>(),
                                           ReadJson(fs::u8path(descriptor.valueAs<std::string>())));
            else if (action == "refresh")
                result = publisher.Refresh();
            else if (action == "restore-metadata")
                result = publisher.RestoreMetadata();
            else if (action == "rotate-root")
                result = publisher.RotateRoot();
            else if (electron)
                result =
                    publisher.ExportElectron(target.valueAs<std::string>(), baseUrl.valueAs<std::string>(),
                                             fs::u8path(output.valueAs<std::string>()));
            else
                result = publisher.ExportAppInstaller(target.valueAs<std::string>(),
                                                      baseUrl.valueAs<std::string>(),
                                                      fs::u8path(output.valueAs<std::string>()));
        }
        std::cout << result.dump() << '\n';
        return 0;
    }
    catch (const CliExit &done)
    {
        std::cout << done.output;
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << Json({{"status", "error"}, {"message", error.what()}}).dump() << '\n';
        return 2;
    }
}
