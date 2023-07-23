#include "instantiations/BazUpgradeGitHub.hpp"
#include "instantiations/TracingBazUpgradeGitHub.hpp"
#include "BazPO.hpp"

using namespace std;
using namespace BazPO;

int main(int argc, const char* argv[])
{   
    string user = "karusb";
    string repo = "FLF-FindLargeFolders";
    string currentVersion = "2.1";
    string assetIdentifier = "Portable";
    bool alphaReleases = true;
    bool downloadTarBall = false;
    bool downloadZipBall = false;
    bool unzipFiles = true;
    string unzipPath = ".";
    string launchPath = "\"BazWare FLF - Find Large Folders-2.2.exe\"";
    bool tracing = true;

    Cli po(argc, argv, "AutoUpgrader v1.0.0");

    po.option("-r", [&user, &repo](const Option& option) 
        { 
            auto path = option.valueAs<string>();
            auto separator = path.find('/');
            user = path.substr(0, separator);
            repo = path.substr(separator + 1);
        }
    , "--repository", "Repository path on GitHub <user>/<repository>").withMaxValueCount(1).mandatory()
    .constrain([](const Option& value) -> bool { return value.valueAs<string>().find('/') != string::npos; }, "incorrect format given for the repository path");
    po.option("-v", [&currentVersion](const Option& option) { currentVersion = option.valueAs<string>(); }
    , "--version", "Current version of the software for version comparison, when not provided downloads the latest release").withMaxValueCount(1);
    po.option("-s", [&assetIdentifier](const Option& option) { assetIdentifier = option.valueAs<string>(); },
        "--select", "Asset selector, downloads the first asset containing this value i.e Windows").withMaxValueCount(1);
    po.flag("-a", [&alphaReleases](const Option& option) { alphaReleases = option.exists(); },
        "Allow updating to pre-release or draft versions", "--alpha");
    po.flag("-tb", [&downloadTarBall](const Option& option) { downloadTarBall = option.exists(); },
        "Download Source Tarball", "--tarball");
    po.flag("-zb", [&downloadZipBall](const Option& option) { downloadZipBall = option.exists(); },
        "Download Source Zipball", "--zipball");
    po.flag("-t", [&tracing](const Option& option) { tracing = option.exists(); },
        "Enable tracing", "--tracing");
    po.option("-uz", [&unzipFiles, &unzipPath](const Option& option) { unzipFiles = option.exists(); unzipPath = option.valueAs<string>();},
        "Unzips all downloaded files to the provided path", "--unzip", ".").withMaxValueCount(1);
    po.option("-l", [&launchPath](const Option& option) { launchPath = option.valueAs<string>();},
        "Launches the provided executable at path after download & extract", "--launch").withMaxValueCount(1);
    po.parse();

//////////// TODO:
    // - More Tracing classes / verbose option
    // - Setup Repo and check GitHub Actions with Code Coverage
    // - Readme
    // - Unblocking launcher (maybe OS specific process launchers)
    // - Zip file cleanup?
    // - Self updater? (pass in arguments to the new release once downloaded)

    if (tracing)
    {
        BazUpgradeGitHub upgrader(user, repo, currentVersion, assetIdentifier, alphaReleases, downloadTarBall, downloadZipBall, unzipFiles, unzipPath, launchPath);
        return upgrader.Upgrade();
    }
    else
    {
        TracingBazUpgradeGitHub upgrader(user, repo, currentVersion, assetIdentifier, alphaReleases, downloadTarBall, downloadZipBall, unzipFiles, unzipPath, launchPath, std::cout);
        return upgrader.Upgrade();
    }
}
