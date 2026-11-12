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

std::string CurlTransport::EncodePath(const std::string &path)
{
    SafeRelative(path);
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string result;
    for (unsigned char c : path)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' ||
            c == '_' || c == '.' || c == '~' || c == '/')
            result += static_cast<char>(c);
        else
        {
            result += '%';
            result += digits[c >> 4];
            result += digits[c & 15];
        }
    }
    return result;
}
curlwrapper::Response CurlTransport::Perform(const std::string &path, std::uint64_t limit,
                                             const Cancellation &cancel, std::ofstream *file)
{
    curlwrapper::Request request;
    request.url = baseUrl + EncodePath(path);
    request.options.emplace();
    request.options->httpsOnly = !loopbackHttp;
    request.options->followRedirects = false;
    request.options->useEnvironmentProxy = false;
    request.options->allowUrlCredentials = false;
    request.options->maxBytes = limit;
    request.options->connectTimeoutSeconds = 10;
    request.options->timeoutSeconds = 300;
    request.options->lowSpeedLimit = 1;
    request.options->lowSpeedSeconds = 20;
    request.cancelled = [&cancel] { return cancel.requested.load(); };
    if (file)
        request.sink = [file](const char *bytes, std::size_t length)
        {
            file->write(bytes, static_cast<std::streamsize>(length));
            return file->good();
        };
    try
    {
        return client->Perform(request);
    }
    catch (const curlwrapper::Error &error)
    {
        throw Error(error.failure == curlwrapper::Failure::cancelled ? ErrorCode::cancelled
                                                                     : ErrorCode::transport,
                    error.what());
    }
}
CurlTransport::CurlTransport(std::string url, bool allowLoopbackHttp)
    : CurlTransport(std::move(url), nullptr, allowLoopbackHttp)
{
}
CurlTransport::CurlTransport(std::string url, std::unique_ptr<HTTPClient> client, bool allowLoopbackHttp)
    : baseUrl(std::move(url)), loopbackHttp(allowLoopbackHttp), client(std::move(client))
{
    try
    {
        const curlwrapper::HttpUrl repository(baseUrl);
        Require(!repository.HasCredentials() && !repository.HasQuery() && !repository.HasFragment(),
                ErrorCode::invalidInput, "Repository URLs cannot contain credentials, a query or a fragment");
        Require(repository.Scheme() == "https" ||
                    (allowLoopbackHttp && (repository.Host() == "localhost" ||
                                           repository.Host() == "127.0.0.1" || repository.Host() == "::1")),
                ErrorCode::invalidInput, "HTTPS is required; development HTTP is restricted to loopback");
        if (!this->client)
            this->client = std::make_unique<HTTPClientCurl>();
    }
    catch (const curlwrapper::Error &error)
    {
        throw Error(error.failure == curlwrapper::Failure::invalidInput ? ErrorCode::invalidInput
                                                                        : ErrorCode::transport,
                    error.what());
    }
    if (baseUrl.back() != '/')
        baseUrl += '/';
}
CurlTransport::~CurlTransport() = default;
std::optional<std::string> CurlTransport::Fetch(const std::string &path, std::uint64_t limit,
                                                const Cancellation &cancel)
{
    auto response = Perform(path, limit, cancel, nullptr);
    if (response.status == 404)
        return std::nullopt;
    Require(response.status == 200, ErrorCode::transport, "Metadata HTTP response was not 200");
    return response.body;
}
void CurlTransport::Download(const std::string &path, const fs::path &destination, std::uint64_t limit,
                             const Cancellation &cancel)
{
    Require(!fs::exists(destination) && !fs::is_symlink(fs::symlink_status(destination)), ErrorCode::io,
            "Download destination already exists");
    std::ofstream output(destination, std::ios::binary);
    Require(output.good(), ErrorCode::io, "Cannot create artifact file");
    auto response = Perform(path, limit, cancel, &output);
    Require(response.status == 200 && output.good(), ErrorCode::transport,
            "Artifact HTTP response was not 200");
    output.close();
    Require(output.good(), ErrorCode::io, "Artifact write failed");
}
} // namespace baz
