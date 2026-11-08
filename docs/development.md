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

## CI boundary

~~~mermaid
flowchart LR
  P[Public BazUpgrade PR] --> C[Formatting and Node tests]
  R[Private CurlWrapper manual build] --> W[Public reusable build.yml]
  W --> D[Configure / Build / Test private SDK]
  D --> B[Configure / Build / CTest BazUpgrade]
  B --> I[Install / Compile consumer]
  I --> A[CPack / Execute packaged tools / Probe C ABI]
  A --> F[Private artifacts]
  F --> M[Explicit release workflow]
  M --> Q[Draft public release]
~~~

The public reusable workflow can run only in a private caller context. Private source never belongs in a public workflow checkout, log or artifact. The companion CurlWrapper bazupgrade.yml workflow invokes it using the caller's read-only GITHUB_TOKEN. All refs are immutable commit SHAs; only invoke reviewed code. Fork pull requests receive no private credentials.

Merge the reviewed manual entry workflows into their default branches before using their dispatch buttons. GitHub requires the workflow file on the default branch for workflow_dispatch; see [GitHub's trigger documentation](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#workflow_dispatch). A feature-branch delivery does not silently merge or activate those workflows.

The visible steps use the same CMake commands as local builds. There is no custom Python CI runner. The private CurlWrapper CMake CI also has separate configure/build/test/install steps.

No live Actions runs were requested for this change: CI credits were unavailable. Commits use [skip ci]. Local results are recorded separately from unexecuted runner configuration in [validation](validation.md).

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

1. Run the private BazUpgrade integration workflow for an exact reviewed BazUpgrade SHA and CurlWrapper SHA. Inspect all three platform results and package verification.
2. Complete the required native E2E and security review for the intended audience. Review [validation](validation.md) and [support](support.md).
3. Create an immutable tag matching VERSION.txt at the tested BazUpgrade SHA.
4. Manually run the public release workflow with that tag, SHA and the successful private integration run ID.
5. Inspect and publish the resulting draft release.

CURLWRAPPER_ARTIFACT_READ_TOKEN is a fine-grained read token for Actions artifacts in the private CurlWrapper repository. It is used only by the manual release workflow. The release checks run success, workflow path/event, source identity and all three platform artifacts, then uploads only release archives and SHA256SUMS.txt. Existing releases are never overwritten.

Production code-signing, timestamps and macOS notarization need operator-managed identities. A temporary guest certificate is test evidence, not public CA signing or a security certification.
