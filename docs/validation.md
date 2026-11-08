# Validation evidence

This records observed local checks for the v2 review changes. A configured workflow is not an executed workflow. No live GitHub Actions run, release tag or public release was created; commits use [skip ci].

## Build and behavior

| Check | Observed result |
|---|---|
| Windows x64 / MSVC 19.51 / CMake 3.31.6 | 35 C++ behavior cases passed |
| Windows CLI / C ABI / loopback / official TUF 7.0.1 | 9 integration tests passed; reference interoperability required |
| Linux / GCC 9 / CMake 3.31.6 | 33 C++ behavior cases and 9 integration tests passed |
| Linux reference client | Local Python 3.8 environment used TUF 3.0 compatibility; current TUF 7.0.1 was checked separately on Windows |
| BazPO | 89 regression tests passed, including strict parsing, owned defaults and multiple translation units |
| CurlWrapper | HTTP/cancellation/isolation/concurrency suite passed on Windows and Linux |
| Node SDK | 6 tests passed |
| Installed C++ SDK consumer | Compiled and ran on Windows and Linux |
| CPack ZIP and TGZ | Generated and verified on Windows and Linux; packaged tools executed and C ABI loaded |
| Private-source exclusion | Verified package contains no CurlWrapper source/headers |
| Workflow definitions | actionlint 1.7.12 passed locally |
| Hosted Linux/Windows/macOS matrix | Not run for this change: CI credit constraint |
| Current macOS build / signing / native installation | Not executed locally |

Windows dependencies resolved by the committed vcpkg baseline include OpenSSL 3.6.5, curl 8.23.0, libarchive 3.8.11 and nlohmann/json 3.12.0. The separate Linux prefix used OpenSSL 4.1.0, curl 8.23.0, libarchive 3.8.11 and nlohmann/json 3.12.0. Minimum API requirements differ from these observed versions.

## Instrumentation

Linux C++ coverage: **86.1% lines**, **92.0% functions**, **47.5% branches**. These percentages cover the selected core directories and Linux code paths, not Windows signing/installers, Node, rendering, native reboot or arbitrary target hardware.

The native behavior binary passed AddressSanitizer and UndefinedBehaviorSanitizer with leak detection enabled. clang-analyzer 10 inspected compiled Linux core translation units. It reported one BazPO constructor virtual-call warning: registration calls the concrete base flag implementation during base construction; derived dispatch is not expected there. This older analyzer and a warning review do not establish absence of vulnerabilities.

## Separate agent security review

At the user's request, a second agent reviewed the code independently of implementation rationale. This was an agent review, not an external human audit or certification.

| Confirmed finding | Remediation | Regression evidence |
|---|---|---|
| Windows case aliases could put signing keys under the served repository | Compare filesystem identity for existing key-path ancestors before creating the private directory | Native Windows case-alias rejection passed |
| Served historical metadata could be refreshed into newly signed stale releases | Persist complete signed publication intent outside the web root; compare all mutable roles before signing; explicit exact restoration | Combined and independent role replays rejected; partial public-write recovery passed |

The reviewer inspected the two fixes in source. Additional follow-up review was blocked by the agent tool's security filter and was not completed. Later filesystem, CLI, workflow and signing changes have maintainer review and tests; do not represent them as independently reviewed.

Additional regression work found and fixed fixed-name temporary files blocking explicit restoration, Windows directory sharing/relative-I/O problems, and msiexec switches being overquoted. Windows junction redirection and directory replacement are exercised by synthetic native filesystem tests. The real MSI test exposed the argument quoting bug; generic subprocess fixtures had not detected it.

## Native signed installation and reboot

See the [Docker Windows harness](../e2e/windows/README.md). It uses only a disposable Docker/QEMU guest and a temporary self-signed test certificate. It does not change host trust or reboot the host.

**Passed with the verified Windows release ZIP's exact binaries.** The guest used the publisher to create a temporary CNG certificate, sign/verify both MSI versions and the three tools/SDK files, initialize/rotate/refresh/restore a TUF repository, publish releases, export an Electron compatibility feed, and check/apply/status/reconcile through the real client.

The real Windows Installer installed 1.0.0 and then major-upgraded to 2.0.0. Both installs returned reboot-required state. pendingReboot survived both guest OS restarts; reconciliation followed observed native markers and changed boot times; a subsequent apply returned upToDate. The old MSI product registration was absent after upgrading. Host trust changes: zero.

Observed UTC guest boot times: 2026-11-08T15:39:17.5000000Z → 2026-11-08T15:41:25.5000000Z → 2026-11-08T15:42:40.5000000Z. Windows kernel boot-event record IDs increased 1012 → 1100 → 1186. The guest used UTC; these are Windows boot transitions, not container restarts or clock-only changes. The final run needed no harness intervention. Earlier failed runs exposed and corrected MSI argument quoting, an incomplete fixture Media table, missing SDK signing dependencies and a shared SMB stderr-log lock.

Pre-signing package binary SHA-256:

| Binary | SHA-256 |
|---|---|
| bazupgrade.exe | 8c985700b19d142c76b68dc03cfb556dbaa663a4e3a222ed85d59e6cf4720a43 |
| bazupgrade-publish.exe | 339dc6b747d50e1a81088c722f4a8dd088c125904caf974de888254d1e7ffc08 |
| bazupgrade-sdk.dll | 7a63b776b2cb43ec881418bdfadc04830de1540e9ac2d778a4ac8770cdd240d6 |

All three matched the verified package before guest-only Authenticode signing. The Docker image was pinned to dockurr/windows@sha256:16fd7c39a3e48bb946d29c59dc6015942e5b900a1fa5d7e5552053bfc48ee2ea. The Microsoft evaluation ISO was SHA-256 verified before use; the observed guest kernel was 10.0.26300.0. Certificate material remains in the disposable guest, not in this repository.

MSIX/App Installer, Electron application upgrades, macOS PKG, privileged DEB/RPM and hardware-specific activation require separate target-specific evidence. Native feed exports alone do not enforce TUF.

## Adoption

Use a staging repository and an isolated pilot application first. Preserve the out-of-band trust bootstrap, signing-state backups and application-specific recovery procedure. Keep production signing identities operator-managed. This release candidate is not a universal compatibility or security guarantee.
