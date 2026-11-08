# Architecture

BazUpgrade keeps the original repository's interface/controller separation and replaces the v1 download/unzip/launch chain with an explicit trust and installation pipeline.

~~~mermaid
flowchart TB
    SDK[CLI / C ABI / language SDK] --> Controller[UpdateClient]
    Controller --> TUF[TufClient]
    TUF --> Transport[Transport interface]
    TUF --> Crypto[CryptoProvider interface]
    Controller --> Installer[Installer interface]
    Controller --> State[File locks and durable journal]
    Installer --> Portable[Portable version directories]
    Installer --> Native[OS package installer]
    Transport --> Curl[CurlWrapper]
    Transport --> Directory[Local repository]
~~~

| Directory | Responsibility |
|---|---|
| interfaces | Shared contracts and typed errors. No installer or crypto implementation dependencies. |
| controllers | Application policy, operation ownership, portable and native installation. |
| tuf | Trusted metadata workflow and ordered delegation lookup. |
| crypto | OpenSSL implementation and publisher signing helpers. |
| curl | Repository transport adapting the separately maintained CurlWrapper library. |
| storage | Process-held locks. |
| publishing | Repository lifecycle, signed publication, and compatibility feeds. |
| sdk | C ABI, public C++ include, and Node integration. |
| util | Strict JSON, canonical encoding, bounded reads, durable replacement, safe paths. |

## Trust and scope

A client starts with an explicitly supplied trusted root. Never bootstrap trust by blindly downloading root.json from the same untrusted source as the update.

A selected target must match the locally configured appId, channel, OS, architecture, and package type. Repository metadata cannot introduce executable shell commands or select a different installer backend.

Application release sequence, trusted metadata version, and application display version are distinct. Trusted release sequence is persisted even when a download has not been installed. File rollback keeps that high-water mark.

State is bound to its bootstrap root and local application scope. Installation ownership prevents two configurations from sharing the same managed installation directory.

## Operation ownership

Operations take a state lock and installations additionally take an installation-directory lock. Different applications in different directories remain independent. Locks are held by the process, not represented by a disposable marker that another cancelled task could remove.

The installation journal records downloading, ready, applying, complete, and pendingReboot. A process interrupted while applying has an unknown effect. Subsequent apply calls refuse to replay it until an operator or host application explicitly reconciles the outcome.

Native subprocess ownership is retained until exit is observed. Cancellation during a native installation does not terminate or abandon an installer that may already have changed the system.

## Portable layout

~~~text
installation/
  owner.json
  installation.lock
  versions/
    SHA256/
      application files
state/
  installation.json
  downloads/
  trust/
~~~

The active executable path is returned in installation state. The host application or launcher reads this path; BazUpgrade never runs a shell command supplied by a release.

Version-directory rollback covers application files. It does not reverse database changes or other external effects.

## Extensions

The C++ UpdateClient constructor accepts explicitly owned CryptoProvider, Transport, and Installer implementations. Registration happens in trusted application code. Downloaded metadata cannot load plugins, replace an implementation, or request elevation.

A custom adapter must make its activation, reboot, recovery, and privilege behavior explicit. The core cannot promise atomic rollback for an arbitrary external installer.
