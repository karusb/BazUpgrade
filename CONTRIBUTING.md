# Contributing

1. Read the [architecture](docs/architecture.md) and [TUF profile](docs/tuf-profile.md).
2. Build with CMake and run the checks in [development](docs/development.md).
3. Add focused behavior, negative, recovery, or concurrency coverage for changed boundaries.
4. Keep public examples synthetic. Never commit private keys, machine paths, credentials, or user data.
5. Keep README claims aligned with actual capabilities and observed tests.

Use C++17, explicit ownership, small interfaces, Allman braces, and the committed clang-format configuration. Keep cryptographic operations in a reviewed backend. Do not implement cryptographic primitives or a second protocol simulation to make tests pass.

C++ interfaces may accept explicitly injected implementations. Repository metadata must never select arbitrary commands or downloaded plugins.
