# wf-kill
A way to close a wayfire view by clicking on it. Returns the client PID.

## Build

meson build --prefix=/usr

ninja -C build

sudo ninja -C build install

## Runtime

Enable Wayfire Kill View Protocol plugin 'wf-kill'

Run `wf-kill` and click on a window

## Example

```
$ wf-kill
PID: 1644456
```

## Notes
wf-kill returns the PID or "UNKNOWN" in the case of a remote client surface