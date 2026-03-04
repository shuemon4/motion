# Code Review Issues — /comment run

Files reviewed: `src/conf.cpp`

## Potential Issues

### 1. movie_retain parameter has no handler in dispatch_edit
- **File**: `src/conf.cpp`
- **Line**: ~190 (config_parms definition) and ~680-970 (dispatch_edit)
- **Severity**: medium
- **Description**: `movie_retain` is defined in `config_parms[]` as `PARM_TYP_LIST` (line 190) but has no corresponding entry in `dispatch_edit()`. When the config file parser encounters this parameter, it routes through `edit_set` → `edit_set_active` → `edit_cat` → `edit_cat10` → `dispatch_edit`, which falls through without matching. The value is silently discarded — never stored, never retrievable. This means `movie_retain` in config files has no effect.
- **Code snippet**:
  ```cpp
  // In config_parms[] (line 190):
  {"movie_retain",              PARM_TYP_LIST,   PARM_CAT_10, PARM_LEVEL_LIMITED,  true},

  // No corresponding handler in dispatch_edit() for "movie_retain"
  ```

### 2. stream_max_connections dispatched but not in config_parms
- **File**: `src/conf.cpp`
- **Line**: ~750 (dispatch_edit)
- **Severity**: low
- **Description**: `dispatch_edit()` has a handler for `stream_max_connections` (line 750), but this parameter is not listed in the `config_parms[]` array. The handler exists but is unreachable via normal config file parsing or API calls, since `edit_set_active()` uses the registry (built from config_parms) for lookup. This is either an orphaned handler from a removed parameter, or a missing `config_parms[]` entry.
- **Code snippet**:
  ```cpp
  // In dispatch_edit (line 750):
  if (name == "stream_max_connections") return edit_generic_int(stream_max_connections, parm, pact, 10, 0, 100);

  // Not present in config_parms[] array
  ```

### 3. Dead code after return statements in multiple handlers
- **File**: `src/conf.cpp`
- **Lines**: 445-446, 490-491, 527-528, 551-552, 571-572, 584-585, 604-605, 624-625, 644-645, 664-665, 678-679
- **Severity**: low
- **Description**: Multiple edit handler functions contain `MOTION_LOG(DBG, ...)` calls immediately after `return;` statements, making them unreachable. These appear to be historical debug traces that were disabled by inserting `return` above them. They add dead code to every affected function and can confuse static analysis tools.
- **Code snippet**:
  ```cpp
  // Example from edit_log_file (lines 444-446):
      return;
      MOTION_LOG(DBG, TYPE_ALL, NO_ERRNO,"%s:%s","log_file",_("log_file"));
  }
  ```
