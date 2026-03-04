# Code Review Issues — /comment run

Files reviewed: `src/picture.cpp`

## Potential Issues

### 1. Duplicate include of picture.hpp
- **File**: `src/picture.cpp`
- **Line**: 33 and 37
- **Severity**: low
- **Description**: `#include "picture.hpp"` appears twice (lines 33 and 37). Include guards prevent compilation errors, but the duplicate is unnecessary and suggests a copy-paste oversight.
- **Code snippet**:
  ```cpp
  #include "picture.hpp"      // line 33
  #include "jpegutils.hpp"
  #include "draw.hpp"
  #include "dbse.hpp"
  #include "picture.hpp"      // line 37 — duplicate
  ```

### 2. Redundant imgts self-assignment in process_preview()
- **File**: `src/picture.cpp`
- **Line**: ~196
- **Severity**: low
- **Description**: `saved_current_image` is assigned `cam->current_image` on line 195, then line 196 does `saved_current_image->imgts = cam->current_image->imgts`. Since both pointers point to the same object at that point, this is a no-op (assigning a struct to itself). Similarly, the restore on line 217 (`cam->current_image->imgts = saved_current_image->imgts`) is a no-op because both pointers are the same again. The code works correctly — the pointer swap/restore is fine — but the imgts copies are dead code.
- **Code snippet**:
  ```cpp
  saved_current_image = cam->current_image;                      // same pointer
  saved_current_image->imgts= cam->current_image->imgts;         // no-op
  ...
  cam->current_image = saved_current_image;                      // restore pointer
  cam->current_image->imgts = saved_current_image->imgts;        // no-op again
  ```
