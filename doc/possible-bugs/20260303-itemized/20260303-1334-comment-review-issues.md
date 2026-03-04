# Code Review Issues — /comment run

Files reviewed: `src/schedule.cpp`

## Potential Issues

### schedule_cam time-range comparison is incorrect for cross-minute windows
- **File**: `src/schedule.cpp`
- **Line**: ~66–76 (in `schedule_cam`)
- **Severity**: high
- **Description**: The schedule window check compares hour and minute as four independent inequalities. This is logically incorrect for any window that crosses a minute boundary within an hour. For example, a window of 14:45–15:30 requires the current time to simultaneously satisfy `tm_min >= 45` AND `tm_min <= 30`, which is impossible. As a result, only windows whose start and end minutes are equal (e.g., 09:00–17:00) ever match a time other than that exact minute. The correct approach is to compare a single combined value such as `tm_hour * 60 + tm_min`.
- **Code snippet**:
  ```cpp
  (c_tm.tm_hour >= p_cam->schedule[cur_dy][indx].st_hr) &&
  (c_tm.tm_min  >= p_cam->schedule[cur_dy][indx].st_min) &&
  (c_tm.tm_hour <= p_cam->schedule[cur_dy][indx].en_hr) &&
  (c_tm.tm_min  <= p_cam->schedule[cur_dy][indx].en_min)
  // For window 14:45-15:30: at 15:10, tm_min(10) >= st_min(45) is false → no match.
  // For window 09:00-17:00: at 14:30, tm_min(30) <= en_min(0) is false → no match.
  ```

### cleandir_cam: unknown freq value causes cleanup to run every 30 seconds
- **File**: `src/schedule.cpp`
- **Line**: ~244–249 (in `cleandir_cam`)
- **Severity**: medium
- **Description**: After running cleanup, `next_ts` is advanced only for `freq` values "hourly", "daily", or "weekly". If `freq` holds any other value (including an empty string or a misconfigured value from the frontend), `next_ts` is never advanced. Since `cleandir_cam` triggers whenever `curr_ts >= next_ts`, the cleanup will run on every scheduling tick (every 30 seconds) for that camera until the process restarts. This could produce excessive DB queries and filesystem operations.
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
