# Folder-Based Media Browsing

**Date:** 2026-01-16
**Status:** Completed

## Overview

Implemented folder-based media browsing to replace the "By Date" view in the Media page. This provides a true filesystem folder browser that works with any folder structure users may have configured, rather than assuming date-based organization.

## Changes Made

### 1. Backend API Endpoints (C++)

**File:** `src/webu_json.cpp`

Added two new API methods:

#### `api_media_folders()` - GET /{camId}/api/media/folders

Folder browsing endpoint with:
- Query params: `path` (relative path), `offset`, `limit`
- Returns: folders (name, path, file_count, total_size) and files (id, filename, path, type, date, time, size, thumbnail)
- Path traversal prevention via `..` checking and `realpath()` validation
- Only returns media files (mp4, mkv, avi, webm, mov, jpg, jpeg, png, gif, bmp)
- Excludes thumbnail files (.thumb.jpg) from file listings
- Pagination support for large folders

#### `api_delete_folder_files()` - DELETE /{camId}/api/media/folders/files

Bulk delete endpoint with:
- Admin role required
- Delete action must be enabled via `webcontrol_actions`
- Deletes only media files (not subfolders or non-media files)
- Automatically deletes associated thumbnails
- Returns counts: movies deleted, pictures deleted, thumbnails deleted
- Database records cleaned up for deleted files

**Helper functions added:**
- `is_media_extension()` - Check if extension is a recognized media type
- `is_thumbnail()` - Check if filename is a thumbnail
- `get_file_extension()` - Extract extension from filename
- `validate_folder_path()` - Security validation for path traversal prevention

### 2. Backend Routing

**File:** `src/webu_ans.cpp`

Added routing for new endpoints:
- GET route: `uri_cmd2 == "media" && uri_cmd3 == "folders"` → `api_media_folders()`
- DELETE route: `uri_cmd3 == "folders" && uri_cmd4 == "files"` → `api_delete_folder_files()`

### 3. Frontend Types

**File:** `frontend/src/api/types.ts`

Added new TypeScript interfaces:

```typescript
interface FolderItem {
  name: string;
  path: string;
  file_count: number;
  total_size: number;
}

interface FolderFileItem {
  id: number;
  filename: string;
  path: string;
  type: 'movie' | 'picture';
  date: string;
  time: string;
  size: number;
  thumbnail?: string;
}

interface FolderContentsResponse {
  path: string;
  parent: string | null;
  folders: FolderItem[];
  files: FolderFileItem[];
  total_files: number;
  offset: number;
  limit: number;
}

interface DeleteFolderFilesResponse {
  success: boolean;
  deleted: { movies: number; pictures: number; thumbnails: number };
  errors: string[];
  path: string;
}
```

### 4. Frontend Queries

**File:** `frontend/src/api/queries.ts`

Added new query hooks:

- `useMediaFolders(camId, path, offset, limit, options)` - Fetch folder contents
- `useDeleteFolderFiles()` - Mutation for bulk folder deletion
- Added `mediaFolders` query key for cache management
- Updated `usePictures` and `useMovies` to support `enabled` option

### 5. Media Page UI

**File:** `frontend/src/pages/Media.tsx`

Complete rewrite of view mode system:

#### View Toggle
- Replaced "All / By Date" with "All / Folders"
- Type selector (Pictures/Movies) disabled in Folders view since folders show all media types

#### Folder Browser
- Breadcrumb navigation (Root / subfolder / subfolder)
- Folder cards with yellow folder icon, file count, and total size
- Click to navigate into folder
- "Up to parent folder" button when in subfolder

#### Delete All Feature (Admin Only)
- "Delete All Media" button appears on hover for each folder card
- "Delete All Media in This Folder" button when viewing a folder with files
- Confirmation dialog with clear warnings about what will be deleted
- Shows deletion results (X movies, Y pictures deleted)

#### Gallery Grid
- Works with both FolderFileItem and MediaItem types
- Properly detects file type from folder response
- Thumbnail support for movies
- Preserves existing preview modal, download, and individual delete functionality

## Files Changed

| File | Change |
|------|--------|
| `src/webu_json.hpp` | Added method declarations for folder APIs |
| `src/webu_json.cpp` | Added folder API implementations (~450 lines) |
| `src/webu_ans.cpp` | Added routing for new endpoints |
| `frontend/src/api/types.ts` | Added folder-related type definitions |
| `frontend/src/api/queries.ts` | Added folder queries and updated existing queries |
| `frontend/src/pages/Media.tsx` | Implemented folder browser UI |

## Files Created

| File | Purpose |
|------|---------|
| `doc/plans/20250116-1200-folder-media-api.md` | Design document for folder API |

## Security Considerations

1. **Path Traversal Prevention**
   - Blocks any path containing `..`
   - Uses `realpath()` to resolve symlinks
   - Validates resolved path is within camera's `target_dir`
   - Logs blocked attempts with client IP

2. **Authorization**
   - Folder browsing: Available to all authenticated users
   - Bulk delete: Admin role required
   - Delete action must be enabled in Motion config

3. **File Type Restrictions**
   - Only recognized media extensions are listed/deleted
   - Thumbnails handled automatically (deleted with their parent movie)

## Verification

- TypeScript compilation: Passed
- Frontend build: Passed
- No new lint errors introduced

---

## Future Work

1. **Recursive File Count**: Currently only counts immediate files in a folder, not nested subfolders
2. **Folder Creation/Rename**: Add ability to organize media into folders from UI
3. **Move Files**: Allow moving files between folders
4. **Sort Options**: Add sorting by name, date, size for folder contents
5. **Search Within Folders**: Find files by name pattern within a folder tree

---

# NoIR Camera ColourTemperature Fix

**Date:** 2026-01-16
**Status:** Completed

## Overview

Fixed an issue where the Color Temperature control was incorrectly shown for NoIR (No IR filter) cameras. NoIR cameras lack an IR filter, so color temperature adjustment doesn't work properly on these sensors. The control should be hidden in the UI.

## Problem

The backend was correctly querying libcamera for supported controls, but libcamera reports `ColourTemperature` as supported for NoIR cameras because the sensor hardware technically supports the control. However, without an IR filter, color temperature adjustment produces incorrect results.

**API Response (Before Fix):**
```json
"supportedControls": {
  "ColourTemperature": true,  // Incorrect for NoIR
  ...
}
```

## Root Cause Analysis

1. **Frontend logic was correct**: `LibcameraSettings.tsx` line 107 checks `capabilities?.ColourTemperature !== false`
2. **Backend was passing through raw libcamera data**: `is_control_supported()` only checks if control exists in libcamera's control list
3. **libcamera reports hardware capability, not semantic appropriateness**: The IMX708 sensor hardware supports ColourTemperature, but it doesn't work meaningfully on NoIR variants

## Changes Made

### 1. Backend NoIR Detection

**File:** `src/libcam.hpp`

Added method declaration:
```cpp
/* NoIR camera detection for capability overrides */
bool is_noir_camera();
```

**File:** `src/libcam.cpp`

Added `is_noir_camera()` method:
```cpp
bool cls_libcam::is_noir_camera()
{
    if (!camera) {
        return false;
    }

    /* Get camera model from libcamera properties */
    const ControlList &props = camera->properties();
    auto model_opt = props.get(properties::Model);

    if (model_opt) {
        std::string model(*model_opt);
        std::string model_lower = model;
        std::transform(model_lower.begin(), model_lower.end(),
                       model_lower.begin(), ::tolower);

        if (model_lower.find("noir") != std::string::npos) {
            return true;
        }
    }

    /* Fallback: also check camera ID */
    std::string id = camera->id();
    std::string id_lower = id;
    std::transform(id_lower.begin(), id_lower.end(),
                   id_lower.begin(), ::tolower);

    return id_lower.find("noir") != std::string::npos;
}
```

### 2. Capability Override

**File:** `src/libcam.cpp`

Modified `get_capability_map()` to override ColourTemperature for NoIR cameras:
```cpp
/* NoIR camera overrides:
 * NoIR cameras lack an IR filter, so color temperature adjustment
 * doesn't work properly. Override these capabilities to false.
 */
if (is_noir_camera()) {
    caps["ColourTemperature"] = false;
    MOTION_LOG(NTC, TYPE_VIDEO, NO_ERRNO
        , "NoIR camera detected - disabling ColourTemperature control");
}
```

## Files Changed

| File | Change |
|------|--------|
| `src/libcam.hpp` | Added `is_noir_camera()` method declaration |
| `src/libcam.cpp` | Added `is_noir_camera()` implementation and capability override in `get_capability_map()` |

## Technical Notes

### Why `camera->id()` Wasn't Enough

Initial implementation attempted to use `camera->id()` which returns the I2C bus path:
```
/base/axi/pcie@1000120000/rp1/i2c@88000/imx708@1a
```

This path contains "imx708" but NOT "noir". The full model name (`imx708_wide_noir`) is only available via `camera->properties().get(properties::Model)`.

### Detection Strategy

The implementation uses a two-tier detection:
1. **Primary**: Check `properties::Model` for "noir" substring (case-insensitive)
2. **Fallback**: Check `camera->id()` for "noir" in case Model property is empty

## Verification

**Tested on:** Raspberry Pi 5 with Pi Camera v3 NoIR Wide (`imx708_wide_noir`)

**API Response (After Fix):**
```json
"supportedControls": {
  "ColourTemperature": false,
  ...
}
```

**Log Output:**
```
[NTC][VID][wc00] get_capability_map: NoIR camera detected - disabling ColourTemperature control
```

## Frontend Impact

No frontend changes required. The existing visibility logic in `LibcameraSettings.tsx` correctly handles this:

```tsx
{/* Color Temperature - only show if camera supports it (NoIR cameras don't) */}
{capabilities?.ColourTemperature !== false && (
  <FormSlider label="Color Temperature" ... />
)}

{/* NoIR camera info - only show when capabilities indicate no ColourTemperature */}
{capabilities?.ColourTemperature === false && (
  <div className="text-xs text-gray-400 bg-surface-elevated p-3 rounded">
    <strong>Note:</strong> Color Temperature control is not available on this camera.
    NoIR cameras and some other sensors don't support this feature.
    Use Red/Blue Gain for manual white balance.
  </div>
)}
```

---

# AWB Mode Visibility Investigation

**Date:** 2026-01-16
**Status:** Closed (User Error)

## Overview

Investigated report that AWB (Auto White Balance) modes were not showing for NoIR cameras.

## Finding

This was not a bug. The AWB Mode dropdown is intentionally hidden when the "Auto White Balance" toggle is OFF. This is by design because AWB modes only apply when AWB is enabled.

**Frontend Logic (`LibcameraSettings.tsx:79-101`):**
```tsx
{awbEnabled && (
  <>
    <FormSelect
      label="AWB Mode"
      value={String(getValue('libcam_awb_mode', 0))}
      onChange={(val) => onChange('libcam_awb_mode', Number(val))}
      options={AWB_MODES.map((mode) => ({
        value: String(mode.value),
        label: mode.label,
      }))}
      helpText="White balance mode"
    />
    ...
  </>
)}
```

## Resolution

No code changes required. User was unaware that enabling the "Auto White Balance" toggle reveals the AWB Mode dropdown.

---

# Settings Fields Not Showing Current Values

**Date:** 2026-01-17
**Status:** Completed

## Overview

Fixed an issue where settings fields (Admin Password, Viewer Password, PID File, Log File, Port, Buffer Count, Base Storage path) were blank on page load instead of showing their current configured values.

## Problem

The "Show Current Values in Settings Fields" feature (commit `27330ec`) was implemented but values weren't displaying because the backend API was returning empty strings for RESTRICTED-level parameters.

**Expected Behavior:** Settings fields should display current values from the server until the user edits them.

**Actual Behavior:** Fields were blank on page load, even when credentials and other settings were configured.

## Root Cause Analysis

In `webu_json.cpp:363-365`, parameter values are hidden when:

```cpp
if ((config_parms[indx_parm].webui_level > app->cfg->webcontrol_parms) &&
    (config_parms[indx_parm].webui_level > PARM_LEVEL_LIMITED)) {
    // Returns empty value
}
```

The parameter visibility levels are:
- `PARM_LEVEL_LIMITED = 1`
- `PARM_LEVEL_ADVANCED = 2`
- `PARM_LEVEL_RESTRICTED = 3`

Authentication parameters (`webcontrol_authentication`, `webcontrol_user_authentication`) are `PARM_LEVEL_RESTRICTED (3)`.

With `webcontrol_parms` defaulting to `2` (ADVANCED):
- Condition: `3 > 2` AND `3 > 1` = TRUE AND TRUE = TRUE
- Result: Values returned as empty strings

## Changes Made

### 1. Backend Default Value

**File:** `src/conf.cpp:718`

Changed `webcontrol_parms` default from `2` to `3`:

```cpp
// Before
if (name == "webcontrol_parms") return edit_generic_int(webcontrol_parms, parm, pact, 2, 0, 3);

// After
if (name == "webcontrol_parms") return edit_generic_int(webcontrol_parms, parm, pact, 3, 0, 3);
```

### 2. Config Template

**File:** `data/motion-dist.conf.in:110`

Updated default and comment:

```conf
; Before
;*****   - Parameters restricted to level 1 (limited visibility)
webcontrol_parms 1

; After
;*****   - webcontrol_parms 3 allows admin access to all settings including credentials
webcontrol_parms 3
```

## Files Changed

| File | Change |
|------|--------|
| `src/conf.cpp` | Changed `webcontrol_parms` default from 2 to 3 |
| `data/motion-dist.conf.in` | Updated `webcontrol_parms` to 3 and updated comment |

## Technical Notes

### Parameter Visibility Levels

| Level | Name | Description |
|-------|------|-------------|
| 0 | ALWAYS | Always visible |
| 1 | LIMITED | Basic parameters |
| 2 | ADVANCED | Advanced parameters (paths, ports) |
| 3 | RESTRICTED | Sensitive parameters (credentials, scripts) |
| 99 | NEVER | Never exposed via API |

### Security Consideration

Setting `webcontrol_parms 3` allows admins to view credential values through the API. This is appropriate because:
1. Access to Settings requires admin authentication
2. Admins already have full system access
3. Being able to view/edit credentials through the UI is expected functionality

## Verification

After rebuilding Motion with the fix:
1. API returns actual values for RESTRICTED parameters
2. Settings fields populate with current values on page load
3. "(modified)" indicator correctly shows when values differ from server
4. Values refresh properly after saving

## Workaround for Existing Installations

Users can apply this fix without rebuilding by adding to their `motion.conf`:

```conf
webcontrol_parms 3
```

Then restart Motion.
