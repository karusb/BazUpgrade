#pragma once

#include "controllers/PortableInstaller.hpp"
#include "controllers/UpdateClient.hpp"
#include "crypto/OpenSslCrypto.hpp"
#include "curl/RepositoryTransport.hpp"
#include "publishing/RepositoryPublisher.hpp"
#include "storage/FileLock.hpp"
#include "tuf/Metadata.hpp"
#include <gtest/gtest.h>

namespace test
{
using namespace baz;

template <class Operation>
::testing::AssertionResult FailsWithUpdateError(ErrorCode expected, Operation operation)
{
    try
    {
        operation();
    }
    catch (const Error &error)
    {
        if (error.code == expected)
            return ::testing::AssertionSuccess();

        return ::testing::AssertionFailure()
               << "Expected update error " << static_cast<int>(expected) << ", received "
               << static_cast<int>(error.code) << ": " << error.what();
    }

    return ::testing::AssertionFailure() << "The operation completed without an update error";
}

void WriteFixtureFile(const fs::path &path, const std::string &bytes);

class TemporaryDirectory final
{
  public:
    TemporaryDirectory();
    ~TemporaryDirectory();
    TemporaryDirectory(const TemporaryDirectory &) = delete;
    TemporaryDirectory &operator=(const TemporaryDirectory &) = delete;

    const fs::path &Path() const noexcept;

  private:
    fs::path directory;
};

class RepositoryFixture final
{
  private:
    TemporaryDirectory workspace;

  public:
    RepositoryFixture();
    Json BuildReleaseDescriptor(std::uint64_t sequence = 1) const;
    void PublishApplicationRelease(std::uint64_t sequence = 1);
    void WriteApplicationArchive(const std::string &contents);
    void WriteSignedMetadata(const std::string &filename, const Json &body, const std::string &role);
    void AddDelegatedRole(const std::string &name, const std::string &pattern, bool terminating,
                          bool includeTarget);
    void CreateUnsafeArchive(const std::string &name, bool symlink = false);
#ifdef _WIN32
    void ReplaceMetadataWithJunctionTo(const fs::path &destination);
#endif

    const fs::path root;
    const fs::path repo;
    const fs::path keys;
    const fs::path bootstrap;
    const fs::path package;
    ClientConfig config;
    OpenSslCrypto crypto;
    Cancellation cancel;

  private:
    void ConfigureClient();
    std::vector<fs::path> GetRoleSigningKeys(const std::string &role) const;
    Json DescribeMetadata(const Json &envelope) const;
    void WriteVersionedMetadata(const std::string &role, const Json &envelope);
    void UpdateDelegationMetadataChain(const std::string &name, const Json &parent, const Json &child);
};

class RepositoryTest : public ::testing::Test
{
  protected:
    RepositoryFixture repository;
};
} // namespace test
