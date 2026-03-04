# Code Review Issues — /comment run

Files reviewed: `src/draw.cpp`

## Potential Issues

### Negative ASCII index in textn() — out-of-bounds array access
- **File**: `src/draw.cpp`
- **Line**: ~1120 (original), inside `textn()`
- **Severity**: medium
- **Description**: `pos_check = (int)text[pos]` where `text` is `const char *`. On platforms where `char` is signed, characters with values > 127 (e.g., extended ASCII or UTF-8 multi-byte sequences) produce a negative `pos_check`. The expression `char_arr_ptr[pos_check]` is then an out-of-bounds negative index into a fixed-size array, which is undefined behavior and can cause a crash or memory corruption. The guard `if (pos_check < 0) { image_ptr++; continue; }` inside the inner loop catches this for the *second* assignment at the re-read location, but the initial assignment `char_ptr = char_arr_ptr[pos_check]` at the start of the outer `for (pos…)` loop is reached before any guard.
- **Code snippet**:
  ```cpp
  int pos_check = (int)text[pos];
  char_ptr = char_arr_ptr[pos_check];   // UB if pos_check < 0
  for (y = 0; y < 8 * factor; y++) {
      for (x = 0; x < 7 * factor; x++) {
          if (pos_check < 0) {           // guard is too late
              image_ptr++;
              continue;
          }
          char_ptr = char_arr_ptr[pos_check] + y/factor*7 + x/factor;
  ```

### 'M' glyph row has only 6 elements instead of 7
- **File**: `src/draw.cpp`
- **Line**: ~910 (row 4 of the 'M' glyph in `draw_table`)
- **Severity**: low
- **Description**: The fourth pixel row of the 'M' bitmap has 6 values instead of the required 7 (`{1,2,1,1,1,2,}` — trailing comma, missing final element). C++ zero-initializes the missing 7th element, so the rightmost column of that row is always 0 (transparent). This may cause a very subtle visual artifact in the 'M' glyph where the top-right area is missing one pixel column on row 4.
- **Code snippet**:
  ```cpp
  {1,2,1,1,1,2,},   /* row 4: only 6 values; 7th defaults to 0 */
  ```

### Dead assignment to `out` in location()
- **File**: `src/draw.cpp`
- **Line**: ~1270 (original), inside `location()`
- **Severity**: low
- **Description**: `out` is assigned from `imgs->image_motion.image_norm` twice — once at the top of the function body and again immediately after a local variable block. The first assignment on the opening line of the function is immediately overwritten by the second. No functional impact since both values are identical, but it indicates a copy-paste artifact.
- **Code snippet**:
  ```cpp
  u_char *out = imgs->image_motion.image_norm;   // first assignment
  // ...local variable declarations...
  out = imgs->image_motion.image_norm;            // redundant duplicate
  ```
