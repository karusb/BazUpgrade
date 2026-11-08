# Migrating from v1

v2 deliberately replaces the GitHub string parser, substring asset selection, working-directory extraction, and shell launcher.

| v1 | v2 |
|---|---|
| GitHub owner/repository flags | TUF repository and trusted bootstrap |
| Version string comparison | Signed monotonic release sequence |
| First matching asset name | Exact target and signed application/platform policy |
| Download/unzip in current directory | Bounded transfer and immutable version staging |
| Shell launch argument | Host-controlled lifecycle |
| Boolean result | Structured JSON and persistent operation phase |

Existing clients need a new trusted application/updater distribution containing the bootstrap and integration. Changing only the server does not retrofit TUF.

Keep application data outside version directories. A launcher can read the activePath returned by status.

For native installations, preserve package identity and select the corresponding adapter or verification integration. Validate migrations with isolated synthetic applications. Completed installers and user prompts are never replayed as a migration shortcut.
