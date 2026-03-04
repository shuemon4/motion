# Medium Severity Bugs

Issues that cause incorrect behavior, resource leaks, security weaknesses, or silent functional failures under specific conditions.

## Bug Status

| # | Bug | Status |
|---|-----|--------|
| 1 | movie_retain no dispatch handler | ✅ Resolved |
| 2 | PostgreSQL conn string not escaped | ✅ Resolved |
| 3 | draw.cpp negative array index | ✅ Resolved |
| 4 | JSON \uXXXX not handled | ✅ Resolved |
| 5 | log_history_init unbounded growth | ✅ Resolved |
| 6 | write_norm 1-byte overflow | ✅ Resolved |
| 7 | passthru_streams uninit retcd | ✅ Resolved |
| 8 | SIGVTALRM thread termination | ⬜ Not a bug (by design) |
| 9 | cleandir_cam unknown freq runaway | ✅ Resolved |
| 10 | ALSA hw_params leak | ✅ Resolved |
| 11 | thumbnail false success retcd | ✅ Resolved |
| 12 | thumbnail partial file on disk | ✅ Resolved |
| 13 | vlp_open_vidpipe fd leak | ✅ Resolved |
| 14 | vlp_startpipe fd leak | ✅ Resolved |
| 15 | is_trusted_proxy CIDR comment | ✅ Resolved |
| 16 | clients_mtx held during exec | ✅ Resolved |

---

## 1. conf.cpp — movie_retain parameter has no handler in dispatch_edit

- **File**: `src/conf.cpp`
- **Line**: ~190 (config_parms), ~694–983 (dispatch_edit)
- **Description**: `movie_retain` is defined in `config_parms[]` as `PARM_TYP_LIST` but has no corresponding entry in `dispatch_edit()`. The config file parser routes through `dispatch_edit` which falls through without matching. The value is silently discarded — never stored, never retrievable. `movie_retain` in config files has no effect.
- **Code snippet**:
  ```cpp
  {"movie_retain", PARM_TYP_LIST, PARM_CAT_10, PARM_LEVEL_LIMITED, true},
  // No corresponding handler in dispatch_edit() for "movie_retain"
  ```

### Assessment: CONFIRMED BUG

**Verdict**: Real bug. The parameter is defined in `config_parms[]` (line 190), has a backing member `std::string movie_retain` in `parm_structs.hpp:255`, an alias in `conf.hpp:248`, and is actively used in `movie.cpp:1676` (`cam->cfg->movie_retain == "secondary"`). But `dispatch_edit()` (lines 694–983) has no handler for it, so the config value from the file is never stored into the member. The `movie_retain` setting silently does nothing.

**Impact**: Users who configure `movie_retain = secondary` (to auto-delete recordings without secondary detection) get no retention filtering. All recordings are kept regardless of the setting.

**Upstream**: No matching upstream issue found. The `movie_retain` parameter may be new to this fork.

**Fix**: Add a list handler in `dispatch_edit()`:
```cpp
static const std::vector<std::string> movie_retain_values = {"all","secondary"};
if (name == "movie_retain") return edit_generic_list(movie_retain, parm, pact, "all", movie_retain_values);
```

---

## 2. dbse.cpp — PostgreSQL connection string values not escaped

- **File**: `src/dbse.cpp`
- **Line**: ~1013–1017
- **Description**: `pgsqldb_init()` builds a libpq connection string by concatenating config values inside single quotes without escaping. If `database_password`, `database_dbname`, `database_host`, or `database_user` contains a single quote, the connection string will be malformed and the connection will fail. Should use `PQconnectdbParams()` or escape quotes in the values.
- **Code snippet**:
  ```cpp
  constr = "dbname='" + app->cfg->database_dbname + "' ";
  constr += " host='" + app->cfg->database_host + "' ";
  constr += " user='" + app->cfg->database_user + "' ";
  constr += " password='" + app->cfg->database_password + "' ";
  ```

### Assessment: CONFIRMED BUG (low practical risk)

**Verdict**: Real bug. A single quote in any database credential (especially passwords like `P@ss'w0rd`) breaks the connection string syntax. This is not SQL injection — libpq's `PQconnectdb()` parses the connection string internally and a broken string simply fails to connect, producing a confusing error message. The correct fix is `PQconnectdbParams()` which takes separate key/value arrays and handles escaping internally.

**Impact**: Users with single quotes in their PostgreSQL credentials get a cryptic connection failure. Not a security vulnerability since this is a local config file, not user-supplied input.

**Upstream**: No upstream issues found. PostgreSQL support is rarely used and seldom tested.

**Fix**: Use `PQconnectdbParams()` instead of string concatenation, or escape single quotes by doubling them (`'` → `''`).

---

## 3. draw.cpp — Negative ASCII index in textn() — out-of-bounds array access

- **File**: `src/draw.cpp`
- **Line**: ~1118–1120
- **Description**: `pos_check = (int)text[pos]` on platforms where `char` is signed produces a negative value for characters > 127 (extended ASCII, UTF-8 multi-byte sequences). `char_arr_ptr[pos_check]` is then an out-of-bounds negative-index access — undefined behavior, potential crash or memory corruption. The guard `if (pos_check < 0)` inside the inner loop catches this for the re-read, but the initial assignment `char_ptr = char_arr_ptr[pos_check]` at the outer loop start is reached before any guard.
- **Code snippet**:
  ```cpp
  int pos_check = (int)text[pos];
  char_ptr = char_arr_ptr[pos_check];   // UB if pos_check < 0
  for (y = 0; y < 8 * factor; y++) {
      for (x = 0; x < 7 * factor; x++) {
          if (pos_check < 0) {           // guard is too late
  ```

### Assessment: CONFIRMED BUG

**Verdict**: Real bug. `char_arr_ptr` is declared as `u_char *char_arr_ptr[ASCII_MAX]` where `ASCII_MAX = 127` (draw.hpp:30). Valid indices are 0–126. On ARM (Raspberry Pi), `char` is unsigned by default, so `(int)text[pos]` for values 128–255 gives positive values 128–255, which are still out-of-bounds (>126). On x86 (where `char` is signed), values 128–255 become -128 to -1, also out-of-bounds. Either way, line 1120 is UB for any non-ASCII byte.

The guard at line 1125 is inside the inner loop, but the first dereference at line 1120 happens once per character before entering the loops. The guard never protects the initial access.

**Impact**: Any UTF-8 text or extended-ASCII character in `text_left`/`text_right` config causes UB. Common trigger: locale-dependent timestamps, user names with accents. On Pi (unsigned char), reading `char_arr_ptr[200]` reads 73 indices past the end of the array.

**Upstream**: No upstream issue found for this specific bug.

**Fix**: Cast to `unsigned char` and clamp: `int pos_check = (unsigned char)text[pos]; if (pos_check >= ASCII_MAX) pos_check = ' ';`

---

## 4. json_parse.cpp — \uXXXX Unicode escape sequences not handled

- **File**: `src/json_parse.cpp`
- **Line**: ~218–220
- **Description**: The `parseString()` switch handles standard escapes but falls through to `setError("Invalid escape sequence")` for `\uXXXX`. Any JSON from a browser or other RFC 8259-compliant producer that contains `\uXXXX` (e.g., `\u0022`) will fail to parse and produce an error. This is a silent incompatibility with the JSON spec.
- **Code snippet**:
  ```cpp
  case 't':  result += '\t'; break;
  default:
      setError("Invalid escape sequence");   // rejects \uXXXX
      return "";
  ```

### Assessment: CONFIRMED BUG (low practical risk)

**Verdict**: Real spec-compliance bug. RFC 8259 requires `\uXXXX` support. The parser rejects valid JSON containing unicode escapes. However, this is in the fork's custom JSON parser (used by the web UI for config changes), not upstream code.

**Impact**: Low in practice. The web UI sends config parameter names and values which are typically ASCII. Browsers only produce `\uXXXX` when the JSON contains non-ASCII characters. The most likely trigger would be a password or filename with non-ASCII characters set via the web UI.

**Upstream**: N/A — `json_parse.cpp` is fork-specific code.

**Fix**: Add a `case 'u':` handler that reads 4 hex digits and converts to UTF-8.

---

## 5. logger.cpp — log_history_init() appends rather than resets on overflow reinit

- **File**: `src/logger.cpp`
- **Line**: ~80–83, ~98–99
- **Description**: `log_history_init()` calls `push_back()` unconditionally. When called from `log_history_add()` as an overflow guard (after 50 million messages), it appends 200 new entries to an already-populated vector instead of resetting it. Each reinit doubles the history buffer size, creating unbounded memory growth for any long-running process under heavy logging.
- **Code snippet**:
  ```cpp
  for (indx=0;indx<200;indx++){
      log_vec.push_back(log_item);   // always appends; never clears existing entries
  }
  ```

### Assessment: CONFIRMED BUG (negligible practical impact)

**Verdict**: Technically correct — `log_history_init()` appends without clearing, so after an overflow reset the vector grows by 200 entries. However, the practical impact is negligible:

- Overflow threshold: 50,000,000 messages
- At 10 messages/second: ~58 days per overflow
- Growth per overflow: 200 entries × ~50 bytes ≈ 10 KB
- After 1 year continuous operation: ~6 overflows × 10 KB = 60 KB leaked

The vector also never shrinks, but the total leak over realistic lifetimes is trivially small.

**Impact**: Negligible memory growth. Not a meaningful issue for any real deployment.

**Upstream**: No upstream issue found.

**Fix**: Add `log_vec.clear()` before the push_back loop, or use assignment instead of append.

---

## 6. logger.cpp — write_norm(): potential 1-byte buffer overflow when appending newline

- **File**: `src/logger.cpp`
- **Line**: ~144, ~149
- **Description**: `strcpy(msg_full + strlen(msg_full), "\n")` appends a newline after `vsnprintf` has filled the buffer with `sizeof(msg_full) - n - 1` bytes and `add_errmsg()` may further fill up to `sizeof(msg_full) - 1` bytes. When the buffer is exactly full, `strcpy` writes `\n` at the last valid index and `\0` one byte past the array boundary.
- **Code snippet**:
  ```cpp
  strcpy(msg_full + strlen(msg_full),"\n");   // may write '\0' past end of array
  ```

### Assessment: CONFIRMED BUG (extremely narrow edge case)

**Verdict**: Real but requires a very specific alignment. Analysis:

1. `vsnprintf` uses `sizeof(msg_full) - n - 1`, reserving 1 byte. Max strlen after vsnprintf: 1022.
2. If `NO_ERRNO`: strlen stays ≤ 1022. `strcpy` at 1022 writes `\n` at 1022, `\0` at 1023. Buffer is 1024 bytes (0–1023). **No overflow.**
3. If `SHOW_ERRNO` and `add_errmsg()` runs without truncation (total < 1024): max strlen = 1023. `strcpy` at 1023 writes `\n` at 1023, `\0` at 1024. **Index 1024 is one past the end — 1-byte overflow.**

The overflow requires: SHOW_ERRNO + message fills buffer to exactly 1023 bytes after error append. This is a narrow but real edge case.

**Upstream**: Issue #1862 reported a closely related buffer overflow crash in the v4.x logger. The v5.0 rewrite addressed the specific crash path but this `write_norm` pattern remains.

**Fix**: Reserve space for the newline: use `sizeof(msg_full) - n - 2` in vsnprintf, or check `strlen(msg_full) < sizeof(msg_full) - 2` before appending.

---

## 7. movie.cpp — Uninitialized retcd in passthru_streams() for non-video/audio streams

- **File**: `src/movie.cpp`
- **Line**: ~1306–1324
- **Description**: If a stream in the netcam transfer format is neither `AVMEDIA_TYPE_VIDEO` nor `AVMEDIA_TYPE_AUDIO` (e.g., subtitle or data), the if/else-if block is skipped and `retcd` retains its uninitialized stack value. The subsequent `if (retcd < 0)` check reads undefined behavior and may cause a spurious early return.
- **Code snippet**:
  ```cpp
  int retcd, indx;  // retcd uninitialized
  ...
  if (stream_in->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      retcd = passthru_streams_video(stream_in);
  } else if (stream_in->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      retcd = passthru_streams_audio(stream_in);
  }
  // If neither, retcd is uninitialized here
  if (retcd < 0) { ... }
  ```

### Assessment: CONFIRMED BUG (very low practical risk)

**Verdict**: Technically UB on the first loop iteration if the first stream is a non-audio/non-video type. On subsequent iterations, retcd retains the previous iteration's value (stale but defined). In practice, RTSP cameras virtually never have subtitle or data streams as their first stream — video is always stream 0. The check would use a stale-but-valid retcd from a previous iteration, or uninitialized memory on the first (extremely unlikely) iteration.

**Impact**: Near-zero. RTSP cameras expose video and sometimes audio streams. Subtitle/data streams in RTSP are essentially non-existent.

**Upstream**: No upstream issue found.

**Fix**: Initialize `retcd = 0;` at declaration, or add `retcd = 0;` before the if/else-if block inside the loop.

---

## 8. netcam.cpp — handler_shutdown(): pthread_kill with SIGVTALRM may not terminate thread

- **File**: `src/netcam.cpp`
- **Line**: ~2286
- **Description**: When both watchdog timeouts expire, `pthread_kill(handler_thread, SIGVTALRM)` is used as a last-resort termination. `SIGVTALRM` is not a termination signal — its effect depends on whether the process has a signal handler installed. The thread may continue running, leaving mutexes locked and dangling state. Using `pthread_cancel` with appropriate cancellation points would be safer.
- **Code snippet**:
  ```cpp
  MOTION_LOG(ERR, TYPE_ALL, NO_ERRNO, _("Memory leaks will occur."));
  pthread_kill(handler_thread, SIGVTALRM);   // may not terminate the thread
  ```

### Assessment: NOT A BUG (by design)

**Verdict**: The bug report's core claim — that SIGVTALRM "may not terminate the thread" — is **incorrect**. Motion explicitly installs a SIGVTALRM handler in `setup_signals()` (motion.cpp:119–121) that calls `pthread_exit(NULL)` (motion.cpp:70–71). This is an intentional design pattern: SIGVTALRM is registered without `SA_RESTART` (line 120) specifically to interrupt blocking ioctls and terminate threads. The pattern is used consistently across 7 files (netcam.cpp, camera.cpp, sound.cpp, alg_sec.cpp, schedule.cpp, allcam.cpp, dbse.cpp).

**However**, there are two legitimate secondary concerns:
1. `pthread_exit()` is not async-signal-safe per POSIX (though it works in practice on Linux/glibc)
2. Mutexes held by the thread at signal time are indeed left locked (the code comments acknowledge this with "Memory leaks will occur")

These are known, accepted tradeoffs for a last-resort watchdog kill after all graceful shutdown attempts have failed.

**Upstream**: Issue #366 reported a related problem with `pthread_kill()` — but about using `pthread_kill(thread_id, 0)` to probe thread state after cancellation (different issue). The SIGVTALRM approach was upstream's intentional replacement.

**Fix**: None needed. This is working as designed. The alternatives (`pthread_cancel` + cleanup handlers, or `exit(1)`) have their own tradeoffs.

---

## 9. schedule.cpp — Unknown freq value causes cleanup to run every 30 seconds in cleandir_cam()

- **File**: `src/schedule.cpp`
- **Line**: ~253–259
- **Description**: After running cleanup, `next_ts` is only advanced for "hourly", "daily", or "weekly". Any other `freq` value (empty string, misconfigured value) leaves `next_ts` unchanged. Since `cleandir_cam` triggers whenever `curr_ts >= next_ts`, cleanup runs on every scheduling tick (every 30 seconds) indefinitely, producing excessive DB queries and filesystem operations.
- **Code snippet**:
  ```cpp
  if (p_cam->cleandir->freq == "hourly") {
      p_cam->cleandir->next_ts.tv_sec += (60 * 60);
  } else if (p_cam->cleandir->freq == "daily") {
      p_cam->cleandir->next_ts.tv_sec += (60 * 60 * 24);
  } else if (p_cam->cleandir->freq == "weekly") {
      p_cam->cleandir->next_ts.tv_sec += (60 * 60 * 24 * 7);
  }
  // No else: next_ts unchanged → cleanup repeats every 30 s
  ```

### Assessment: CONFIRMED BUG (depends on config validation)

**Verdict**: The code logic is correct — a missing `else` clause means an unrecognized `freq` value causes `next_ts` to never advance, triggering cleanup every tick. The question is whether invalid `freq` values can reach this code.

The `cleandir_params` config is a freeform string parsed by the schedule module. If the freq field is not validated during parsing (or if the parser defaults to an empty string on malformed input), this becomes a real runaway-cleanup bug. Without seeing the parser, the defensive coding principle says this should have an `else` clause.

**Impact**: Excessive filesystem scans and database queries every 30 seconds if an invalid freq value slips through. High CPU usage on Pi — directly violates the project's CPU efficiency principle.

**Upstream**: No upstream issue found. The cleandir feature appears minimally tested.

**Fix**: Add an else clause that either logs a warning and defaults to "daily", or disables cleanup entirely.

---

## 10. sound.cpp — alsa_start leaks hw_params on every early-return error path

- **File**: `src/sound.cpp`
- **Line**: ~365–479
- **Description**: `hw_params` is allocated with `snd_pcm_hw_params_malloc` at ~line 365. It is only freed at ~line 479 after all parameters are successfully set. Nine `if (retcd < 0)` error paths that follow the allocation (lines 375–477) all set `device_status = STATUS_CLOSED` and return without calling `snd_pcm_hw_params_free`. Each device open failure leaks the allocation.
- **Code snippet**:
  ```cpp
  retcd = snd_pcm_hw_params_malloc(&hw_params);  // allocated here
  // ... many error paths return without calling snd_pcm_hw_params_free(hw_params)
  snd_pcm_hw_params_free(hw_params);              // only reached on full success
  ```

### Assessment: CONFIRMED BUG

**Verdict**: Real resource leak. After `snd_pcm_hw_params_malloc()` succeeds at line 365, there are 9 error paths (lines 375, 386, 396, 406, 416, 426, 435, 444, 454, 464) that return without calling `snd_pcm_hw_params_free(hw_params)`. The free at line 479 is only reached on the full-success path.

The leak occurs on every failed ALSA device initialization. If the sound device has a transient issue (wrong format, wrong rate), each retry attempt leaks the allocation.

**Impact**: Small per-leak (ALSA hw_params is a small struct), but accumulates if the device fails repeatedly. Also, `snd_pcm_close()` is not called on the error paths after `snd_pcm_open()` succeeded (line 355), leaking the PCM handle as well.

**Upstream**: No upstream issue found.

**Fix**: Use a goto-cleanup pattern or add `snd_pcm_hw_params_free(hw_params)` to all error paths after the malloc.

---

## 11. thumbnail.cpp — encode_thumbnail returns non-negative retcd on failure paths

- **File**: `src/thumbnail.cpp`
- **Line**: ~382–450
- **Description**: After `sws_scale` succeeds, `retcd` holds a positive line count. Three later failure paths (`jpgutl_put_yuv420p` returning ≤ 0, `fopen` returning nullptr, `fwrite` short-write) all `goto cleanup` without resetting `retcd` to a negative value. `generate()` then sees `retcd >= 0` and logs "Generated thumbnail" even though no file was written.
- **Code snippet**:
  ```cpp
  retcd = sws_scale(...);          // retcd = positive line count on success
  if (retcd < 0) { goto cleanup; }
  ...
  if (jpg_size <= 0) {
      goto cleanup;                // retcd still >= 0 — false success reported
  }
  retcd = 0;  // only reached on full success
  ```

### Assessment: CONFIRMED BUG

**Verdict**: Real bug. The caller `generate()` (line 158–168) checks `if (retcd < 0)` and logs "Generated thumbnail" otherwise. When `sws_scale` succeeds (retcd = positive line count) but JPEG encoding or file I/O fails, the positive retcd propagates through the goto-cleanup path, and the caller reports false success.

The failure paths at lines 409 (jpg_size ≤ 0), 416 (fopen fails), and 422 (fwrite short-write) all jump to cleanup without setting retcd to a negative value.

**Impact**: False "Generated thumbnail" log messages. The UI may show a broken thumbnail link. Combined with bug #12 below, the partial file prevents future regeneration.

**Upstream**: N/A — thumbnail.cpp is fork-specific code.

**Fix**: Set `retcd = -1` before each `goto cleanup` on the failure paths, or set `retcd = -1` at the top of cleanup and only set `retcd = 0` on the explicit success path.

---

## 12. thumbnail.cpp — Partial/empty thumbnail file left on disk after write failure

- **File**: `src/thumbnail.cpp`
- **Line**: ~415–426, 430–433
- **Description**: If `fopen` succeeds but `fwrite` fails (e.g., disk full), the file is closed in the cleanup block but not removed. On the next run, `exists()` finds the stale file via `stat()` and returns true, permanently skipping regeneration. The thumbnail is silently missing from the UI with no recovery path short of manually deleting the `.thumb.jpg` file.
- **Code snippet**:
  ```cpp
  f = fopen(thumb_path.c_str(), "wb");  // file created here
  if (f == nullptr) { goto cleanup; }
  if (fwrite(jpg_buffer, 1, (size_t)jpg_size, f) != (size_t)jpg_size) {
      goto cleanup;  // file exists on disk but is empty/partial, never removed
  }
  ```

### Assessment: CONFIRMED BUG

**Verdict**: Real bug. The `fopen("wb")` creates/truncates the file on disk. If `fwrite` fails (disk full, I/O error), the cleanup block closes the file but never `remove()`s it. The `exists()` check (via `stat()`) at the top of `generate()` finds the stale file on the next call and skips regeneration permanently.

**Impact**: A single disk-full event during thumbnail generation permanently disables thumbnails for that video. The only recovery is manual file deletion. Combined with bug #11 (false success reporting), the user gets no indication that anything went wrong.

**Upstream**: N/A — thumbnail.cpp is fork-specific code.

**Fix**: In the cleanup block, if `retcd != 0` and `f != nullptr`, call `remove(thumb_path.c_str())` to clean up the partial file.

---

## 13. video_loopback.cpp — fd leak in vlp_open_vidpipe() on success path

- **File**: `src/video_loopback.cpp`
- **Line**: ~109, ~143–149, ~152
- **Description**: When the video pipe `tfd` open succeeds, `break` exits the loop without closing the sysfs name file descriptor `fd`. `close(fd)` is only reached when the `tfd` open fails. Every successful auto-detection leaks one fd.
- **Code snippet**:
  ```cpp
  if ((tfd = open(buffer, O_RDWR|O_CLOEXEC)) >= 0) {
      pipe_fd = tfd;
      break;     // fd (sysfs name file) never closed here
  }
  close(fd);    // only reached when tfd open failed
  ```

### Assessment: CONFIRMED BUG

**Verdict**: Real fd leak. When `tfd = open()` succeeds at line 143, the code breaks out of the while loop at line 149. The `close(fd)` at line 152 is structurally outside the `if (fd = open(...))` block (closed by the brace at line 151), so it is unreachable after the break. The sysfs name file descriptor `fd` (opened at line 109) is never closed on the success path.

Additionally, `close(fd)` at line 152 sits outside the `if (fd >= 0)` check, so it would also be called with an invalid fd on iterations where the sysfs open failed — harmless (EBADF) but incorrect.

**Impact**: One fd leak per successful loopback pipe initialization. Since loopback is typically set up once per camera, this leaks one fd per camera at startup. Low severity but real.

**Upstream**: No upstream issue found.

**Fix**: Add `close(fd);` before the `break` at line 149.

---

## 14. video_loopback.cpp — dev fd leaked on ioctl failures in vlp_startpipe()

- **File**: `src/video_loopback.cpp`
- **Line**: ~223–256
- **Description**: Three early-return error paths after `dev` is opened do not call `close(dev)` before returning -1: `VIDIOC_QUERYCAP` failure, `VIDIOC_G_FMT` failure, and `VIDIOC_S_FMT` failure. Each failure leaks the open device fd.
- **Code snippet**:
  ```cpp
  if (ioctl(dev, VIDIOC_QUERYCAP, &vc) == -1) {
      return -1;   // dev not closed
  }
  if (ioctl(dev, VIDIOC_G_FMT, &v) == -1) {
      return -1;   // dev not closed
  }
  if (ioctl(dev, VIDIOC_S_FMT, &v) == -1) {
      return -1;   // dev not closed
  }
  ```

### Assessment: CONFIRMED BUG

**Verdict**: Real fd leak. `dev` is opened at line 210 or 212 (via `vlp_open_vidpipe()` or direct `open()`). Three subsequent ioctl error paths (lines 223–225, 235–237, 253–255) all return -1 without calling `close(dev)`.

**Impact**: One fd leak per ioctl failure during pipe initialization. Since ioctl failures are uncommon in normal operation (only if the V4L2 loopback device is misconfigured), this is low frequency. But each leak is a permanent fd until process exit.

**Upstream**: No upstream issue found.

**Fix**: Add `close(dev);` before each `return -1` after the `dev` open.

---

## 15. webu_ans.cpp — is_trusted_proxy() claims CIDR support but only does exact IP match

- **File**: `src/webu_ans.cpp`
- **Line**: ~272–273 (comment), ~289 (implementation)
- **Description**: The function's doc comment says "Supports comma-separated list of IPs or CIDR ranges", but the implementation only compares strings with `==`. CIDR notation like `192.168.1.0/24` never matches any real IP. A misconfigured `trusted_proxies` entry using CIDR notation silently fails with no warning, and the comment creates a false expectation of CIDR support.
- **Code snippet**:
  ```cpp
  // Supports comma-separated list of IPs or CIDR ranges  ← claims CIDR support
  if (trusted == ip) {   // exact string match only, no CIDR parsing
  ```

### Assessment: MISLEADING DOCUMENTATION, NOT A BUG

**Verdict**: The comment is inaccurate, but the code works correctly for its actual feature set (exact IP matching). This is a documentation issue, not a functional bug. The code does exactly what it implements — comma-separated exact IP matching — and does it correctly.

CIDR support was never implemented; the comment describes aspirational functionality. Since `webu_ans.cpp` was rewritten in this fork, this is likely a comment copied from a design spec that wasn't fully implemented.

**Impact**: A user who reads the comment and configures `trusted_proxies = 192.168.1.0/24` gets no proxy trust and X-Forwarded-For headers are ignored. The failure is silent.

**Upstream**: N/A — webu code was fully rewritten in this fork.

**Fix**: Either implement CIDR parsing or correct the comment to say "Supports comma-separated list of IPs".

---

## 16. webu_ans.cpp — clients_mtx held while calling util_exec_command() in failauth_check()

- **File**: `src/webu_ans.cpp`
- **Line**: ~550 (lock), ~571 (exec call)
- **Description**: `failauth_check()` holds `webu->clients_mtx` for the entire function including the call to `util_exec_command()`, which spawns an external lock script. If the script is slow (firewall rule insertion, network lookup), all threads calling `client_connect()` or `failauth_log()` block for the duration. This degrades responsiveness for legitimate clients during a brute-force attack.
- **Code snippet**:
  ```cpp
  std::lock_guard<std::mutex> lock(webu->clients_mtx);  // held for full function
  ...
  util_exec_command(cam, tmp.c_str(), NULL);  // external process while mutex held
  ```

### Assessment: CONFIRMED DESIGN ISSUE (not a correctness bug)

**Verdict**: This is a valid performance concern, not a correctness bug. The mutex is legitimately needed to safely iterate `wb_clients` and access `it->userid_fail_nbr`. The external command (`util_exec_command`) runs while the mutex is held, which could stall all web connections if the script is slow.

However, this only triggers when an attacker exceeds `webcontrol_lock_attempts` failed logins, which is already an exceptional condition. And `util_exec_command` typically spawns the command asynchronously (via `system()` + `&` or similar), so the hold time may be minimal.

**Impact**: Under active brute-force attack, legitimate clients experience brief stalls while the lock script runs. Low practical impact since the lock script path is rarely executed.

**Upstream**: N/A — webu code was fully rewritten in this fork.

**Fix**: Copy the needed data (script command, userid_fail_nbr, clientip), release the mutex, then execute the command. Or spawn the command asynchronously.

---

## Summary

| # | Bug | Confirmed? | Severity | In Upstream? |
|---|-----|-----------|----------|-------------|
| 1 | movie_retain no dispatch handler | **YES** | Medium — feature silently broken | Unknown |
| 2 | PostgreSQL conn string not escaped | **YES** (low risk) | Low — fails to connect, not injectable | Yes |
| 3 | draw.cpp negative array index | **YES** | Medium — UB on non-ASCII text overlay | Yes |
| 4 | JSON \uXXXX not handled | **YES** (low risk) | Low — spec non-compliance, rare trigger | Fork-specific |
| 5 | log_history_init unbounded growth | **YES** (negligible) | Negligible — ~10KB per 58 days | Yes |
| 6 | write_norm 1-byte overflow | **YES** (narrow edge) | Low — requires exact buffer fill + errno | Yes (related #1862) |
| 7 | passthru_streams uninit retcd | **YES** (very low risk) | Low — UB only with exotic stream types | Yes |
| 8 | SIGVTALRM thread termination | **NO** — by design | N/A — working as intended | Yes (designed) |
| 9 | cleandir_cam unknown freq runaway | **YES** (conditional) | Medium — if invalid freq reaches code | Yes |
| 10 | ALSA hw_params leak | **YES** | Medium — leaks on every device open failure | Yes |
| 11 | thumbnail false success retcd | **YES** | Medium — false "Generated" log + broken UI | Fork-specific |
| 12 | thumbnail partial file on disk | **YES** | Medium — permanently blocks regeneration | Fork-specific |
| 13 | vlp_open_vidpipe fd leak on success | **YES** | Low — one fd per camera at startup | Yes |
| 14 | vlp_startpipe fd leak on ioctl fail | **YES** | Low — one fd per ioctl failure | Yes |
| 15 | is_trusted_proxy CIDR comment | **NO** — misleading comment | Low — documentation issue only | Fork-specific |
| 16 | clients_mtx held during exec | **Design issue** | Low — stalls only under active attack | Fork-specific |

**15 of 16 reports confirmed** as real issues. Bug #8 (SIGVTALRM) is **not a bug** — Motion intentionally installs a handler that calls `pthread_exit()`. Bug #15 is a misleading comment, not a functional bug.

---

## Resolution Summary

**Date Resolved:** 2026-03-03

### Bug #1: movie_retain no dispatch handler
**Root Cause:** `movie_retain` defined in `config_parms[]` but missing handler in `dispatch_edit()`.
**Fix:** Added `edit_generic_list` handler with valid values `{"all","secondary"}`, default `"all"`.
**File:** `src/conf.cpp` — added 2 lines after `movie_passthrough` handler.

### Bug #2: PostgreSQL connection string not escaped
**Root Cause:** `PQconnectdb()` connection string values not escaped for single quotes/backslashes.
**Fix:** Added lambda `pq_escape()` to escape `'` and `\` with backslash prefix per libpq syntax.
**File:** `src/dbse.cpp` — added escape helper in `pgsqldb_init()`.

### Bug #3: draw.cpp negative/OOB array index
**Root Cause:** `(int)text[pos]` produces values outside 0-126 range for non-ASCII bytes. Array indexed before guard check.
**Fix:** Cast to `(unsigned char)`, clamp `>= ASCII_MAX` to space character, removed ineffective inner-loop guard.
**File:** `src/draw.cpp` — modified `textn()` character loop.

### Bug #4: JSON \uXXXX not handled
**Root Cause:** `parseString()` switch had no `case 'u':` handler, rejecting valid RFC 8259 JSON.
**Fix:** Added full `\uXXXX` handler with UTF-8 encoding and surrogate pair support.
**File:** `src/json_parse.cpp` — added `case 'u':` block in escape switch.

### Bug #5: log_history_init unbounded growth
**Root Cause:** `log_history_init()` appends 200 entries via `push_back()` without clearing existing entries on overflow reinit.
**Fix:** Added `log_vec.clear()` before the push_back loop.
**File:** `src/logger.cpp` — 1 line added.

### Bug #6: write_norm 1-byte buffer overflow
**Root Cause:** `strcpy(msg_full + strlen(msg_full), "\n")` can write `\0` one byte past buffer end when `add_errmsg()` fills to 1023 bytes.
**Fix:** Replaced `strcpy` with bounds-checked direct assignment: only append `\n\0` if `strlen < sizeof(msg_full) - 2`.
**File:** `src/logger.cpp` — replaced both `strcpy` calls in `write_norm()`.

### Bug #7: passthru_streams uninitialized retcd
**Root Cause:** `retcd` declared without initialization; if first stream is neither video nor audio, `if (retcd < 0)` reads uninitialized value.
**Fix:** Initialized `retcd = 0` at declaration.
**File:** `src/movie.cpp` — 1 character change.

### Bug #9: cleandir_cam unknown freq runaway
**Root Cause:** Missing `else` clause after hourly/daily/weekly check leaves `next_ts` unchanged for unrecognized freq values, causing cleanup every 30 seconds.
**Fix:** Added `else` clause that logs a warning and defaults to daily interval.
**File:** `src/schedule.cpp` — added 4 lines.

### Bug #10: ALSA hw_params leak on error paths
**Root Cause:** 10 error paths after `snd_pcm_hw_params_malloc()` returned without freeing `hw_params` or closing `pcm_dev`.
**Fix:** Converted to goto-cleanup pattern. Error label frees `hw_params`, closes `pcm_dev`, nulls pointer. Special case for malloc failure (no hw_params to free).
**File:** `src/sound.cpp` — restructured `alsa_start()` error handling.

### Bug #11: thumbnail false success retcd
**Root Cause:** After `sws_scale` success (positive retcd), three later failure paths jumped to cleanup without resetting retcd to negative. Caller reported false "Generated thumbnail".
**Fix:** Added `retcd = -1` before each `goto cleanup` on the JPEG encode, fopen, and fwrite failure paths.
**File:** `src/thumbnail.cpp` — 3 lines added.

### Bug #12: thumbnail partial file on disk
**Root Cause:** `fopen("wb")` creates file, but `fwrite` failure leaves it on disk. `exists()` finds the stale file and permanently skips regeneration.
**Fix:** In cleanup block, after closing file, if `retcd != 0` call `remove(thumb_path)` to clean up partial/empty files.
**File:** `src/thumbnail.cpp` — 3 lines added in cleanup block.

### Bug #13: vlp_open_vidpipe fd leak on success
**Root Cause:** When `tfd = open()` succeeds, `break` exits loop without closing the sysfs name fd.
**Fix:** Added `close(fd)` before the `break` statement.
**File:** `src/video_loopback.cpp` — 1 line added.

### Bug #14: vlp_startpipe fd leak on ioctl failures
**Root Cause:** Three ioctl error paths return -1 without closing the opened `dev` fd.
**Fix:** Added `close(dev)` before each `return -1`.
**File:** `src/video_loopback.cpp` — 3 lines added.

### Bug #15: is_trusted_proxy misleading CIDR comment
**Root Cause:** Comment claims "Supports comma-separated list of IPs or CIDR ranges" but code only does exact IP match.
**Fix:** Corrected comment to "Supports comma-separated list of exact IP addresses."
**File:** `src/webu_ans.cpp` — comment updated.

### Bug #16: clients_mtx held during exec
**Root Cause:** `lock_guard` held for entire `failauth_check()` including `util_exec_command()` call. Slow scripts block all web connections.
**Fix:** Restructured to use scoped lock block. Mutex is released before `util_exec_command()` runs. Command string and lock state copied inside lock scope.
**File:** `src/webu_ans.cpp` — restructured `failauth_check()`.

### Files Modified

| File | Bugs Fixed |
|------|-----------|
| `src/conf.cpp` | #1 |
| `src/dbse.cpp` | #2 |
| `src/draw.cpp` | #3 |
| `src/json_parse.cpp` | #4 |
| `src/logger.cpp` | #5, #6 |
| `src/movie.cpp` | #7 |
| `src/schedule.cpp` | #9 |
| `src/sound.cpp` | #10 |
| `src/thumbnail.cpp` | #11, #12 |
| `src/video_loopback.cpp` | #13, #14 |
| `src/webu_ans.cpp` | #15, #16 |

**Status:** ✅ All 15 confirmed bugs resolved. Bug #8 not a bug (by design).
