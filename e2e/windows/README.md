# Signed installation and reboot E2E

This test runs a full Windows guest inside Docker using QEMU/KVM. Linux containers alone cannot prove Windows installation or reboot behavior. It creates no separate Hyper-V VM and does not reboot or change the host certificate store.

~~~mermaid
sequenceDiagram
  participant H as Docker host
  participant W as Windows guest
  participant P as BazUpgrade publisher
  participant C as BazUpgrade client
  H->>W: Verified evaluation ISO + selected tool binaries
  W->>P: Create ephemeral test certificate
  P->>W: Sign MSI and tools with SHA-256
  P->>C: Signed TUF repository / release 1
  C->>W: Real MSI installation / exit 3010
  W->>W: Windows reboot
  W->>C: Inspect native state / explicit reconcile
  P->>C: Release 2 major upgrade
  C->>W: Install / reboot / reconcile
  W->>H: Boot times, journal and version evidence
~~~

## Prerequisites

- Docker Linux engine with /dev/kvm, at least 8 GiB available RAM and space for a 64 GiB sparse guest disk.
- A Windows evaluation ISO from Microsoft, checked against Microsoft's published SHA-256. Supply your own licensed/evaluation image; the repository contains no Windows image.
- Current Windows x64 BazUpgrade tools and SDK from a successful native build.
- Your installed Windows SDK's signtool.exe and its adjacent signing DLL/manifests. These are supplied locally and never committed or packaged here.

The container image is pinned by digest. Networking uses user mode, without privileged mode or NET_ADMIN. The browser console binds only to 127.0.0.1:18006. Only the ISO (read-only), OEM bootstrap (read-only) and fresh synthetic workspace are mounted. Bootstrap selects UTC inside the guest so Docker cold starts do not reinterpret the virtual RTC using a different time zone; use a trustworthy synchronized guest clock before creating certificates or TUF metadata.

## Prepare and run

1. Create a fresh workspace with input/ and evidence/ directories.
2. Copy Guest.ps1, CreateFixtureMsi.ps1, bazupgrade.exe, bazupgrade-publish.exe and bazupgrade-sdk.dll into input/.
3. Add the selected Windows SDK signing files: signtool.exe and its manifest, mssign32.dll, wintrust.dll, appxsip.dll, AppxPackaging.dll, OpcServices.dll, and their Microsoft.Windows.Build signing/Appx manifests. Follow manifest dependencies transitively; copying only signtool.exe is insufficient.
4. Write input/ready.json with guard set to bazupgrade-docker-e2e and nonce set to a new 32-character lowercase hexadecimal GUID.
5. Set BAZUPGRADE_WINDOWS_ISO, BAZUPGRADE_E2E_WORKSPACE and BAZUPGRADE_GUEST_PASSWORD to explicit local paths and an ephemeral random password.
6. Run docker compose -p bazupgrade-e2e-UNIQUE -f e2e/windows/compose.yml up -d.
7. Inspect evidence/YOUR-NONCE/result.json or failure.json and the guest transcript. Preserve these until reviewed. input-binaries.json records the exact pre-signing hashes for comparison with the package you supplied.

Do not bind a home directory, credentials, real signing keys or production installation into the guest. OEM bootstrap copies only the selected input. The guest identity guard requires QEMU, the BazUpgradeE2E hostname and a session nonce before installation or restart is allowed.

## What counts as passing

The fixture is a real Windows Installer database with registry components, product registration, a major-upgrade relationship and ScheduleReboot. The publisher creates a seven-day non-exportable Windows CNG test identity, signs and verifies the MSI/tools with signtool, publishes TUF metadata, and the client invokes the real msiexec.

Passing requires both installed version markers, two distinct Windows boot transitions, increasing Windows kernel boot-event record IDs, pendingReboot surviving restart, explicit successful reconciliation, no replay when applying again and removal of the old MSI product. A clock adjustment, container restart or mocked exit code alone is insufficient.

Only the guest trusts the temporary certificate. This does not establish public CA signing, timestamp-service behavior, actual production application migrations, MSIX/PKG/DEB/RPM E2E or security certification. The exact observed scope is recorded in [validation](../../docs/validation.md).

## Cleanup

After preserving evidence, stop the selected Compose project with docker compose -p YOUR-EXACT-PROJECT -f e2e/windows/compose.yml down. Remove its exact named test volume only when you no longer need the guest. Do not prune unrelated Docker resources. Test keys remain only in the disposable guest disk.
