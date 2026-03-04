# Code Review Issues — /comment run

Files reviewed: `src/sound.cpp`

## Potential Issues

### HammingWindow formula is incorrect
- **File**: `src/sound.cpp`
- **Line**: ~652 (in `HammingWindow`)
- **Severity**: high
- **Description**: The standard Hamming window is `0.54 - 0.46 * cos(2π·n / (N-1))`. The code computes `cos((2 * M_PI * n1)) / (N2 - 1)`, which passes `2π·n` to cos (without dividing by N-1) and then divides the cosine result by (N-1) as a scale factor. This produces a wildly oscillating function rather than a smooth 0–1 taper, incorrectly windowing every sample.
- **Code snippet**:
  ```cpp
  return 0.54F - 0.46F * (float)(cos((2 * M_PI * n1)) / (N2 - 1));
  // Should be: 0.54 - 0.46 * cos(2 * M_PI * n1 / (N2 - 1))
  ```

### HannWindow formula is incorrect
- **File**: `src/sound.cpp`
- **Line**: ~656 (in `HannWindow`)
- **Severity**: high
- **Description**: The standard Hann window is `0.5 * (1 - cos(2π·n / (N-1)))`. The code computes `cos(2 * M_PI * n1 * N2)`, multiplying `n` by `N` instead of dividing. With, for example, n=10 and N=2048, the cosine argument becomes 20480π, producing a random-looking value rather than a smooth taper. This renders the Hann window option effectively useless.
- **Code snippet**:
  ```cpp
  return 0.5F * (float)(1 - (cos(2 * M_PI * n1 * N2)));
  // Should be: 0.5 * (1 - cos(2 * M_PI * n1 / (N2 - 1)))
  ```

### alsa_start leaks hw_params on every early-return error path
- **File**: `src/sound.cpp`
- **Line**: ~354–467 (in `alsa_start`)
- **Severity**: medium
- **Description**: `hw_params` is allocated with `snd_pcm_hw_params_malloc` at ~line 354. It is only freed with `snd_pcm_hw_params_free` at ~line 467, after all parameters are successfully queried. The seven `if (retcd < 0)` error paths that precede the free (for `params_any`, `set_access`, `set_format`, `set_rate_near`, `set_channels`, `set_period_size_near`, `hw_params`, `prepare`) all set `device_status = STATUS_CLOSED` and return without freeing. Each device open failure leaks the allocation.
- **Code snippet**:
  ```cpp
  retcd = snd_pcm_hw_params_malloc(&hw_params);  // allocated here
  // ... many error paths return without calling snd_pcm_hw_params_free(hw_params)
  snd_pcm_hw_params_free(hw_params);              // only reached on full success
  ```

### check_alerts trigger_count uses equality check; could miss threshold if count skips
- **File**: `src/sound.cpp`
- **Line**: ~731 (in `check_alerts`)
- **Severity**: low
- **Description**: Alert firing uses `trigger_count == trigger_threshold` (equality). If two triggers arrive in the same analysis period (or if `trigger_count` increments past `trigger_threshold` for any reason), the alert never fires. A `>=` comparison would be more robust.
- **Code snippet**:
  ```cpp
  if (it->trigger_count == it->trigger_threshold) {
      // fires only at exactly the threshold, never if count exceeds it
  ```
