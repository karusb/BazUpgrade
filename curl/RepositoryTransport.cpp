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

fs::path DirectoryTransport::ResolveRepositoryPath(const std::string &path) const
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

    auto file = ResolveRepositoryPath(path);

    if (!fs::exists(file))
        return std::nullopt;

    return ReadFile(file, limit);
}

void DirectoryTransport::CopyArtifact(std::istream &input, std::ostream &output, std::uint64_t limit,
                                      const Cancellation &cancel)
{
    char buffer[65536];
    std::uint64_t total = 0;

    while (input.read(buffer, sizeof(buffer)) || input.gcount())
    {
        cancel.Check();
        const auto length = static_cast<std::uint64_t>(input.gcount());

        Require(length <= limit - total, ErrorCode::transport, "Artifact exceeds signed size");

        output.write(buffer, input.gcount());
        Require(output.good(), ErrorCode::io, "Artifact write failed");
        total += length;
    }

    Require(input.eof(), ErrorCode::transport, "Artifact transfer failed");
}

void DirectoryTransport::Download(const std::string &path, const fs::path &destination, std::uint64_t limit,
                                  const Cancellation &cancel)
{
    cancel.Check();
    Require(!fs::exists(destination) && !fs::is_symlink(fs::symlink_status(destination)), ErrorCode::io,
            "Download destination already exists");

    std::ifstream input(ResolveRepositoryPath(path), std::ios::binary);
    std::ofstream output(destination, std::ios::binary);
    Require(input.good() && output.good(), ErrorCode::transport, "Cannot open artifact transfer");

    CopyArtifact(input, output, limit, cancel);

    output.close();
    Require(output.good(), ErrorCode::io, "Artifact write failed");
}

void CurlTransport::ValidateRepositoryUrl(const curlwrapper::HttpUrl &url, bool allowLoopbackHttp)
{
    Require(!url.HasCredentials() && !url.HasQuery() && !url.HasFragment(), ErrorCode::invalidInput,
            "Repository URLs cannot contain credentials, a query or a fragment");

    const auto &host = url.Host();
    const bool loopback = host == "localhost" || host == "127.0.0.1" || host == "::1";

    Require(url.Scheme() == "https" || (allowLoopbackHttp && loopback), ErrorCode::invalidInput,
            "HTTPS is required; development HTTP is restricted to loopback");
}

curlwrapper::RequestOptions CurlTransport::CreateRepositoryRequestOptions(bool allowLoopbackHttp)
{
    curlwrapper::RequestOptions options;

    options.httpsOnly = !allowLoopbackHttp;
    options.followRedirects = false;
    options.useEnvironmentProxy = false;
    options.allowUrlCredentials = false;
    options.connectTimeoutSeconds = 10;
    options.timeoutSeconds = 300;
    options.lowSpeedLimit = 1;
    options.lowSpeedSeconds = 20;

    return options;
}

curlwrapper::TransferOptions CurlTransport::CreateBoundedTransferOptions(std::uint64_t limit,
                                                                         const Cancellation &cancel)
{
    curlwrapper::TransferOptions transfer;

    transfer.maxBytes = limit;
    transfer.cancelled = [&cancel] { return cancel.requested.load(); };

    return transfer;
}

Error CurlTransport::TranslateTransferError(const curlwrapper::Error &error, ErrorCode inputError)
{
    auto code = ErrorCode::transport;

    if (error.failure == curlwrapper::Failure::cancelled)
        code = ErrorCode::cancelled;
    else if (error.failure == curlwrapper::Failure::invalidInput)
        code = inputError;

    return Error(code, error.what());
}

CurlTransport::CurlTransport(std::string url, bool allowLoopbackHttp)
{
    try
    {
        baseUrl = std::make_unique<curlwrapper::HttpUrl>(std::move(url));
        ValidateRepositoryUrl(*baseUrl, allowLoopbackHttp);
        client = std::make_unique<HTTPClientCurl>(CreateRepositoryRequestOptions(allowLoopbackHttp));
    }
    catch (const curlwrapper::Error &error)
    {
        throw TranslateTransferError(error, ErrorCode::invalidInput);
    }
}

CurlTransport::~CurlTransport() = default;

std::string CurlTransport::ResolveRepositoryUrl(const std::string &path) const
{
    SafeRelative(path);

    try
    {
        return baseUrl->AppendPath(path).String();
    }
    catch (const curlwrapper::Error &error)
    {
        throw TranslateTransferError(error);
    }
}

std::optional<std::string> CurlTransport::Fetch(const std::string &path, std::uint64_t limit,
                                                const Cancellation &cancel)
{
    cancel.Check();
    try
    {
        auto response = client->Get(ResolveRepositoryUrl(path), CreateBoundedTransferOptions(limit, cancel));

        if (response.status == 404)
            return std::nullopt;

        Require(response.status == 200, ErrorCode::transport, "Metadata HTTP response was not 200");

        return response.body;
    }
    catch (const curlwrapper::Error &error)
    {
        throw TranslateTransferError(error);
    }
}

void CurlTransport::Download(const std::string &path, const fs::path &destination, std::uint64_t limit,
                             const Cancellation &cancel)
{
    cancel.Check();
    const auto url = ResolveRepositoryUrl(path);

    Require(!fs::exists(destination) && !fs::is_symlink(fs::symlink_status(destination)), ErrorCode::io,
            "Download destination already exists");
    std::ofstream output(destination, std::ios::binary);
    Require(output.good(), ErrorCode::io, "Cannot create artifact file");

    try
    {
        const auto response = client->Download(url, output, CreateBoundedTransferOptions(limit, cancel));
        Require(response.status == 200 && output.good(), ErrorCode::transport,
                "Artifact HTTP response was not 200");
    }
    catch (const curlwrapper::Error &error)
    {
        throw TranslateTransferError(error);
    }

    output.close();
    Require(output.good(), ErrorCode::io, "Artifact write failed");
}
} // namespace baz
