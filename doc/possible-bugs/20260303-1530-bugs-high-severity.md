# High Severity Bugs

Issues that cause crashes, deadlocks, incorrect behavior, or silent data corruption.

## Bug Status

| # | Bug | Status |
|---|-----|--------|
| 1 | Double mutex lock deadlock | ✅ Resolved |
| 2 | Null deref in passthru_check | ✅ Resolved |
| 3 | Buffer underflow in ff_log | ✅ Resolved |
| 4 | Schedule time comparison | ✅ Resolved |
| 5 | HammingWindow formula | ✅ Resolved |
| 6 | HannWindow formula | ✅ Resolved |

---

## 1. motion.cpp — Double mutex lock (deadlock) in check_restart()

- **File**: `src/motion.cpp`
- **Line**: ~466–470
- **Description**: The database restart block calls `pthread_mutex_lock` twice instead of lock then unlock. The second call should be `pthread_mutex_unlock`. POSIX default mutexes are not recursive, so the second lock attempt deadlocks the calling thread permanently.
- **Code snippet**:
  ```cpp
  pthread_mutex_lock(&dbse->mutex_dbse);
      dbse->shutdown();
      cfg->parms_copy(conf_src, PARM_CAT_15);
      dbse->startup();
  pthread_mutex_lock(&dbse->mutex_dbse);   // <-- should be pthread_mutex_unlock
  ```

### Assessment: CONFIRMED BUG

**Verdict**: This is a real, high-severity bug. Line 470 is `pthread_mutex_lock` when it must be `pthread_mutex_unlock`. This is a copy/paste error — the surrounding log and webu restart blocks don't use mutexes, so the pattern wasn't available to copy correctly.

**Impact**: Any database configuration restart (triggered via web config changes to database params) will permanently deadlock the main application thread. Motion becomes unresponsive and would only recover via watchdog kill or manual restart.

**Upstream**: Not present in upstream Motion. The upstream `check_restart()` does not use `mutex_dbse` at all. This bug is specific to this fork's addition of mutex-protected database restart. Related upstream deadlock bugs (PR #1373) fixed similar missing-unlock issues in netcam/bktr code.

**Mitigating factors**: Database config changes via the web UI are rare in normal operation. Most users configure the database once and don't change it at runtime, so few would trigger this code path.

**Fix**: Change line 470 from `pthread_mutex_lock` to `pthread_mutex_unlock`.

### Resolution

**Date Resolved:** 2026-03-03

**Root Cause:** Copy/paste error — second `pthread_mutex_lock` should have been `pthread_mutex_unlock`.

**Solution:** Changed `pthread_mutex_lock(&dbse->mutex_dbse)` to `pthread_mutex_unlock(&dbse->mutex_dbse)` on line 470.

**Files Modified:**
| File | Change |
|------|--------|
| src/motion.cpp:470 | `pthread_mutex_lock` → `pthread_mutex_unlock` |

**Status:** ✅ RESOLVED

---

## 2. movie.cpp — Null pointer dereference in passthru_check()

- **File**: `src/movie.cpp`
- **Line**: ~1334–1344
- **Description**: `netcam_data->status` is accessed before the `netcam_data == nullptr` guard. If `netcam_data` is null, the status check dereferences a null pointer and causes a SEGV. The nullptr guard must come first.
- **Code snippet**:
  ```cpp
  if ((netcam_data->status == NETCAM_NOTCONNECTED  ) ||   // dereferences netcam_data
      (netcam_data->status == NETCAM_RECONNECTING  )) {
      ...
  }
  if (netcam_data == nullptr) {  // too late, already dereferenced above
  ```

### Assessment: CONFIRMED BUG (low practical risk)

**Verdict**: The check ordering is definitively wrong. The nullptr guard on line 1341 is dead code — if `netcam_data` were null, the program would SEGV on line 1334 before reaching it. The author clearly intended to guard against null (the check exists), but placed it after the dereference.

**Practical risk**: Low. `passthru_check()` is only called from `passthru_open()`, which is only invoked for passthrough-mode recordings on netcams. In this code path, `netcam_data` is set in the movie constructor from `cam->netcam_high` or `cam->netcam` (lines 2011–2016), which should be non-null for any active netcam. However, race conditions during camera disconnect/reconnect could theoretically produce a null.

**Notably**, the sibling function `passthru_put()` (line 1160) has the checks in the **correct** order — nullptr first, then status. This confirms the intended pattern and that `passthru_check()` is simply wrong.

**Upstream**: This exact bug exists in the upstream Motion code (`src/movie.cpp:1073-1082`). It has not been reported as a distinct issue, though several passthrough-related SEGV bugs have been filed (#610, #619, #1455) that could be manifestations of it.

**Fix**: Move the `netcam_data == nullptr` check before the status dereference, matching `passthru_put()`.

### Resolution

**Date Resolved:** 2026-03-03

**Root Cause:** Guard ordering error — nullptr check placed after the pointer dereference, making it dead code.

**Solution:** Swapped the two `if` blocks so the nullptr check comes first, matching the pattern in `passthru_put()`.

**Files Modified:**
| File | Change |
|------|--------|
| src/movie.cpp:1334-1344 | Moved `netcam_data == nullptr` guard before `netcam_data->status` access |

**Status:** ✅ RESOLVED

---

## 3. logger.cpp — Buffer underflow when FFmpeg emits an empty message in ff_log()

- **File**: `src/logger.cpp`
- **Line**: ~47
- **Description**: `buff[strlen(buff)-1] = 0` strips a trailing newline. If `vsnprintf` produces zero output characters (e.g., `""` or `"%%"`), `strlen(buff) == 0` and `buff[0 - 1]` = `buff[(size_t)-1]` is an out-of-bounds write — undefined behavior and a potential crash. FFmpeg occasionally calls its log callback with empty or whitespace-only messages.
- **Code snippet**:
  ```cpp
  vsnprintf(buff, sizeof(buff), fmt, vlist);
  buff[strlen(buff)-1] = 0;   // UB if strlen(buff) == 0
  ```

### Assessment: CONFIRMED BUG

**Verdict**: This is a real bug. `strlen()` returns `size_t` (unsigned), so `0 - 1` wraps to `SIZE_MAX` (~18 quintillion on 64-bit), producing an out-of-bounds write far beyond the 1024-byte buffer. This is undefined behavior and will typically SEGV.

**Trigger conditions**: FFmpeg does occasionally call its log callback with formats that produce empty output. This is rare under normal operation but becomes more likely with elevated FFmpeg log levels (`log_fflevel`). The `"%%"` format string is a known edge case that produces empty output after vsnprintf processing.

**Upstream**: This bug exists in the upstream code (`src/logger.cpp:36`). Upstream issue #1862 reported a closely related crash (`SIGABRT` / `__strcat_chk buffer overflow detected`) in the v4.x logger when FFmpeg debug logging was active. The reporter confirmed v5.0 fixed that specific crash, but the `strlen(buff)-1` underflow pattern remains in the v5.0 `ff_log()` function.

**Fix**: Add a length check before the newline strip:
```cpp
size_t len = strlen(buff);
if (len > 0) {
    buff[len-1] = 0;
}
```

### Resolution

**Date Resolved:** 2026-03-03

**Root Cause:** `strlen()` returns `size_t` (unsigned). With empty string, `0 - 1` wraps to `SIZE_MAX`, causing out-of-bounds write.

**Solution:** Added length check with early return on empty string. Also improved the newline strip to only remove the character if it's actually `'\n'` (the original unconditionally zeroed the last character).

**Files Modified:**
| File | Change |
|------|--------|
| src/logger.cpp:47 | Added `len == 0` early return and conditional newline strip |

**Status:** ✅ RESOLVED

---

## 4. schedule.cpp — Time-range comparison incorrect for cross-minute windows in schedule_cam()

- **File**: `src/schedule.cpp`
- **Line**: ~68–73
- **Description**: The schedule window check compares hour and minute as four independent inequalities. Any window that crosses a minute boundary within an hour is impossible to satisfy. For example, window 14:45–15:30 requires `tm_min >= 45 AND tm_min <= 30` simultaneously — always false. Even whole-hour windows like 09:00–17:00 fail: at 14:30, `tm_min(30) <= en_min(0)` is false. The correct approach is to compare a single combined value such as `tm_hour * 60 + tm_min`.
- **Code snippet**:
  ```cpp
  (c_tm.tm_hour >= p_cam->schedule[cur_dy][indx].st_hr) &&
  (c_tm.tm_min  >= p_cam->schedule[cur_dy][indx].st_min) &&
  (c_tm.tm_hour <= p_cam->schedule[cur_dy][indx].en_hr) &&
  (c_tm.tm_min  <= p_cam->schedule[cur_dy][indx].en_min)
  ```

### Assessment: CONFIRMED BUG

**Verdict**: This is a clear logic error. The independent hour/minute comparison fails for virtually all practical schedules. Worked examples:

| Schedule | Time | Hour check | Min check | Result | Correct? |
|----------|------|------------|-----------|--------|----------|
| 09:00–17:00 | 14:00 | 14≥9 ✓, 14≤17 ✓ | 0≥0 ✓, 0≤0 ✓ | MATCH | ✓ |
| 09:00–17:00 | 14:30 | 14≥9 ✓, 14≤17 ✓ | 30≥0 ✓, **30≤0 ✗** | NO MATCH | ✗ Should match |
| 09:30–10:15 | 09:45 | 9≥9 ✓, 9≤10 ✓ | 45≥30 ✓, **45≤15 ✗** | NO MATCH | ✗ Should match |
| 09:30–10:15 | 10:00 | 10≥9 ✓, 10≤10 ✓ | **0≥30 ✗** | NO MATCH | ✗ Should match |
| 09:00–17:00 | 09:00 | 9≥9 ✓, 9≤17 ✓ | 0≥0 ✓, 0≤0 ✓ | MATCH | ✓ |

The schedule only works correctly when `st_min <= current_min <= en_min` — which only happens at the exact start/end minutes or when start and end minutes are both 0 and the current minute is also 0. For `09:00–17:00`, the schedule is active only at `HH:00` each hour.

**Upstream**: This exact bug exists in the upstream code (`src/schedule.cpp`). The schedule feature was contributed in PR #1602 and appears to have received minimal testing. The upstream maintainer noted they were "generally not adding enhancements." No bug report has been filed about the time comparison.

**Fix**: Compare linearized minutes:
```cpp
int cur_total = c_tm.tm_hour * 60 + c_tm.tm_min;
int st_total  = p_cam->schedule[cur_dy][indx].st_hr * 60 + p_cam->schedule[cur_dy][indx].st_min;
int en_total  = p_cam->schedule[cur_dy][indx].en_hr * 60 + p_cam->schedule[cur_dy][indx].en_min;
if ((p_cam->schedule[cur_dy][indx].action == "stop") &&
    (cur_total >= st_total) && (cur_total <= en_total)) {
```

### Resolution

**Date Resolved:** 2026-03-03

**Root Cause:** Comparing hours and minutes as four independent inequalities fails for any schedule where `start_min > end_min` (which is most practical schedules).

**Solution:** Linearize time to total minutes (`hour * 60 + minute`) and compare as single integers. Also added `(int)` cast on `size()` to avoid signed/unsigned comparison warning.

**Files Modified:**
| File | Change |
|------|--------|
| src/schedule.cpp:68-73 | Replaced four independent hour/minute comparisons with linearized minute comparison |

**Status:** ✅ RESOLVED

---

## 5. sound.cpp — HammingWindow formula is incorrect

- **File**: `src/sound.cpp`
- **Line**: ~669
- **Description**: The standard Hamming window is `0.54 - 0.46 * cos(2π·n / (N-1))`. The code computes `cos((2 * M_PI * n1)) / (N2 - 1)` — it passes the full `2π·n` to cos without dividing by N-1, then divides the cosine result by (N-1) as a scale factor. This produces a wildly oscillating function rather than a smooth 0–1 taper, incorrectly windowing every sample.
- **Code snippet**:
  ```cpp
  return 0.54F - 0.46F * (float)(cos((2 * M_PI * n1)) / (N2 - 1));
  // Should be: 0.54 - 0.46 * cos(2 * M_PI * n1 / (N2 - 1))
  ```

### Assessment: CONFIRMED BUG

**Verdict**: The parentheses are in the wrong place, making the formula mathematically incorrect. The code computes:

`0.54 - 0.46 * (cos(2πn) / (N-1))`

Since `n` is always an integer, `cos(2πn) = 1.0` exactly, so this simplifies to:

`0.54 - 0.46/(N-1)`

For a typical N=2048: `0.54 - 0.46/2047 ≈ 0.5398`

**Every sample gets the same value (~0.54)**, making this a flat (rectangular) window with a slight offset — completely defeating the purpose of windowing. The window provides no spectral leakage reduction. The initial bug report's claim of "wildly oscillating" is technically incorrect (integer n makes cos(2πn) constant), but the function is still broken — it's a flat constant instead of a smooth taper.

**Upstream**: This exact bug exists in the upstream code (`src/sound.cpp:642`). Never reported. The sound/FFTW feature appears to have been contributed without verification of the window functions.

**Fix**:
```cpp
return 0.54F - 0.46F * (float)cos(2 * M_PI * n1 / (N2 - 1));
```

### Resolution

**Date Resolved:** 2026-03-03

**Root Cause:** Parentheses placed incorrectly — `cos(2πn)` was computed first (always = 1.0 for integer n), then divided by `(N-1)`. Result was a flat constant ~0.54 for all samples.

**Solution:** Moved `/ (N2 - 1)` inside the `cos()` argument to produce the correct smooth taper.

**Files Modified:**
| File | Change |
|------|--------|
| src/sound.cpp:669 | Fixed parentheses: `cos((2*M_PI*n1))/(N2-1)` → `cos(2*M_PI*n1/(N2-1))` |

**Status:** ✅ RESOLVED

---

## 6. sound.cpp — HannWindow formula is incorrect

- **File**: `src/sound.cpp`
- **Line**: ~674
- **Description**: The standard Hann window is `0.5 * (1 - cos(2π·n / (N-1)))`. The code computes `cos(2 * M_PI * n1 * N2)`, multiplying `n` by `N` instead of dividing. With n=10 and N=2048 the cosine argument becomes 20480π, producing a random-looking value rather than a smooth taper. The Hann window option is effectively non-functional.
- **Code snippet**:
  ```cpp
  return 0.5F * (float)(1 - (cos(2 * M_PI * n1 * N2)));
  // Should be: 0.5 * (1 - cos(2 * M_PI * n1 / (N2 - 1)))
  ```

### Assessment: CONFIRMED BUG

**Verdict**: The formula uses multiplication (`n * N`) instead of division (`n / (N-1)`). Since `n` and `N` are both integers, `n*N` is always an integer, and `cos(2π * integer) = 1.0` mathematically. This means the function returns:

`0.5 * (1 - 1) = 0.0`

**Every sample is multiplied by zero**, completely zeroing out the input signal before FFT. The FFT would then process an all-zero buffer, detecting no frequencies at all. Any user with `snd_window = "hann"` would get zero sound detection.

Note: In practice, due to IEEE 754 floating-point precision loss with large arguments (e.g., `cos(2π * 20971520)` for n=10240, N=2048), the result may not be exactly 1.0, so tiny non-zero residuals may appear. But the window is fundamentally wrong regardless.

**Upstream**: This exact bug exists in the upstream code (`src/sound.cpp:647`). Never reported.

**Fix**:
```cpp
return 0.5F * (float)(1 - cos(2 * M_PI * n1 / (N2 - 1)));
```

### Resolution

**Date Resolved:** 2026-03-03

**Root Cause:** Formula used multiplication (`n1 * N2`) instead of division (`n1 / (N2 - 1)`). Since both are integers, `cos(2π * integer) = 1.0`, making every sample 0.0 — zeroing all input to the FFT.

**Solution:** Changed `cos(2 * M_PI * n1 * N2)` to `cos(2 * M_PI * n1 / (N2 - 1))`.

**Files Modified:**
| File | Change |
|------|--------|
| src/sound.cpp:674 | `n1 * N2` → `n1 / (N2 - 1)` in Hann window formula |

**Status:** ✅ RESOLVED

---

## Summary

| # | Bug | Confirmed? | Severity | In Upstream? | Upstream Report? |
|---|-----|-----------|----------|-------------|-----------------|
| 1 | Double mutex lock deadlock | **YES** | High — deadlocks main thread | No (fork-specific) | N/A |
| 2 | Null deref in passthru_check | **YES** (low practical risk) | Medium — SEGV possible but unlikely trigger | Yes | No (related SEGVs filed) |
| 3 | Buffer underflow in ff_log | **YES** | Medium — UB/SEGV on rare empty FFmpeg log | Yes | Partial (#1862, #1441) |
| 4 | Schedule time comparison | **YES** | High — schedule non-functional for most windows | Yes | No |
| 5 | HammingWindow formula | **YES** | Medium — flat window, no spectral improvement | Yes | No |
| 6 | HannWindow formula | **YES** | High — zeros all input, kills sound detection | Yes | No |

All six reported issues are confirmed as real bugs. Bugs #1 (deadlock) is fork-specific. Bugs #2–6 exist in the upstream Motion codebase and have largely gone unreported.
