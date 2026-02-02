# Configurable System Commands

**Date**: 2026-02-02
**Branch**: legacy-compat

## Problem

The reboot, shutdown, and service-restart commands in `webu_json_system.cpp` were hardcoded. Different deployments may need different commands (e.g., non-systemd init systems, custom scripts, containerized environments). Additionally, the `system()` calls had unchecked return values, producing compiler warnings.

## Solution

Added 3 new configuration parameters that follow the existing `on_*` script pattern:

| Parameter | Default | Purpose |
|-----------|---------|---------|
| `on_reboot` | `""` | Command to reboot the device |
| `on_shutdown` | `""` | Command to shut down the device |
| `on_service_restart` | `""` | Command to restart the Motion service |

When empty (default), the existing fallback command chains are used unchanged. When set, the configured command runs instead.

Properties:
- **Category**: PARM_CAT_08 (Scripts)
- **Level**: PARM_LEVEL_RESTRICTED
- **Hot reload**: disabled (security-sensitive, requires Motion restart)
- **Scope**: `ctx_parm_app` (application-level, not per-camera)

## Files Modified

| File | Change |
|------|--------|
| `src/parm_structs.hpp` | Added 3 strings to `ctx_parm_app` |
| `src/conf.hpp` | Added reference aliases in `cls_config` |
| `src/conf.cpp` | Registered params in `config_parms[]` + dispatch handlers |
| `src/webu_json_system.cpp` | Use config values with fallback; check all `system()` return values |
| `data/motion-dist.conf.in` | Documented new params in Scripts section |

## Design Decisions

- **App-level, not per-camera**: System commands affect the whole device, so they belong in `ctx_parm_app` rather than `ctx_parm_cam`.
- **No hot reload**: Changing system commands at runtime is a security risk. Requires Motion restart.
- **Config string captured by value into thread**: The detached `std::thread` lambdas capture the command string by value to avoid dangling references if config is modified.
- **Return value checking on all `system()` calls**: Eliminates 3 compiler warnings from the original code.
