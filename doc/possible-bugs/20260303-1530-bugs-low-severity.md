# Low Severity Bugs

Issues that are minor functional problems, code quality issues, dead code, undocumented behavior, or minor inefficiencies with limited real-world impact.

---

## 1. allcam.cpp — getimg_src increments all_cnct but never decrements it

- **File**: `src/allcam.cpp`
- **Line**: ~91–93
- **Description**: When waiting for `img_data` to become non-null, `strm_c->all_cnct` is incremented (if currently 0) to signal the camera thread there is a consumer. However, `getimg_src` never decrements `all_cnct` on return — success or timeout. The underlying camera stream's connection counter remains ≥ 1, preventing the camera from idling that stream type. Whether this is intentional (always-on for all-camera view) is unclear.
- **Code snippet**:
  ```cpp
  if (strm_c->all_cnct == 0){
      strm_c->all_cnct++;   // incremented here, never decremented
  }
  ```

### Assessment: LIKELY BY DESIGN (minor concern)

**Verdict**: The `if (all_cnct == 0) all_cnct++` pattern acts as a sticky flag — once the all-camera view requests a stream type, the camera keeps generating it. The actual stream viewer connection lifecycle (connect/disconnect) is managed in `webu_stream.cpp` via `cls_webu_ans::deinit_counter()`. The `all_cnct` here is a separate mechanism that signals "the all-camera mosaic has been activated for this stream."

The concern about preventing idling is valid: once the all-camera view is loaded even once, individual cameras keep generating that stream type forever. However, this may be intentional to avoid cold-start latency when the mosaic is re-loaded. The overhead is minimal since cameras already generate stream data for their direct MJPEG endpoints.

**Impact**: Minor — cameras keep generating one extra stream type after first all-camera view load. Negligible CPU impact.

**Upstream**: No upstream issue found.

**Fix**: If the always-on behavior is undesired, decrement `all_cnct` in the all-camera stream's disconnect handler. Otherwise, add a comment explaining the intentional sticky behavior.

---

## 2. allcam.cpp — getsizes_scale casts integer division result to float losing precision

- **File**: `src/allcam.cpp`
- **Line**: ~287
- **Description**: `(int)((float)(mx_h*100 / p_cam->all_sizes.src_h))` performs integer division before the float cast. Any fractional component is truncated, so the scale is systematically floor-rounded. For example, mx_h=720 / src_h=1080 yields 66 instead of 66.67. The `(float)` cast is a no-op since the value is already an integer. This may cause cameras to appear slightly smaller than intended.
- **Code snippet**:
  ```cpp
  p_cam->all_loc.scale = (int)((float)(mx_h*100 / p_cam->all_sizes.src_h));
  // Should be: (int)((float)(mx_h * 100) / p_cam->all_sizes.src_h)
  ```

### Assessment: NOT A BUG (code style issue only)

**Verdict**: The `(float)` cast is indeed a no-op since it's applied to an already-computed integer. However, the final result is identical either way. Even with proper float division (`(float)(mx_h * 100) / src_h`), the `(int)` outer cast truncates to the same value. For mx_h=720, src_h=1080: integer path gives 66, float path gives (int)66.67 = 66. Same result.

This is because `(int)` truncation on positive values produces the same result as integer division. The "slightly smaller" claim is wrong — the difference is sub-pixel (<1%) and truncation is actually the correct behavior for pixel-based scaling.

**Impact**: None. The `(float)` cast is cosmetic dead code.

**Fix**: Remove the useless `(float)` cast for clarity: `p_cam->all_loc.scale = mx_h * 100 / p_cam->all_sizes.src_h;`

---

## 3. allcam.cpp — stream_free and stream_alloc use a magic number for stream slot count

- **File**: `src/allcam.cpp`
- **Line**: ~214, ~238
- **Description**: Both functions hard-code `indx < 5` for the five stream slots. If a new stream type is added to `ctx_stream`, this magic number must be updated in both places and a new `if (indx == N)` branch added. There is no compile-time guarantee they stay in sync.
- **Code snippet**:
  ```cpp
  for (indx=0;indx<5;indx++) {   // magic number — must match stream type count
  ```

### Assessment: CODE QUALITY ISSUE (not a bug)

**Verdict**: Valid maintainability concern. The five stream types (norm, motion, secondary, source, sub) are accessed via an index-to-pointer mapping with a hard-coded loop count. This is a common C/C++ pattern that relies on developer discipline to keep in sync. It works correctly today.

**Impact**: None currently. Future risk if stream types are added/removed.

**Fix**: Define a constant `STREAM_TYPE_COUNT = 5` or use an array/vector instead of the if/else chain.

---

## 4. conf.cpp — stream_max_connections dispatched but not in config_parms

- **File**: `src/conf.cpp`
- **Line**: ~763 (dispatch_edit), absent from config_parms[] (~230–242)
- **Description**: `dispatch_edit()` has a handler for `stream_max_connections`, but this parameter is not listed in `config_parms[]`. The handler is unreachable via normal config file parsing or API calls. This is either an orphaned handler from a removed parameter or a missing `config_parms[]` entry.
- **Code snippet**:
  ```cpp
  if (name == "stream_max_connections") return edit_generic_int(...);
  // Not present in config_parms[] array
  ```

### Assessment: CONFIRMED BUG (minor)

**Verdict**: Real inconsistency. The member `stream_max_connections` exists in `parm_structs.hpp:281`, a handler exists in `dispatch_edit()` (line 763, default=10), and the value is used in `webu_stream.cpp`. But it's missing from `config_parms[]`, which means:

- Cannot be set via config file (config parser validates against `config_parms[]`)
- The web API's GET/SET for config values also uses `config_parms[]` for enumeration
- The handler default (10) is applied when dispatch_edit is called with PARM_ACT_DFLT

The parameter is functional (hardcoded default works), but users cannot configure it.

**Impact**: `stream_max_connections` is stuck at the default value (10). Users who need more or fewer concurrent stream connections cannot change it.

**Upstream**: No upstream issue found. This appears to be a regression from the config refactor.

**Fix**: Add `{"stream_max_connections", PARM_TYP_INT, PARM_CAT_14, PARM_LEVEL_LIMITED, true}` to `config_parms[]`.

---

## 5. conf.cpp — Dead code after return statements in multiple handlers

- **File**: `src/conf.cpp`
- **Lines**: 446–447, 490–491, 527–528, 551–552, 571–572, 584–585, 604–605, 624–625, 644–645, 664–665, 678–679
- **Description**: Multiple edit handler functions contain `MOTION_LOG(DBG, ...)` calls immediately after `return;` statements, making them unreachable. These appear to be historical debug traces disabled by inserting `return` above them. They add dead code and can confuse static analysis tools.
- **Code snippet**:
  ```cpp
  return;
  MOTION_LOG(DBG, TYPE_ALL, NO_ERRNO,"%s:%s","log_file",_("log_file"));  // unreachable
  ```

### Assessment: CONFIRMED (dead code)

**Verdict**: Verified at line 446–447. The `return;` on line 446 makes the MOTION_LOG on line 447 unreachable. This pattern repeats across 11 handler functions. These are debug logging statements that were disabled by inserting `return` above them rather than deleting or commenting them out.

**Impact**: None functional. Noise for static analysis tools (GCC `-Wunreachable-code`, Clang, etc.).

**Fix**: Delete the unreachable MOTION_LOG lines.

---

## 6. dbse.cpp — CLOCK_MONOTONIC passed to localtime_r in handler()

- **File**: `src/dbse.cpp`
- **Line**: ~1409–1411
- **Description**: `handler()` uses `CLOCK_MONOTONIC` to get the current time, then passes `ts2.tv_sec` to `localtime_r()`. `CLOCK_MONOTONIC` returns seconds since an arbitrary epoch (typically system boot), while `localtime_r` expects seconds since the Unix epoch. The resulting `tm_hour` will not represent the actual wall-clock hour. Should use `CLOCK_REALTIME` or `time()`.
- **Code snippet**:
  ```cpp
  clock_gettime(CLOCK_MONOTONIC, &ts2);
  localtime_r(&ts2.tv_sec, &lcl_tm);
  hr_cur = lcl_tm.tm_hour;
  ```

### Assessment: CONFIRMED BUG (functionally benign)

**Verdict**: Real bug — `CLOCK_MONOTONIC` returns seconds since boot, not since the Unix epoch, so `localtime_r` produces a meaningless calendar time. However, the code only uses `hr_cur = lcl_tm.tm_hour` to detect "hour changed" via `hr_cur != hr_prev`. Since `tm_hour` still wraps through 0–23 every 24 hours of monotonic time, the hour comparison changes every 3600 real seconds, so `dbse_clean()` still runs approximately once per hour.

The cleanup doesn't run at wall-clock hour boundaries (e.g., exactly at 3:00 AM) — it runs at "hours since boot" boundaries. For a periodic cleanup task, this doesn't matter.

**Impact**: Cleanup runs once per hour correctly, just not aligned to wall-clock hours. No functional harm.

**Upstream**: No upstream issue found.

**Fix**: Change to `clock_gettime(CLOCK_REALTIME, &ts2)` or `time_t now = time(NULL); localtime_r(&now, &lcl_tm);`

---

## 7. dbse.cpp — mysql_init return value not checked

- **File**: `src/dbse.cpp`
- **Line**: ~730–731
- **Description**: In `mariadb_init()`, `mysql_init()` is called on a `mymalloc`-allocated struct but its return value is not checked. `mysql_init()` can return `NULL` on allocation failure; if it does, subsequent `mysql_real_connect()` would use an uninitialized structure.
- **Code snippet**:
  ```cpp
  database_mariadb = (MYSQL *) mymalloc(sizeof(MYSQL));
  mysql_init(database_mariadb);   // return value not checked
  ```

### Assessment: CONFIRMED BUG (extremely unlikely trigger)

**Verdict**: `mysql_init()` with a non-NULL argument can return NULL if internal library initialization fails (e.g., `mysql_library_init()` hasn't been called, or out of memory during SSL init). If it returns NULL, the struct pointed to by `database_mariadb` is left in an indeterminate state, and `mysql_real_connect()` would likely crash or produce garbage.

**Impact**: Near-zero. `mysql_init()` failure on a pre-allocated struct is extraordinarily rare and would only happen under extreme system resource exhaustion. The `mymalloc` already handles the OOM case (it aborts on failure).

**Upstream**: No upstream issue found.

**Fix**: Check return value: `if (mysql_init(database_mariadb) == NULL) { MOTION_LOG(ERR, ...); return; }`

---

## 8. draw.cpp — 'M' glyph row has only 6 elements instead of 7

- **File**: `src/draw.cpp`
- **Line**: ~908
- **Description**: The fourth pixel row of the 'M' bitmap has 6 values instead of 7. C++ zero-initializes the missing element, so the rightmost column of that row is always 0 (transparent). This causes a subtle visual artifact where the top-right area of the 'M' glyph is missing one pixel column on row 4.
- **Code snippet**:
  ```cpp
  {1,2,1,1,1,2,},   /* row 4: only 6 values; 7th defaults to 0 */
  ```

### Assessment: CONFIRMED BUG (cosmetic)

**Verdict**: Verified. Row 4 (line 908) has `{1,2,1,1,1,2,}` — 6 values. All other rows of 'M' have 7 values. The surrounding rows show the pattern: row 3 is `{1,2,1,2,1,2,1}` and row 5 is `{1,2,1,0,1,2,1}`. Row 4 should logically be `{1,2,1,1,1,2,1}` — the missing 7th element is a `1` (border pixel), not `0` (transparent). The zero-initialized 7th element makes the top-right corner of the 'M' glyph have a tiny transparent gap.

**Impact**: One-pixel cosmetic defect on the 'M' character in text overlays. Only visible at 1x scale; barely noticeable at text_scale > 1.

**Upstream**: Issue #1231 mentioned glyph enhancements but not this specific defect.

**Fix**: Change line 908 to `{1,2,1,1,1,2,1},`

---

## 9. draw.cpp — Dead assignment to `out` in location()

- **File**: `src/draw.cpp`
- **Line**: ~1274, ~1280
- **Description**: `out` is assigned from `imgs->image_motion.image_norm` twice — once at the top of the function and again immediately after a local variable block. The first assignment is immediately overwritten. No functional impact since both values are identical, but it indicates a copy-paste artifact.
- **Code snippet**:
  ```cpp
  u_char *out = imgs->image_motion.image_norm;   // first assignment
  // ...local variable declarations...
  out = imgs->image_motion.image_norm;            // redundant duplicate
  ```

### Assessment: CONFIRMED (dead code)

**Verdict**: Verified. Line 1274 and line 1280 both assign the same value. The first is overwritten before any use. Harmless dead code.

**Impact**: None.

**Fix**: Remove line 1280.

---

## 10. json_parse.cpp — parseBool() returns false on both "false" and parse error

- **File**: `src/json_parse.cpp`
- **Line**: ~311–312
- **Description**: `parseBool()` returns `false` for both a legitimately parsed `"false"` literal and for a parse failure (with `setError()`). Callers that don't separately check `error_` cannot distinguish the two cases. The function's contract is ambiguous.
- **Code snippet**:
  ```cpp
  setError("Invalid boolean value");
  return false;   // same return as a valid "false"
  ```

### Assessment: NOT A BUG (correct caller pattern)

**Verdict**: The caller in `parseValue()` (line 247) calls `parseBool()`, then the parent `parsePair()` (line 181) checks `if (!error_.empty()) return false;` immediately after. So the error case IS distinguished — via the `error_` member, not the return value. This is a standard pattern for parsers with separate error state.

The ambiguity only matters if a future caller forgets to check `error_`. Since `json_parse.cpp` is self-contained fork-specific code with exactly one call site, this is not a practical concern.

**Impact**: None with current callers.

**Fix**: None needed. Optionally, document the contract in a comment.

---

## 11. json_parse.cpp — null, arrays, and nested objects silently rejected

- **File**: `src/json_parse.cpp`
- **Line**: ~254
- **Description**: `parseValue()` only handles strings, booleans, and numbers. JSON `null` literals, arrays (`[...]`), and nested objects (`{...}`) trigger "Unexpected character in value". There is no documented contract that callers are responsible for sending only flat objects; if the API surface grows, clients sending arrays or null values will be silently rejected.
- **Code snippet**:
  ```cpp
  setError("Unexpected character in value");
  return nullptr;   // triggered by 'n' (null), '[', or nested '{'
  ```

### Assessment: NOT A BUG (by design)

**Verdict**: Motion's web API only accepts flat key-value configuration parameters (string, int, bool). The JSON parser is purpose-built for this use case. Supporting null, arrays, and nested objects would add complexity with no benefit — there are no config parameters that use those types.

The error message "Unexpected character in value" is clear enough for debugging. This is an intentional simplification, not an oversight.

**Impact**: None for current use case.

**Fix**: None needed. Optionally, add a comment noting the flat-object-only design.

---

## 12. json_parse.cpp — Duplicate key silently overwrites earlier value

- **File**: `src/json_parse.cpp`
- **Line**: ~184
- **Description**: `values_[key] = value` silently overwrites any previous value if the same key appears more than once. RFC 8259 permits this behavior, but it means a malformed or adversarially crafted payload could shadow a parameter by repeating its key with no error or warning.
- **Code snippet**:
  ```cpp
  values_[key] = value;   // last writer wins; no duplicate detection
  ```

### Assessment: NOT A BUG (spec-compliant)

**Verdict**: RFC 8259 Section 4 states: "The names within an object SHOULD be unique" — with SHOULD, not MUST. Last-writer-wins is explicitly a valid implementation choice. Many JSON parsers (including the major ones in browsers) use this behavior. The "adversarial shadowing" concern is theoretical — the web API applies the same config validation regardless of duplicate keys, and each parameter is independently validated.

**Impact**: None.

**Fix**: None needed.

---

## 13. logger.cpp — syslog loglvl offset is fragile and undocumented

- **File**: `src/logger.cpp`
- **Line**: ~124, ~148
- **Description**: `syslog(loglvl-1, ...)` uses a `-1` offset to map Motion log levels to syslog priorities. This offset is never explained or documented. If log level constants change or are reordered, the syslog priority mapping will silently shift.
- **Code snippet**:
  ```cpp
  syslog(loglvl-1, "%s", flood_repeats);   // unexplained -1 offset
  ```

### Assessment: CORRECT BUT FRAGILE (code quality issue)

**Verdict**: The mapping is correct. Motion's log levels (1=EMG, 2=ALR, 3=CRT, 4=ERR, 5=WRN, 6=NTC, 7=INF, 8=DBG) map to syslog priorities (0=LOG_EMERG, 1=LOG_ALERT, 2=LOG_CRIT, 3=LOG_ERR, 4=LOG_WARNING, 5=LOG_NOTICE, 6=LOG_INFO, 7=LOG_DEBUG) with a consistent -1 offset. This works because both systems use the same ordering.

The `-1` is fragile — if someone adds a new log level or changes the numbering, the syslog output silently shifts. But this has been stable for many years.

**Impact**: None currently.

**Fix**: Define a mapping function or add a comment explaining the -1 offset.

---

## 14. netcam.cpp — read_image(): nodata limit of 1000 is undocumented and may be too large

- **File**: `src/netcam.cpp`
- **Line**: ~1474–1478
- **Description**: When `decode_packet()` returns 0, the loop retries up to 1000 times with no delay. On a stream that consistently delivers non-video packets (e.g., audio-only bursts), this could busy-spin 1000 iterations before giving up. CPU cost may be non-trivial on a Pi under high audio traffic.
- **Code snippet**:
  ```cpp
  nodata++;
  if (nodata > 1000) {   // arbitrary, no delay between retries
      context_close();
      return -1;
  }
  ```

### Assessment: NOT A BUG (implicit blocking)

**Verdict**: The bug report's concern about "busy-spinning" is incorrect. Each loop iteration calls `av_read_frame()` (line ~1425), which performs a blocking network read. The `decode_packet() == 0` case means a packet was successfully read from the network but it wasn't a video frame (e.g., audio packet, or a video frame that didn't produce decoded output). The loop re-reads from the network, which blocks until the next packet arrives. This is NOT a busy spin — there's an implicit network I/O delay on every iteration.

The 1000-iteration limit is a safety net for pathological streams that send thousands of non-video packets. The `/* The 1000 is arbitrary */` comment is accurate and honest.

**Impact**: None. Network I/O prevents busy-spinning.

**Fix**: None needed. The comment acknowledges the arbitrary choice.

---

## 15. netcam.cpp — pktarray_resize(): newsize can be negative if idnbr_first < idnbr_last

- **File**: `src/netcam.cpp`
- **Line**: ~460–464
- **Description**: `newsize = (idnbr_first - idnbr_last) + (idnbr - idnbr_last) * 2` can be negative when the image ring wraps (ring_out ahead of ring_in). The `if (newsize < 30)` clamp handles this gracefully, but the intent and the ring-buffer wrap condition are not documented.
- **Code snippet**:
  ```cpp
  newsize =(int)(((idnbr_first - idnbr_last) * 1 ) +
      ((idnbr - idnbr_last ) * 2));
  if (newsize < 30) {
      newsize = 30;   // clamp handles negative, but reason is undocumented
  }
  ```

### Assessment: NOT A BUG (correctly handled)

**Verdict**: The clamp at line 462 correctly handles negative values by falling back to the minimum size of 30. The comment `/* The 30 is arbitrary */` (line 458) is already present. This is working as intended — the formula estimates a good packet array size, and the clamp is a safety floor for edge cases including ring-buffer wrap.

**Impact**: None.

**Fix**: None needed. Optionally add a comment explaining the ring-buffer wrap case.

---

## 16. netcam.cpp — url_match(): caller must free() returned string but no ownership is documented

- **File**: `src/netcam.cpp`
- **Line**: ~243–258
- **Description**: `url_match()` returns a `mymalloc`-allocated string. The caller in `url_parse()` correctly calls `free(s)`, but the ownership contract is not documented. If a future caller omits the `free()` there will be a leak with no compiler warning.
- **Code snippet**:
  ```cpp
  char *cls_netcam::url_match(regmatch_t m, const char *input) {
      // returns mymalloc'd string — caller must free()
  }
  ```

### Assessment: CODE QUALITY ISSUE (not a bug)

**Verdict**: The current code is correct — the single caller `url_parse()` does free the result. The ownership contract is undocumented, but this is a private class method with one call site. The risk of a future memory leak is low.

**Impact**: None currently.

**Fix**: The comment describing the function (line 242) already notes "newly-allocated C string." Optionally add `/* Caller must free() */` to the return.

---

## 17. netcam.cpp — netcam_start(): context_close() before handler thread is subtle

- **File**: `src/netcam.cpp`
- **Line**: ~2320
- **Description**: After the initial connect and first image read, `context_close()` is explicitly called before starting the handler thread to avoid cross-thread codec contamination. This means startup does a full open + read + close just to validate connectivity. The relationship between the startup close and the handler reconnect path is subtle and easy to misread as a bug.
- **Code snippet**:
  ```cpp
  context_close();          /* Close in this thread to open it again within handler thread */
  status = NETCAM_RECONNECTING;
  ```

### Assessment: NOT A BUG (intentional design)

**Verdict**: This is a deliberate design pattern. FFmpeg decoder contexts are not thread-safe. The startup sequence validates connectivity and reads one frame to confirm the camera works, then closes everything so the handler thread can open a fresh context in its own thread. The inline comment clearly explains the intent: "Close in this thread to open it again within handler thread."

**Impact**: None. The open-validate-close-reopen pattern adds a small startup delay but ensures thread safety.

**Fix**: None needed.

---

## 18. picture.cpp — Duplicate include of picture.hpp

- **File**: `src/picture.cpp`
- **Line**: 33 and 37
- **Description**: `#include "picture.hpp"` appears twice. Include guards prevent compilation errors, but the duplicate is unnecessary and suggests a copy-paste oversight.
- **Code snippet**:
  ```cpp
  #include "picture.hpp"      // line 33
  #include "jpegutils.hpp"
  #include "draw.hpp"
  #include "dbse.hpp"
  #include "picture.hpp"      // line 37 — duplicate
  ```

### Assessment: CONFIRMED (trivial)

**Verdict**: Verified. Harmless due to include guards but unnecessary.

**Impact**: None.

**Fix**: Remove line 37.

---

## 19. picture.cpp — Redundant imgts self-assignment in process_preview()

- **File**: `src/picture.cpp`
- **Line**: ~201–202, ~222–223
- **Description**: `saved_current_image` is assigned `cam->current_image` (same pointer), then `saved_current_image->imgts = cam->current_image->imgts` assigns a struct to itself — a no-op. The restore on line 223 is also a no-op. The pointer swap/restore works correctly, but the `imgts` copies are dead code.
- **Code snippet**:
  ```cpp
  saved_current_image = cam->current_image;
  saved_current_image->imgts= cam->current_image->imgts;  // no-op (same object)
  ```

### Assessment: CONFIRMED (dead code)

**Verdict**: Verified. Line 201 sets `saved_current_image = cam->current_image`, so line 202 is `ptr->imgts = ptr->imgts` — a self-assignment. Similarly, after restoring the pointer at line 222, line 223 is also a self-assignment. The meaningful operation is the pointer save/restore (lines 201, 204, 222), not the imgts copies.

**Impact**: None.

**Fix**: Remove lines 202 and 223.

---

## 20. sound.cpp — check_alerts trigger_count uses equality check; could miss threshold if count skips

- **File**: `src/sound.cpp`
- **Line**: ~755
- **Description**: Alert firing uses `trigger_count == trigger_threshold` (equality). If two triggers arrive in the same analysis period or `trigger_count` increments past the threshold for any reason, the alert never fires. A `>=` comparison would be more robust.
- **Code snippet**:
  ```cpp
  if (it->trigger_count == it->trigger_threshold) {
      // fires only at exactly the threshold, never if count exceeds it
  ```

### Assessment: NOT A BUG (equality is correct)

**Verdict**: The bug report's concern about "count skipping" is unfounded. Looking at the code (lines 748–752):

```cpp
if ((trig_ts.tv_sec - it->trigger_time.tv_sec) > it->trigger_duration) {
    it->trigger_count = 1;   // reset to 1 on new window
} else {
    it->trigger_count++;     // always increments by exactly 1
}
```

`trigger_count` is always incremented by exactly 1 per analysis period. It cannot "skip" a value. The `==` check fires the alert exactly once when the threshold is reached, then the counter continues incrementing past it (4, 5, 6, ...) without re-firing. This is the correct behavior — fire once per trigger window, not on every subsequent period.

Using `>=` would cause the alert to fire on EVERY analysis period after reaching the threshold until the duration window resets. That would spam the `on_sound_alert` command.

**Impact**: None. The `==` is intentionally correct.

**Fix**: None needed.

---

## Summary

| # | Bug | Confirmed? | Category | Fix needed? |
|---|-----|-----------|----------|-------------|
| 1 | all_cnct never decremented | Likely by design | Minor concern | Clarify intent |
| 2 | getsizes_scale float cast | **NOT A BUG** | Code style | Remove dead cast |
| 3 | Magic number 5 for streams | Code quality | Maintainability | Define constant |
| 4 | stream_max_connections missing from config_parms | **YES** | Config bug | Add to array |
| 5 | Dead code after return in handlers | **YES** | Dead code | Delete lines |
| 6 | CLOCK_MONOTONIC with localtime_r | **YES** (benign) | Wrong clock | Use CLOCK_REALTIME |
| 7 | mysql_init return not checked | **YES** (near-zero risk) | Missing check | Add null check |
| 8 | 'M' glyph missing 7th pixel | **YES** | Cosmetic | Add missing `1` |
| 9 | Dead assignment to `out` | **YES** | Dead code | Remove line |
| 10 | parseBool false ambiguity | **NOT A BUG** | Correct pattern | None |
| 11 | null/arrays/objects rejected | **NOT A BUG** | By design | None |
| 12 | Duplicate key overwrites | **NOT A BUG** | Spec-compliant | None |
| 13 | syslog loglvl-1 offset | Correct but fragile | Code quality | Add comment |
| 14 | nodata 1000 busy spin | **NOT A BUG** | Network blocks | None |
| 15 | pktarray_resize negative | **NOT A BUG** | Correctly clamped | None |
| 16 | url_match ownership undocumented | Code quality | Documentation | Add comment |
| 17 | context_close before handler | **NOT A BUG** | Intentional | None |
| 18 | Duplicate include | **YES** | Trivial | Remove line |
| 19 | Redundant imgts self-assignment | **YES** | Dead code | Remove lines |
| 20 | trigger_count == threshold | **NOT A BUG** | Equality is correct | None |

**8 of 20 confirmed as real issues** (all minor). **8 are not bugs** — they are either by-design, spec-compliant, or based on incorrect analysis in the report. **4 are code quality/style issues** that don't affect functionality.

Notable corrections:
- Bug #2 (float cast precision): The report's math is wrong — integer truncation and float truncation produce identical results.
- Bug #14 (nodata busy spin): `av_read_frame()` blocks on network I/O each iteration — not a busy spin.
- Bug #20 (trigger_count ==): Equality is correct — `>=` would spam the alert command. The count never skips values.
