# DNS.mitm
As of 0.18.0, atmosphère provides a mechanism for redirecting DNS resolution requests.

By default, atmosphère redirects resolution requests for official telemetry servers, redirecting them to a loopback address.

## Hosts files

DNS.mitm can be configured through the usage of a slightly-extended `hosts` file format, which is parsed only once on system startup.

In particular, hosts files parsed by DNS.mitm have the following extensions to the usual format:
+ `*` is treated as a wildcard character, matching any collection of 0 or more characters wherever it occurs in a hostname.
+ `%` is treated as a stand-in for the value of `nsd!environment_identifier`. This is always `lp1`, on production devices.

If multiple entries in a host file match a domain, the last-defined match is used.

Please note that homebrew may trigger a hosts file re-parse by sending the extension IPC command 65000 ("AtmosphereReloadHostsFile") to a connected `sfdnsres` session.

### Hosts file selection

Atmosphère will try to read hosts from the following file paths, in order, stopping once it successfully performs a file read:

+ (emummc only) `/atmosphere/hosts/emummc_%04lx.txt`, formatted with the emummc's id number (see `emummc.ini`).
+ (emummc only) `/atmosphere/hosts/emummc.txt`.
+ (sysmmc only) `/atmosphere/hosts/sysmmc.txt`.
+ `/atmosphere/hosts/default.txt`

If `/atmosphere/hosts/default.txt` does not exist, atmosphère will create it to contain the defaults.

### Atmosphère defaults

By default, atmosphère's default redirections are parsed **in addition to** the contents of the loaded hosts file.

This is equivalent to thinking of the loaded hosts file as having the atmosphère defaults prepended to it.

This setting is considered desirable, because it minimizes the telemetry risks if a user forgets to update a custom hosts file on a system update which changes the telemetry servers.

This behavior can be opted-out from by setting `atmosphere!add_defaults_to_dns_hosts = u8!0x0` in `system_settings.ini`.

The current default redirections are:

```
# Nintendo telemetry servers
127.0.0.1 receive-%.dg.srv.nintendo.net receive-%.er.srv.nintendo.net
```

## Debugging

On startup (or on hosts file re-parse), DNS.mitm will log both what hosts file it selected and the contents of all redirections it parses to `/atmosphere/logs/dns_mitm_startup.log`.

In addition, if the user sets `atmosphere!enable_dns_mitm_debug_log = u8!0x1` in `system_settings.ini`, DNS.mitm will log all requests to GetHostByName/GetAddrInfo to `/atmosphere/logs/dns_mitm_debug.log`. All redirections will be noted when they occur.

## Opting-out of DNS.mitm entirely
If you wish to disable DNS.mitm entirely, `system_settings.ini` can be edited to set `atmosphere!enable_dns_mitm = u8!0x0`.
