# pm

pm is a reimplementation of Nintendo's process manager. This module is responsible for tracking running processes on the system, and managing resource limits. pm is also required to create and manage processes for homebrew applications.

## Atmosphère Extensions

There are a few ways in which the Stratosphere implementation of pm differs intentionally from the stock pm.

### IPC: AtmosphereGetProcessHandle

The Stratosphere implementation of pm adds an additional command to the [`pm:dmnt`](https://reswitched.github.io/SwIPC/ifaces.html#nn::pm::detail::IDebugMonitorInterface) interface, called `AtmosphereGetProcessHandle`. Its command ID is `65000` on all system firmware versions. It takes a `u64 process_id` and returns a process handle for the specified process, if that process is known. Notable exceptions include KIPs, which are not known to pm. If the specified process cannot be found, error code 0x20F is returned.

The SwIPC definition for this command follows.
```
interface nn::pm::detail::IDebugMonitorInterface is pm:dmnt {
  ...
  [65000] AtmosphereGetProcessHandle(u64 pid) -> handle<copy, process> process_handle;
}
```

### Extra System Memory for Sysmodules

The Stratosphere implementation of pm shrinks the APPLET memory pool by 24 MiB by default, giving this memory to the SYSTEM pool. This allows custom sysmodules to use more memory without hitting the SYSTEM memory limit.
