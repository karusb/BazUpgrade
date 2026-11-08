# Dependencies

| Dependency | Purpose | Resolution |
|---|---|---|
| OpenSSL 3+ | Ed25519, hashes and publisher keys | Security-patched system package or pinned vcpkg baseline |
| curl 7.85+ | HTTPS | System package or pinned vcpkg baseline |
| libarchive | ZIP/TAR | System package or pinned vcpkg baseline |
| nlohmann/json 3.12.0 | JSON model | Installed package or SHA-256 pinned public archive |
| CurlWrapper 2 | Isolated requests, limits and cancellation | Installed private CMake SDK only |
| BazPO 0.3 | Every C++ executable's options | Installed package or immutable public commit |
| Python / official python-tuf | CLI/C ABI/HTTP and interoperability tests | requirements-tests.txt |
| Node 20+ | Optional Node/Electron SDK | No runtime npm dependencies |
| Windows SDK signtool | Optional native Authenticode signing and verification | Explicit absolute operator-selected executable |

CurlWrapper remains private. BazUpgrade never fetches its private source or distributes its private headers. Building from source and using the static C++ SDK require authorized access to that SDK. Distributed binaries and the shared C ABI are separate integration paths.

The full C++ workflow runs inside a private caller repository. Public fork checks only run on public source, without private credentials. Read [development](development.md) before changing this boundary.

BazPO supplies parsing, generated help and option constraints. Scalar tool options use `withMaxValueCount(1)` to reject repeated tags, including aliases. The tools require an explicit value for each provided scalar option before update or publishing operations. BazPO retains raw values, optional values and custom tag syntax; it also owns defaults and supports header-only linkage across translation units.

Unpinned 11Zip, bundled old curl and v1 GoogleTest/gMock build scaffolding are retired from BazUpgrade. Custom Python build/sync/document/package scripts have been removed. Python remains only where the behavior under test requires it.

Mbed TLS is not a shipped backend. CryptoProvider is an extension point; a replacement must implement the selected schemes and pass interoperability tests. A PSA Ed25519 constant alone does not establish actual signing/verifying support.

Minimum versions specify APIs. Use maintained, patched releases and review advisories. System package patch versions vary across runners. Record the actual versions for a tested build.

The pinned vcpkg baseline has not yet adopted libarchive 3.8.11's security fixes. The small [public-source overlay](../cmake/ports/libarchive/README.md) updates that package to 3.8.11 with a fixed SHA-512 while retaining the baseline for other dependencies. Build and SDK consumer configurations share one explicit vcpkg installation directory on Windows.

Dependencies retain their own licenses. Static Windows packages include available vcpkg notices. The BazUpgrade license does not relicense private dependencies.
