# sm

sm is a reimplementation of Nintendo's service manager. It allows Atmosphère to add or remove process handle limits, add new services, or intercept service calls. This allows high-level intercepting of Horizon OS functionality.

## Atmosphère Extensions

There are a few ways in which the Stratosphere implementation of sm differs intentionally from the stock sm.

### IPC: MITM Commands

The Stratosphere implementation of sm adds a few additional commands to the [`sm:`](https://reswitched.github.io/SwIPC/ifaces.html#nn::sm::detail::IUserInterface) port session.

Their SwIPC definitions follow.
```
interface nn::sm::detail::IUserInterface is sm: {
  ...
  [65000] AtmosphereInstallMitm(ServiceName service) -> handle<port, move> service, handle<server_session, move> query;
  [65001] AtmosphereUninstallMitm(ServiceName service);
  [65002] AtmosphereAssociatePidTidForMitm(u64 pid, u64 tid);
}
```

#### AtmosphereInstallMitm

This command alters the registration for the named service, in order to allow services to intercept communication between client processes and their intended services. It is used by [fs_mitm](fs_mitm.md).

It takes the name of the service to install an MITM for, and returns two handles. The first is a port handle, similar to those returned from the [RegisterService](https://reswitched.github.io/SwIPC/ifaces.html#nn::sm::detail::IUserInterface(2)) command. The second is the server side of a session, called the query session. This session will used by sm to determine whether or not a new session should be intercepted, and to inform the MITM service of the identity of new processes.

The query session is expected to implement the following interface.
```
interface MitmQueryService {
  [65000] ShouldMitm(u64 pid) -> u64 should_mitm;
  [65001] AssociatePidTid(u64 pid, u64 tid);
}
```

The `ShouldMitm` command is invoked whenever a process attempts to make a new connection to the MITM'd service. It should return `0` if the process's connection should not be intercepted. Any other value will cause the process's connection to be intercepted. If the command returns an error code, the process's connection will not be intercepted.

The `AssociatePidTid` command is invoked on all MITM query sessions whenever a new process is created, in order to inform those services of the identity of a newly created process before it attempts to connect to any services.

If the process that installed the MITM attempts to connect to the service, it will always connect to the original service.

This command requires that the session be initialized, returning error code 0x415 if it is not.
If the given service name is invalid, error code 0xC15 is returned.
If the user does not have service registration permission for the named service, error code 0x1015 is returned.
If the service has not yet been registered, error code 0xE15 is returned.
If the service already has an MITM installed, error code 0x815 is returned.

#### AtmosphereUninstallMitm

Removes any installed MITM for the named service.

This command requires that the session be initialized, returning error code 0x415 if it is not.

#### AtmosphereAssociatePidTidForMitm

This command is used internally by the Stratosphere implementation of the [loader](loader.md) sysmodule, when a new process is created. It will call the `AssociatePidTid` command on every registered MITM query session.

If the given process ID refers to a kernel internal process, error code 0x1015 is returned. This command requires that the session be initialized, returning error code 0x415 if it is not.

### Minimum Session Limit

When a service is registered, the sysmodule registering it must specify a limit on the number of sessions that are allowed to be active for that service at a time. This is used to ensure that services like `fs-pr`, `fs-ldr`, and `ldr:pm` can only be connected to once, adding an additional layer of safety over the regular service verification to ensure that those services are only connected to by the highly priveleged process they are intended to be used by.

By default, the Stratosphere implementation of PM will raise any session limits to at least 8, meaning that for services like `fs-pr` and those mentioned above, up to 8 processes will be able to connect to those sessions, leaving 7 sessions for homebrew to use.

### Weak Service Verification

In system firmware versions before 3.0.1, if a process did not call the [Initialize](https://reswitched.github.io/SwIPC/ifaces.html#nn::sm::detail::IUserInterface(0)) command on its `sm:` session, normally used to inform sm of the process's identity, sm would assume that the process was a kernel internal process and skip any service registration or access checks. The Stratosphere implementation of sm reimplements this vulnerability, allowing homebrew processes to skip service registration and access checks.
