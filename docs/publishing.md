# Repository operations

[Quickstart](quickstart.md) creates the first release.

## Keep keys separate

~~~text
private-signing/          NEVER serve or package this directory
  keyring.json
  trusted-root.json
  publication.json       protected signed publication checkpoint
  root-*.pem
  targets-*.pem
  snapshot-*.pem
  timestamp-*.pem
repository/              the only web root
  metadata/
  targets/
~~~

The publisher rejects a key directory inside the served repository. Protect directory permissions and Windows ACLs.

Move root private keys offline after initialization. Routine publish/refresh uses online role keys; root rotation requires the root keys in an isolated signing environment.

| Command | Purpose |
|---|---|
| init | Create separate roles and keys; root requires two of three keys |
| pack | Create a portable ZIP from a selected directory |
| publish | Publish a verified copy and signed application descriptor |
| refresh | Renew metadata with increasing versions |
| restore-metadata | Restore exact online metadata from the protected checkpoint |
| rotate-root | Sign an intermediate root with old and new root thresholds |
| export-electron | Explicit unsigned compatibility feed |
| export-appinstaller | App Installer XML for a signed MSIX descriptor |
| create-test-certificate | Windows-only seven-day CNG test code-signing identity |
| sign-windows | Sign and verify with an explicitly selected Windows SDK signtool |

## Host and refresh

A static HTTPS server serves downloads. See [nginx example](../examples/server/nginx.conf).

~~~sh
bazupgrade-publish refresh --repo /srv/bazupgrade --keys /var/lib/bazupgrade-signing
~~~

Metadata refresh is needed even without a new application release. The default lifetimes are one year for root, 30 days for targets, seven days for snapshot, and one day for timestamp.

The [service](../examples/server/bazupgrade-refresh.service) and [timer](../examples/server/bazupgrade-refresh.timer) schedule refresh. Configure accounts and paths before enabling them. Monitor failures and root expiry.

When syncing to another host, upload artifacts and numbered metadata first; timestamp.json goes last. Keep historical public roots and consistent snapshot files. Use one signing keyring to serialize publication to a repository.

GitHub Pages, object storage, or a private web server can host the static layout. An arbitrary GitHub REST release response is not a TUF repository.

## Rotate root

~~~sh
bazupgrade-publish rotate-root --repo ./repository --keys ./private-signing
~~~

Existing clients retain their original bootstrap and walk the intermediate roots. Old private files remain for deliberate offline archival. This rotates root-role keys; online role-key rotation needs a separately reviewed workflow.

## Recovery

Inspect interrupted publisher work before retrying. A leftover .next file or unreferenced numbered metadata is not permission to overwrite keys or reset version counters.

Each publication commits complete signed targets, snapshot and timestamp envelopes to private-signing/publication.json before writing the public metadata. The publisher compares every mutable online role against that protected checkpoint before signing. An attacker who can rewrite the served repository cannot turn a historical release into fresh signed metadata through routine refresh.

If public writes fail after the checkpoint advances, ordinary publish/refresh refuses the inconsistent served state. Inspect the repository, remove the cause of failure, then explicitly restore:

~~~sh
bazupgrade-publish restore-metadata --repo ./repository --keys ./private-signing
bazupgrade-publish refresh --repo ./repository --keys ./private-signing
~~~

Restoration verifies signatures and metadata references and republishes the exact checkpoint. It does not re-sign, extend expiration or reset versions. Expired restored metadata stays expired until an authorized refresh. It does not recover artifacts, root-rotation history or missing private keys. Back up the entire protected directory and public root history separately.

The combined signed checkpoint has a 4 MiB quota. This is a publisher repository-size limit; large target sets may need a future reviewed delegated-publishing design.

Repositories created before publication.json existed require migration from a trusted signing-state backup or deliberate offline reconstruction. A missing checkpoint fails closed. The tool never adopts the currently served metadata as trusted signing state automatically.

For Windows signing, see the [guest-only E2E guide](../e2e/windows/README.md). create-test-certificate exports only the public certificate and never adds trust automatically. sign-windows selects CurrentUser/My by explicit thumbprint, uses SHA-256, optionally timestamps over an explicitly selected HTTPS URL, and requires native trust verification to succeed. Protect and review the selected signtool binary; production certificates remain operator-managed.

For clients:

~~~sh
bazupgrade status --config client.json
bazupgrade reconcile --config client.json --installed false
~~~

Use installed true only after observing successful native installation or completing a pending reboot. A failed external installer may already have changed the system.

Portable reconciliation as not installed discards unactivated staging and partial downloads. It never automatically replays an installer.
