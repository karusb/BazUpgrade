#include "curl/RepositoryTransport.hpp"
#include "curl/HttpClientCurl.hpp"
#include "interfaces/HttpUrl.hpp"
#include <fstream>
#include <memory>

namespace baz
{
DirectoryTransport::DirectoryTransport(fs::path directory) : root(fs::canonical(std::move(directory)))
{
}

fs::path DirectoryTransport::Resolve(const std::string &path) const
{
    fs::path result = root;
    for (const auto &part : SafeRelative(path))
    {
        result /= part;
        Require(!fs::is_symlink(fs::symlink_status(result)), ErrorCode::transport,
                "Repository symlinks are not allowed");
    }
    return result;
}

std::optional<std::string> DirectoryTransport::Fetch(const std::string &path, std::uint64_t limit,
                                                     const Cancellation &cancel)
{
    cancel.Check();
    auto file = Resolve(path);
    if (!fs::exists(file))
        return std::nullopt;
    return ReadFile(file, limit);
}

void DirectoryTransport::Download(const std::string &path, const fs::path &destination, std::uint64_t limit,
                                  const Cancellation &cancel)
{
    Require(!fs::exists(destination) && !fs::is_symlink(fs::symlink_status(destination)), ErrorCode::io,
            "Download destination already exists");
    std::ifstream input(Resolve(path), std::ios::binary);
    std::ofstream output(destination, std::ios::binary);
    Require(input.good() && output.good(), ErrorCode::transport, "Cannot open artifact transfer");
    char buffer[65536];
    std::uint64_t total = 0;
    while (input.read(buffer, sizeof(buffer)) || input.gcount())
    {
        cancel.Check();
        total += static_cast<std::uint64_t>(input.gcount());
        Require(total <= limit, ErrorCode::transport, "Artifact exceeds signed size");
        output.write(buffer, input.gcount());
        Require(output.good(), ErrorCode::io, "Artifact write failed");
    }
    Require(input.eof(), ErrorCode::transport, "Artifact transfer failed");
    output.close();
    Require(output.good(), ErrorCode::io, "Artifact write failed");
}

std::string CurlTransport::ResolveUrl(const std::string &path) const
{
    SafeRelative(path);
    try
    {
        return curlwrapper::HttpUrl(baseUrl).AppendPath(path).String();
    }
    catch (const curlwrapper::Error &error)
    {
        throw Error(ErrorCode::transport, error.what());
    }
}
CurlTransport::CurlTransport(std::string url, bool allowLoopbackHttp) : baseUrl(std::move(url))
{
    try
    {
        const curlwrapper::HttpUrl endpoint(baseUrl);
        Require(!endpoint.HasCredentials() && !endpoint.HasQuery() && !endpoint.HasFragment(),
                ErrorCode::invalidInput, "Repository URLs cannot contain credentials, a query or a fragment");
        Require(endpoint.Scheme() == "https" ||
                    (allowLoopbackHttp && (endpoint.Host() == "localhost" || endpoint.Host() == "127.0.0.1" ||
                                           endpoint.Host() == "::1")),
                ErrorCode::invalidInput, "HTTPS is required; development HTTP is restricted to loopback");
        curlwrapper::RequestOptions options;
        options.httpsOnly = !allowLoopbackHttp;
        options.followRedirects = false;
        options.useEnvironmentProxy = false;
        options.allowUrlCredentials = false;
        options.connectTimeoutSeconds = 10;
        options.timeoutSeconds = 300;
        options.lowSpeedLimit = 1;
        options.lowSpeedSeconds = 20;
        client = std::make_unique<HTTPClientCurl>(options);
    }
    catch (const curlwrapper::Error &error)
    {
        throw Error(error.failure == curlwrapper::Failure::invalidInput ? ErrorCode::invalidInput
                                                                        : ErrorCode::transport,
                    error.what());
    }
}
CurlTransport::~CurlTransport() = default;
std::optional<std::string> CurlTransport::Fetch(const std::string &path, std::uint64_t limit,
                                                const Cancellation &cancel)
{
    cancel.Check();
    curlwrapper::TransferOptions transfer;
    transfer.maxBytes = limit;
    transfer.cancelled = [&cancel] { return cancel.requested.load(); };
    try
    {
        auto response = client->Get(ResolveUrl(path), transfer);
        if (response.status == 404)
            return std::nullopt;
        Require(response.status == 200, ErrorCode::transport, "Metadata HTTP response was not 200");
        return response.body;
    }
    catch (const curlwrapper::Error &error)
    {
        throw Error(error.failure == curlwrapper::Failure::cancelled ? ErrorCode::cancelled
                                                                     : ErrorCode::transport,
                    error.what());
    }
}
void CurlTransport::Download(const std::string &path, const fs::path &destination, std::uint64_t limit,
                             const Cancellation &cancel)
{
    cancel.Check();
    const auto url = ResolveUrl(path);
    Require(!fs::exists(destination) && !fs::is_symlink(fs::symlink_status(destination)), ErrorCode::io,
            "Download destination already exists");
    std::ofstream output(destination, std::ios::binary);
    Require(output.good(), ErrorCode::io, "Cannot create artifact file");
    curlwrapper::TransferOptions transfer;
    transfer.maxBytes = limit;
    transfer.cancelled = [&cancel] { return cancel.requested.load(); };
    try
    {
        const auto response = client->Download(url, output, transfer);
        Require(response.status == 200 && output.good(), ErrorCode::transport,
                "Artifact HTTP response was not 200");
    }
    catch (const curlwrapper::Error &error)
    {
        throw Error(error.failure == curlwrapper::Failure::cancelled ? ErrorCode::cancelled
                                                                     : ErrorCode::transport,
                    error.what());
    }
    output.close();
    Require(output.good(), ErrorCode::io, "Artifact write failed");
}
} // namespace baz
