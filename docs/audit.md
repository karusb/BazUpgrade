# Audit and adoption decision

This is a maintainer engineering review and remediation record, not an independent security audit. The review starts from BazUpgrade main at 188e1bc and CurlWrapper main at 5981451. The v2 rewrite is intentionally an API-breaking release candidate.

## Was v1 suitable as a shared production updater?

Not as a general trust and installation foundation. V1 provided useful separation between selection, download, extraction and launch, but selecting a GitHub release and downloading its assets did not establish independently signed release authority, metadata freshness or rollback protection. A public repository is a distribution location, not a replacement for update signatures.

| Observed boundary | Consequence | V2 response |
| --- | --- | --- |
| Selection by release ordering and asset filename | A matching asset name is insufficient application or platform authorization | Exact logical target plus signed application, channel, platform, installer and sequence policy |
| Download/unzip pipeline without a TUF trust workflow | Repository compromise and stale metadata were outside the client trust model | Threshold signatures, root rotation, expiration, metadata references and persisted versions |
| Selected ZIP extraction into the working directory | Updating live application files lacks a separate activation and recovery boundary | Managed version directories, bounded extraction, explicit entrypoint and durable installation state |
| A reusable CurlWrapper easy handle and retained request options | POST/GET/download behavior could inherit prior request state | Reset per-request options and borrowed pointers while retaining connections; behavior regressions |
| Ignored curl_slist_append return value | Requested headers were not reliably retained | Owned header lists and allocation checks |
| curl transport success without HTTP status validation | HTTP errors could be treated as successful requests | Typed HTTP errors and explicit optional metadata not-found handling |
| No transfer deadlines or signed download size bound in the old facade | A server could hold an operation or consume unbounded resources | Connect/overall/low-speed limits and streaming size enforcement |
| Old HTTP client imposed a TLS 1.2 maximum | Modern TLS negotiation was unnecessarily constrained | Platform-maintained libcurl TLS negotiation with peer/hostname checks |
| Download/extract/launch success without an operation journal | Interrupted native effects could not be safely inferred | Explicit unknown-outcome reconciliation; no automatic replay |
| Source dependencies and build-only usage | Consumers lacked an installed SDK contract and package validation | Pinned source inputs, exported CMake packages, C ABI, CLI and packaged-binary checks |

## Recommended adoption

Use v2 first for an isolated portable application and a staging repository. Keep signing keys outside the public web root; retain the original client bootstrap through rotations. Use the documented refresh schedule and monitor expiration failures. A static HTTPS server is sufficient; the signing/publishing workstation is a separate trust boundary.

For an existing MSIX/App Installer or Electron deployment, first decide which component owns verification and installation. Native feed exports preserve compatibility but do not make stock native clients enforce TUF. The Electron verification hook must be supported by the chosen updater version. Delta companions require their own authorized targets and are rejected by the provided single-artifact hook.

The default crypto implementation uses OpenSSL. CryptoProvider allows an application to supply an Mbed TLS implementation without changing repository wire format, provided it supports the profile's exact Ed25519 and hash operations. An Mbed TLS backend is not included or claimed as tested.

## Remaining release gates

The later [validation record](validation.md) contains the separate agent review, fixes, actual local checks and native E2E status. It supersedes any implication that configured CI or a generic installer fixture proves real package/reboot behavior.

- Independent review of the TUF client, key lifecycle and filesystem boundaries before broad production rollout.
- Signed real-package installation tests for each native backend, including permissions, reboot and application-specific migrations.
- Target-specific trust-store, deployment and recovery tests. Synthetic native subprocess tests cannot establish real installer behavior.
- A device-specific activation/recovery adapter for firmware. The desktop filesystem engine does not itself implement a firmware bootloader or guarantee recovery after power loss on arbitrary hardware.

The [support matrix](support.md), [TUF profile](tuf-profile.md) and [development guide](development.md) define the shipped scope. CI results establish behavior for their exact commit and runner environment; they do not establish universal updater compatibility.
