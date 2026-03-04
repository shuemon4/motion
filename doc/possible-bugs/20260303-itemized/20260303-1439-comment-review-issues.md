# Code Review Issues — /comment run

Files reviewed: `src/logger.cpp`

## Potential Issues

### ff_log(): buffer underflow when FFmpeg emits an empty message
- **File**: `src/logger.cpp`
- **Line**: ~47 (`ff_log()`)
- **Severity**: high
- **Description**: `buff[strlen(buff)-1] = 0` strips the trailing newline by overwriting the last character with `\0`. If `vsnprintf` produces zero output characters (e.g., a format string like `""` or `"%%"` that resolves to an empty string), then `strlen(buff) == 0` and `buff[0 - 1]` = `buff[(size_t)-1]` is an out-of-bounds write — undefined behavior and a potential crash. FFmpeg occasionally calls its log callback with empty or whitespace-only messages.
- **Code snippet**:
  ```cpp
  vsnprintf(buff, sizeof(buff), fmt, vlist);
  buff[strlen(buff)-1] = 0;   // UB if strlen(buff) == 0
  ```

### log_history_init() appends rather than resets on overflow reinit
- **File**: `src/logger.cpp`
- **Line**: ~72–81 (`log_history_init()` and its caller `log_history_add()`)
- **Severity**: medium
- **Description**: `log_history_init()` uses `log_vec.push_back()` unconditionally. At startup this is fine. But when called from `log_history_add()` as an overflow guard (after 50 million messages), `push_back` appends 200 new entries to an already-populated vector instead of resetting it. Each reinit doubles the history buffer size, creating an unbounded memory growth path for any long-running process that logs very heavily.
- **Code snippet**:
  ```cpp
  void cls_log::log_history_init() {
      // ...
      for (indx=0;indx<200;indx++){
          log_vec.push_back(log_item);   // always appends; never clears existing entries
      }
  }
  ```

### write_norm(): potential 1-byte buffer overflow when appending newline
- **File**: `src/logger.cpp`
- **Line**: ~138, ~143 (`write_norm()`)
- **Severity**: medium
- **Description**: `strcpy(msg_full + strlen(msg_full), "\n")` appends a newline to the end of `msg_full`. The `vsnprintf` call in `write_msg()` uses `sizeof(msg_full) - n - 1` as the limit, and `add_errmsg()` may further fill the buffer up to `sizeof(msg_full) - 1` bytes. When the buffer is exactly full (last byte is `\0` at index `sizeof(msg_full)-1`), `strcpy` writes `\n` at index `sizeof(msg_full)-1` and a `\0` at index `sizeof(msg_full)`, which is one byte past the array boundary.
- **Code snippet**:
  ```cpp
  strcpy(msg_full + strlen(msg_full),"\n");   // may write '\0' past end of array
  fputs(msg_full, log_file_ptr);
  ```

### syslog loglvl offset is fragile and undocumented
- **File**: `src/logger.cpp`
- **Line**: ~119 (`write_flood()`), ~142 (`write_norm()`)
- **Severity**: low
- **Description**: `syslog(loglvl-1, ...)` is called with `loglvl-1` to map Motion log levels to syslog priorities. This offset is never explained or documented. If the log level constants change or are reordered, the syslog priority mapping will silently shift. A named mapping table or a `motion_level_to_syslog()` helper would make the relationship explicit.
- **Code snippet**:
  ```cpp
  syslog(loglvl-1, "%s", flood_repeats);   // unexplained -1 offset
  ```
