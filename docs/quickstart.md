# Your first signed update

~~~mermaid
sequenceDiagram
    participant P as Publisher
    participant R as Repository
    participant C as Client
    P->>R: Package and signed metadata
    C->>R: Refresh metadata
    C->>C: Verify trust, scope and sequence
    C->>R: Download target
    C->>C: Verify, stage and activate
~~~

Build using [installation](installation.md). The commands below assume the tools are on PATH.

## 1. Initialize

~~~sh
bazupgrade-publish init --repo ./repository --keys ./private-signing
~~~

Copy repository/metadata/1.root.json to trusted-root.json in your trusted application distribution. Production trust must be provisioned out of band; blindly downloading a root from the update server is not a trust bootstrap.

## 2. Pack and describe

Create an application directory containing hello.txt:

~~~sh
bazupgrade-publish pack --directory ./application --output ./application.zip
~~~

Save release.json:

~~~json
{
  "schemaVersion": 1,
  "appId": "example",
  "channel": "stable",
  "os": "linux",
  "arch": "x64",
  "packageType": "portable",
  "appVersion": "1.0.0",
  "releaseSequence": 1,
  "entrypoint": "hello.txt",
  "minUpdaterMajor": 2
}
~~~

The text-file entrypoint is returned to the host, never executed by this demo.

## 3. Publish

~~~sh
bazupgrade-publish publish --repo ./repository --keys ./private-signing --artifact ./application.zip --target example-stable.zip --descriptor release.json
~~~

## 4. Configure

Save client.json next to trusted-root.json:

~~~json
{
  "schemaVersion": 1,
  "repository": "./repository",
  "trustedRoot": "./trusted-root.json",
  "appId": "example",
  "channel": "stable",
  "os": "linux",
  "arch": "x64",
  "packageType": "portable",
  "target": "example-stable.zip",
  "stateDirectory": "./client-state",
  "installDirectory": "./client-install"
}
~~~

Paths are relative to the configuration file. App, channel, platform, and installer must match signed metadata exactly.

## 5. Update

~~~sh
bazupgrade check --config ./client.json
bazupgrade apply --config ./client.json
bazupgrade status --config ./client.json
~~~

Status returns the active file path. Applying again reports upToDate without replaying the update. Increase releaseSequence for each subsequent release.

For hosting, change repository to an HTTPS base URL. Loopback HTTP requires explicit allowLoopbackHttp true. Continue with [repository operations](publishing.md).
