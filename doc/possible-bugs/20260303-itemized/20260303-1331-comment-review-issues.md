# Code Review Issues — /comment run

Files reviewed: `src/thumbnail.cpp`

## Potential Issues

### encode_thumbnail returns non-negative retcd on several failure paths
- **File**: `src/thumbnail.cpp`
- **Line**: ~400–418 (in `encode_thumbnail`)
- **Severity**: medium
- **Description**: After `sws_scale` succeeds, `retcd` holds the number of output scan lines (a positive integer). Three later failure paths (`jpgutl_put_yuv420p` returning ≤ 0, `fopen` returning nullptr, `fwrite` short-write) all `goto cleanup` without resetting `retcd` to a negative value. `generate()` then sees `retcd >= 0` and logs "Generated thumbnail" even though the file was not written. The final `retcd = 0` assignment on line 418 is only reached on full success, but the intermediate paths escape with the sws_scale result instead of -1.
- **Code snippet**:
  ```cpp
  retcd = sws_scale(...);          // retcd = positive line count on success
  if (retcd < 0) { goto cleanup; }

  /* ... */

  if (jpg_size <= 0) {
      goto cleanup;                // retcd is still the sws_scale line count (>= 0)
  }

  f = fopen(thumb_path.c_str(), "wb");
  if (f == nullptr) {
      goto cleanup;                // same — retcd still >= 0
  }

  if (fwrite(...) != (size_t)jpg_size) {
      goto cleanup;                // same
  }

  retcd = 0;  // only reached on success
  ```

### Partial or zero-byte thumbnail file left on disk after write failure
- **File**: `src/thumbnail.cpp`
- **Line**: ~405–416 (in `encode_thumbnail`)
- **Severity**: medium
- **Description**: If `fopen` succeeds but `fwrite` fails (e.g., out of disk space), the file descriptor is closed in the `cleanup` block but the newly-created, empty (or partially-written) thumbnail file is not removed. On the next run, `exists()` uses `stat()` and will find the stale file, returning true and skipping regeneration permanently. The thumbnail would be silently missing from the UI with no way to recover short of manually deleting the `.thumb.jpg` file.
- **Code snippet**:
  ```cpp
  f = fopen(thumb_path.c_str(), "wb");  // file created here
  if (f == nullptr) { goto cleanup; }

  if (fwrite(jpg_buffer, 1, (size_t)jpg_size, f) != (size_t)jpg_size) {
      goto cleanup;  // file exists on disk but is empty/partial
  }

  // cleanup:
  if (f != nullptr) {
      fclose(f);   // closed but NOT unlinked on failure
  }
  ```
