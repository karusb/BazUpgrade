# Support and validation scope

A compiled adapter is not evidence that every third-party installer behaves correctly. Native package signatures, application identity, permissions, reboot behavior, and installer-specific effects remain platform concerns.

| Path | Implementation | Automated evidence |
|---|---|---|
| Portable ZIP/TAR | Safe extraction, quotas, version directories, state activation | Synthetic package lifecycle and negative filesystem tests |
| Local and HTTPS repository | Directory transport and CurlWrapper | Loopback HTTP, failure, cancellation, and bounded transfer tests |
| TUF | Ed25519 profile, thresholds, metadata persistence, root rotation, ordered delegation | Cryptographic vectors, negative behavior tests, reference-client interoperability |
| C ABI | Opaque client, owned strings, cancellation | ctypes integration |
| Node SDK | Structured subprocess protocol and cancellation ownership | Node built-in test suite |
| Electron hook | TUF file verification before native installation | SDK contract tests; real packaged Electron upgrade requires application-specific validation |
| MSI | Windows Installer invocation, 3010 reboot state, explicit reconciliation | Real self-signed MSI 1→2 major upgrade and two Windows guest reboots; exact package binaries checked |
| MSIX / NSIS | Windows native invocation | Native compilation; synthetic NSIS subprocess/log contract; real signed MSIX/NSIS E2E remains separate |
| DEB / RPM | Native package manager invocation | Cross-platform compilation; privileged package installation is separate |
| PKG | macOS installer invocation | Cross-platform compilation; signed package installation is separate |
| Custom devices | C++ injection points | Host-supplied adapter tests required |

## Capabilities

Run bazupgrade capabilities on the actual binary. The response lists installers available in that build and explicitly reports verification-only integrations, lack of automatic elevation, lack of universal native rollback, and lack of a built-in firmware flasher.

## Release candidate boundaries

- A separate agent review identified two confirmed issues and reviewed their fixes in source. This is not an external human audit or certification; see [validation](validation.md).
- No automatic database rollback, privilege broker, or arbitrary command-runner plugin is shipped.
- Root rotation is implemented; online role-key rotation requires a separately reviewed operational workflow.
- A native feed does not enforce TUF unless the client is integrated with BazUpgrade verification.
- Native installer E2E must use isolated synthetic applications. Never test by replaying a real user's installer or prompts.

Observed checks for a specific commit belong in its PR/release evidence. This matrix describes test categories and does not assert that a future commit's CI has passed.
