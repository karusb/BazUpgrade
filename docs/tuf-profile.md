# BazUpgrade TUF profile

This document records BazUpgrade v2's protocol and operational choices. It is not an independent certification of TUF conformance.

Reference: [TUF specification 1.0.37](https://theupdateframework.github.io/specification/v1.0.37/).

## Wire format

- UTF-8 JSON with duplicate object members rejected.
- TUF/OLPC canonical JSON for signed bytes: sorted object keys, integer numbers, minimal escaping of quote and backslash. RFC 8785 is not substituted.
- Ed25519 metadata signatures with hexadecimal public keys and signatures.
- SHA-256 target hashes are required by this profile; SHA-512 is also verified when present.
- Initial root is provisioned out of band. Root updates follow numbered intermediate roots and both old and new thresholds.
- Root, timestamp, snapshot, and targets expiration are checked against the fixed refresh start time.
- Metadata versions and referenced metadata versions are persisted to reject rollback.
- Changed signed content at an unchanged metadata version is rejected.
- Ordinary ordered delegations, terminating roles, path patterns, and path-hash prefixes are supported.
- Succinct delegations and arbitrary signature algorithms are rejected as unsupported.

## Repository layout

~~~text
metadata/
  1.root.json
  2.root.json
  root.json
  timestamp.json
  N.snapshot.json
  snapshot.json
  N.targets.json
  targets.json
targets/
  SHA256.logical-target.zip
~~~

The publisher uses consistent snapshots. The reader also accepts a repository declaring consistent_snapshot false.

Delegation traversal is bounded by depth and number of visited roles. Metadata reads are bounded to 4 MiB. The client rejects unsafe local path forms and delegated role names that would escape its metadata directory.

## Signed application descriptor

A target's custom object uses schemaVersion 1:

~~~json
{
  "schemaVersion": 1,
  "appId": "example",
  "channel": "stable",
  "os": "windows",
  "arch": "x64",
  "packageType": "portable",
  "appVersion": "1.10.0",
  "releaseSequence": 2,
  "entrypoint": "example.exe",
  "minUpdaterMajor": 2
}
~~~

Release sequence is a positive integer up to 2^53−1. It must increase for each newly published release at a logical target path. Display versions are never ordered lexically.

Application policy is additional to TUF verification. A valid metadata signature does not authorize a target for a different application or installer.

## Operational assumptions

Trusted bootstrap files, local configuration, and writable client state must be protected against untrusted local modification. The client depends on an adequately maintained system clock. Expiration failures do not silently disable verification.

The bootstrap publisher generates three root keys with a two-signature threshold and separate online role keys. Operational separation of those keys remains the operator's responsibility.

See [security policy](../SECURITY.md), [publishing](publishing.md), and [validation scope](support.md).
