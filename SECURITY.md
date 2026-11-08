# Security policy

BazUpgrade v2 is a release candidate. Its TUF-based design and tests do not replace an independent security audit.

For a vulnerability report, use the repository's GitHub private vulnerability reporting feature when available. Do not include real signing keys, access tokens, or private user data in a public issue.

## Security boundaries

- Provision the trusted root through a trusted channel.
- Protect local configuration, trust state, installation directories, publisher keyrings, and signing keys.
- Keep private signing material outside every served directory and release artifact.
- Maintain metadata expiration even when no application version changes.
- Do not bypass a failed signature, expired metadata, or rollback rejection by falling back to an unverified feed.
- A signed release is authorized publisher code. TUF cannot make malicious authorized application code safe.
- Native installers retain their own certificate, package identity, privilege, and reboot requirements.
- Portable extraction rejects links, special files, traversal, duplicate paths, and quota violations.
- Interrupted native installation requires observation and explicit reconciliation.

## Testing

Use generated keys and isolated synthetic applications. Public CI must not depend on private signing credentials or perform privileged upgrades of real applications.

See [TUF profile](docs/tuf-profile.md), [architecture](docs/architecture.md), and [support scope](docs/support.md).
