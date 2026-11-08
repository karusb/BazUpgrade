# Build and install

[Quickstart](quickstart.md) · [Publishing](publishing.md) · [Integrations](integrations.md)

## Choose your integration

| Path | Needs private CurlWrapper source access? |
|---|---|
| Packaged CLI or shared C ABI SDK | No. Compatible runtime libraries may still be needed on Linux/macOS. |
| Build BazUpgrade or link the static C++ SDK | Yes: obtain an authorized CurlWrapper 2 CMake SDK installation. |
| Host a signed repository | Only the publisher binary and protected signing state are required. |

BazUpgrade is public; CurlWrapper remains private. Apache-2.0 for BazUpgrade does not grant access to that dependency. Public source builds are not self-contained. There is no credential-bearing submodule, vendored private snapshot, or automatic private clone.

## Requirements

CMake 3.25+, C++17, OpenSSL 3+, curl 7.85+, libarchive, nlohmann/json 3.12, CurlWrapper 2 and BazPO 0.3. BazPO uses an installed package or a pinned public commit. Python is only needed for behavior/interoperability tests; Node is optional for SDK tests.

Use maintained security-patched packages. API minimums do not imply an old unpatched release is safe.

## 1. Install the private CMake SDK

Clone CurlWrapper using your own authorized access, outside the public project. No credentials belong in CMake, Docker images or repository files.

~~~sh
cmake -S /path/to/CurlWrapper -B /path/to/dependencies/curl-build \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/path/to/dependencies/installed
cmake --build /path/to/dependencies/curl-build --config Release --parallel 4
ctest --test-dir /path/to/dependencies/curl-build -C Release --output-on-failure
cmake --install /path/to/dependencies/curl-build --config Release
export BAZUPGRADE_DEPENDENCY_PREFIX=/path/to/dependencies/installed
~~~

Install curl development libraries first. CurlWrapper's HTTP regression tests use BazPO; that public dependency is pinned by commit.

## 2. Build BazUpgrade

### Ubuntu 24.04 / devcontainer

~~~sh
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build git libssl-dev libcurl4-openssl-dev libarchive-dev python3-venv
cmake --preset dev
cmake --build --preset dev --parallel 4
ctest --preset dev
~~~

The devcontainer sets the SDK prefix to /workspaces/dependencies/installed. Install your authorized SDK there, or override BAZUPGRADE_DEPENDENCY_PREFIX. Container creation does not clone private repositories.

### macOS

~~~sh
brew install cmake ninja openssl@3 curl libarchive
export CMAKE_PREFIX_PATH="$(brew --prefix openssl@3);$(brew --prefix curl);$(brew --prefix libarchive)"
cmake --preset dev
cmake --build --preset dev --parallel 4
ctest --preset dev
~~~

### Windows

Use Visual Studio 2022 or a newer Developer PowerShell with CMake and Ninja. Initialize the committed vcpkg baseline:

~~~powershell
git clone https://github.com/microsoft/vcpkg.git .dependencies/vcpkg
git -C .dependencies/vcpkg checkout 6fdd23fa0a9479a9f9a1c48178696f60cf39255d
.dependencies/vcpkg/bootstrap-vcpkg.bat -disableMetrics
$env:VCPKG_ROOT = "$PWD/.dependencies/vcpkg"
$env:BAZUPGRADE_DEPENDENCY_PREFIX = 'C:/dependencies/installed'
$env:BAZUPGRADE_VCPKG_INSTALLED_DIR = "$PWD/.dependencies/vcpkg_installed"
$native = @("-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake",
  "-DVCPKG_INSTALLED_DIR=$env:BAZUPGRADE_VCPKG_INSTALLED_DIR",
  '-DVCPKG_TARGET_TRIPLET=x64-windows-static',
  '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>')
cmake -S C:/authorized/CurlWrapper -B .dependencies/curl-build -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_INSTALL_PREFIX=$env:BAZUPGRADE_DEPENDENCY_PREFIX" @native
cmake --build .dependencies/curl-build --parallel 4
ctest --test-dir .dependencies/curl-build --output-on-failure
cmake --install .dependencies/curl-build
cmake --preset release @native
cmake --build --preset release --parallel 4
ctest --preset release
~~~

The CI windows preset uses the Visual Studio 2022 generator. The Ninja instructions above also work with Visual Studio 2026. Use the same architecture and runtime for dependencies and consumers.

## 3. Install, package and execute the packages

~~~sh
cmake --install out/release
cpack --preset release
cmake -DPACKAGE_DIR="$PWD/out/packages" \
  -DTOOLS_CONFIG="$PWD/out/release/package-tools-Release.cmake" \
  -P cmake/VerifyPackages.cmake
~~~

Or run configure → build → test → package together with cmake --workflow --preset release, after setting up dependencies.

Package verification runs both packaged executables, loads the packaged C ABI library, checks public SDK/license files and rejects private CurlWrapper source. A CPack archive is not a signed/notarized native installer.

## Consume the installed C++ SDK

~~~cmake
find_package(BazUpgrade 2 CONFIG REQUIRED)
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE BazUpgrade::Core)
~~~

Set CMAKE_PREFIX_PATH to the BazUpgrade SDK, private CurlWrapper SDK and other dependency installations. C ABI consumers can load the shared library without compiling or obtaining the private headers.

## Reference TUF interoperability

~~~sh
python3 -m venv .venv
.venv/bin/pip install -r requirements-tests.txt
cmake --preset dev -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset dev
BAZUPGRADE_REQUIRE_REFERENCE_TUF=1 ctest --preset dev
~~~

On Windows use .venv/Scripts/python.exe. The remaining Python files are test suites for real CLI/C ABI/HTTP behavior and interoperability with the official TUF client. Build, packaging and CI orchestration use CMake and reusable actions.
