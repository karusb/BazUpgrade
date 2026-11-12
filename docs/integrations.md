# Application integrations

## C++

~~~cpp
#include <bazupgrade/BazUpgrade.hpp>
baz::UpdateClient updater(baz::ClientConfig::Load("client.json"));
baz::Cancellation cancellation;
auto result = updater.CheckForUpdate(cancellation);
~~~

| Operation | C++ client method |
|---|---|
| Check the trusted repository for an update | `CheckForUpdate` |
| Download, verify and install an update | `ApplyUpdate` |
| Read the persisted installation state | `ReadInstallationState` |
| Verify a downloaded artifact against trusted metadata | `VerifyDownloadedFile` |
| Record an observed interrupted installation outcome | `ReconcileInstallationOutcome` |
| Select the previous portable installation | `RollbackPortableInstallation` |

These names replace the shorter v2 development API names (`Check`, `Apply`, `Status`, `VerifyFile`, `Reconcile`, `Rollback`). CLI commands and C ABI entry points remain compatible.

The constructor also accepts owned CryptoProvider, Transport, and Installer implementations. Registration occurs in trusted application code. Metadata cannot load a plugin or supply a command line.

A custom installer must define its activation, reboot, privilege, and recovery behavior. The core cannot promise arbitrary external installers have atomic rollback.

## C ABI

Include bazupgrade.h and load the SDK library.

1. baz_client_create loads trusted local configuration.
2. baz_client_run returns allocated JSON.
3. baz_string_free releases each result or error.
4. baz_client_cancel requests cancellation.
5. baz_client_destroy runs after outstanding operations return.

One handle supports one active operation. Cancellation is the supported concurrent call. Do not destroy a handle while another thread uses it.

## Node / Electron

Use the [main-process SDK](../sdk/node/README.md). Renderer IPC must not accept arbitrary executable paths, configuration paths, or updater commands.

Self-update readiness is reported after download and verification. Save application state and quit through the normal application lifecycle; the helper waits for process exit.

For existing electron-updater applications, use packageType electron and attach a compatible downloaded-file verification hook. Native signature checks and native version/install handling remain active. Companion web-installer packages are rejected by the initial hook.

## Native feed compatibility

| Mode | TUF verification | Installation |
|---|---|---|
| BazUpgrade adapter | C++ core | Portable engine or native installer |
| Electron verification hook | C++ core verifies the selected downloaded file | electron-updater |
| Plain App Installer export | Native client does not verify BazUpgrade TUF | Windows App Installer |
| Plain Electron export | Native client does not verify BazUpgrade TUF | Electron updater |

### Electron

~~~sh
bazupgrade-publish export-electron --repo repository --keys private-signing --target example.exe --base-url https://updates.example.com/ --output latest.yml --unsigned-native-feed true
~~~

The output is JSON-compatible YAML with version, URL, length, and SHA-512. It is explicitly unsigned and does not implement electron-builder's newer signed-manifest format. Use the official builder signing pipeline when required. Do not disable native verification to consume this export.

### MSIX / App Installer

Add nativeIdentity to the signed descriptor:

~~~json
{
  "nativeIdentity": {
    "name": "Example.App",
    "publisher": "CN=Example Publisher",
    "version": "1.0.0.0",
    "processorArchitecture": "x64"
  }
}
~~~

~~~sh
bazupgrade-publish export-appinstaller --repo repository --keys private-signing --target example.msix --base-url https://updates.example.com/ --output example.appinstaller
~~~

Upload the XML at its output filename under that base URL. MSIX identity and device certificate trust still apply. Dependencies subsequently resolved by Windows remain under native trust unless separately covered by an application-specific signed plan.

## Privileges and results

BazUpgrade does not elevate itself. Native adapters use fixed command lines and current permissions. Keep configuration and state protected, particularly when embedding the engine in a privileged manager. Native rollback and database rollback are not advertised.
