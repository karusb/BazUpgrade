# Development and releases

## Local flow

Install the [separate dependency SDK](installation.md), then:

~~~sh
cmake --preset dev
cmake --build --preset dev --parallel 4
ctest --preset dev
node --test sdk/node/test.mjs
~~~

Release workflow preset: cmake --workflow --preset release. Windows CI uses the windows preset; newer Visual Studio Developer shells can use Ninja with the vcpkg arguments in the installation guide.

Python test suites exercise CLI/C ABI, local HTTP and the official TUF client. BAZUPGRADE_REQUIRE_REFERENCE_TUF=1 makes missing interoperability support fail instead of skipping. Keys and applications are generated in isolated synthetic directories.

## Independent builds

~~~mermaid
flowchart LR
  L[Private library build] --> S[Versioned CMake SDK artifact]
  S --> B[Manual BazUpgrade build]
  P[Public BazUpgrade PR] --> C[Formatting and Node tests]
  B --> T[Build / CTest / SDK consumer]
  T --> A[CPack / Packaged tool and C ABI checks]
  A --> F[BazUpgrade artifacts]
  F --> R[BazUpgrade draft release]
~~~

CurlWrapper owns its SDK build and tests. It contains no BazUpgrade workflow, configuration or documentation. BazUpgrade consumes an installed CMake SDK as an ordinary dependency; its build never checks out or builds the library's private implementation. The SDK's API headers are needed during the authorized build and are excluded from public packages and uploaded artifacts.

Run the independent SDK build first. It produces a private `sdk-<OS>-<ARCH>-<attempt>` artifact for each platform, containing `CurlWrapper-SDK.zip`, a SHA-256 checksum and source identity. Then manually dispatch this repository's `build.yml` at a reviewed BazUpgrade ref with:

- `curlwrapper-sha`: the reviewed SDK source commit (40 hex digits).
- `sdk-build-run`: the successful SDK producer run ID.
- `sdk-build-attempt`: the exact producer attempt, normally `1`.

Configure `CURLWRAPPER_ARTIFACT_READ_TOKEN` as a fine-grained Actions-read token for the SDK producer repository. It is supplied only to the dependency download step. The action checks the producer workflow, event, conclusion, commit and attempt, then verifies the SDK platform/runner identity and ZIP checksum. Fork checks receive no token or private SDK. Do not dispatch a credential-bearing build for unreviewed code.

The BazUpgrade workflow has visible configure/build/CTest, installed consumer, coverage, sanitizer and package verification steps. Its artifacts belong to this repository. CMake/Ninja come from the pinned native setup action; Python installs only interoperability and coverage test dependencies. The same CMake commands run locally; no custom Python CI runner is used.

Workflow-dispatch entry points must be registered on the default branch before GitHub exposes their dispatch controls. Feature-branch delivery does not merge them or start a run. No hosted runs were requested because of the CI credit constraint; local results and unexecuted runner configuration are distinguished in [validation](validation.md).

## Coverage and sanitizers

~~~sh
cmake --preset coverage
cmake --build --preset coverage --parallel 4
ctest --preset coverage
gcovr --root . --filter '(controllers|crypto|curl|publishing|storage|tuf|util)/' out/coverage
cmake --preset sanitizers
cmake --build --preset sanitizers --parallel 4
ctest --preset sanitizers -R bazupgrade.behavior
~~~

Loading an ASan SDK into an unrelated Python process requires matching runtime setup. Run the instrumented native behavior binary for sanitizers. Unit/integration coverage does not establish signed native installation or reboot E2E coverage.

Format project C++ with clang-format 18 and the committed style. See [Windows Docker E2E](../e2e/windows/README.md) for privileged tests inside a disposable guest.

## Release a reviewed commit

1. Complete this repository's manual build for the exact BazUpgrade ref and reviewed prebuilt SDK. Inspect all three platform results and package verification.
2. Complete the required native E2E and security review for the intended audience. Review [validation](validation.md) and [support](support.md).
3. Create an immutable tag matching VERSION.txt at the tested BazUpgrade SHA.
4. Manually run `release.yml` with the tag, BazUpgrade SHA, successful BazUpgrade build run ID and its attempt number.
5. Inspect and publish the resulting draft release.

The release workflow reads only this repository's artifacts with its own Actions-read `GITHUB_TOKEN`. It verifies the successful build event/workflow, exact commit, existing tag/version, consistent SDK source identity and all three platform packages. It uploads only release archives and SHA256SUMS.txt; SDK artifacts are not republished. Artifact names include run attempts to preserve prior attempts, and existing releases are never overwritten.

Production code-signing, timestamps and macOS notarization need operator-managed identities. A temporary guest certificate is test evidence, not public CA signing or a security certification.
