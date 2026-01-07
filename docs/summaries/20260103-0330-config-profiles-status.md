# Configuration Profiles Implementation Status
**Date**: 2026-01-03 03:30
**Feature**: Configuration Profile System
**Plan Document**: `docs/plans/20260103-0230-configuration-profiles.md`

## Overview

Implementation of configuration profiles feature allowing users to save and quickly switch between camera settings (libcamera controls, motion detection, framerate) via the Dashboard Quick Settings panel.

## Completion Status: ~85% (Phases 1-3 Complete, Phase 4 Pending)

**Updated**: 2026-01-03 (Following session from original status document)
- ✅ Phase 1: Backend Foundation (COMPLETE - from previous session)
- ✅ Phase 2: API Endpoints (COMPLETE - this session)
- ✅ Phase 3: Frontend Components (COMPLETE - this session)
- ⏳ Phase 4: Integration & Testing (PENDING - requires Pi deployment)

## What Was Completed This Session

### Backend Implementation (Phase 2)
1. **7 API Endpoint Handlers** in `src/webu_json.cpp` (~400 lines)
   - All GET, POST, PATCH, DELETE operations implemented
   - CSRF validation for all write operations
   - Proper JSON response formatting
   - Error handling and logging (NTC level)

2. **Route Wiring** in `src/webu_ans.cpp` (~60 lines)
   - GET routes for list/get operations
   - POST routes for create/apply/set-default
   - PATCH routes for update operations
   - DELETE routes for profile deletion
   - Proper request body accumulation for JSON endpoints

3. **Build System** (`src/Makefile.am`)
   - Added conf_profile.hpp and conf_profile.cpp to source list
   - ✅ Successful compilation with SQLite3 support

### Frontend Implementation (Phase 3)
1. **API Client** (`frontend/src/api/profiles.ts` - 140 lines)
   - TypeScript interfaces for all API data types
   - 7 API methods matching backend endpoints
   - Proper error handling using existing apiGet/Post/Patch/Delete

2. **React Query Hooks** (`frontend/src/hooks/useProfiles.ts` - 170 lines)
   - Complete CRUD hooks with TanStack Query
   - Optimistic UI updates for delete/set-default
   - Proper cache invalidation and query key management
   - Loading and error state management

3. **UI Components**
   - **ConfigurationPresets.tsx** (133 lines) - Main profile selector with save/apply
   - **ProfileSaveDialog.tsx** (123 lines) - Modal for creating new profiles
   - ✅ Successful frontend build (Vite)

### Build Verification
- ✅ Backend: Motion binary compiled and linked successfully
- ✅ Frontend: TypeScript + Vite build passed, assets in data/webui/
- ✅ No compilation errors or warnings (except unrelated doc file)

---

### ✅ Phase 1: Backend Foundation (COMPLETE)

#### Files Created
1. **`src/conf_profile.hpp`** (105 lines)
   - Class declaration for `cls_config_profile`
   - Profile metadata structure `ctx_profile_info`
   - Full CRUD operation signatures
   - Thread-safe with pthread mutex

2. **`src/conf_profile.cpp`** (650+ lines)
   - SQLite database initialization with schema:
     - `config_profiles` table (metadata)
     - `config_profile_params` table (key-value params)
     - Indexes and triggers for single default per camera
   - Profile CRUD operations:
     - `create_profile()` - Create with transaction support
     - `load_profile()` - Load parameters into map
     - `update_profile()` - Update params with timestamp
     - `delete_profile()` - Delete with cascade
   - Profile queries:
     - `list_profiles()` - List all profiles for camera
     - `get_profile_info()` - Get metadata by ID
     - `get_default_profile()` / `set_default_profile()`
   - Configuration operations:
     - `snapshot_config()` - Extract 31 profileable params
     - `apply_profile()` - Apply to config with hot-reload
     - `is_profileable_param()` - Filter logic
   - Stub implementations when SQLite3 not available

#### Files Modified
1. **`src/motion.hpp`**
   - Added `cls_config_profile` forward declaration (line 99)
   - Added `cls_config_profile *profiles` member to `cls_motapp` (line 200)

2. **`src/motion.cpp`**
   - Added `#include "conf_profile.hpp"` (line 22)
   - Added `profiles = new cls_config_profile(this);` in init (line 544)
   - Added `mydelete(profiles);` in deinit (line 573)

3. **`src/webu_json.hpp`**
   - Added 7 profile API method declarations (lines 44-51):
     - `api_profiles_list()`
     - `api_profiles_get()`
     - `api_profiles_create()`
     - `api_profiles_update()`
     - `api_profiles_delete()`
     - `api_profiles_apply()`
     - `api_profiles_set_default()`

#### Profileable Parameters (31 total)

**Libcamera Controls (14 params)**:
```
libcam_brightness, libcam_contrast, libcam_iso
libcam_awb_enable, libcam_awb_mode, libcam_awb_locked
libcam_colour_temp, libcam_colour_gain_r, libcam_colour_gain_b
libcam_af_mode, libcam_lens_position, libcam_af_range, libcam_af_speed
libcam_params
```

**Motion Detection (16 params)**:
```
threshold, threshold_maximum, threshold_sdevx, threshold_sdevy
threshold_sdevxy, threshold_ratio, threshold_ratio_change, threshold_tune
noise_level, noise_tune, despeckle_filter, area_detect
lightswitch_percent, lightswitch_frames, minimum_motion_frames, event_gap
```

**Device Settings (1 param)**:
```
framerate (requires camera restart)
```

#### Database Schema
```sql
CREATE TABLE config_profiles (
    profile_id INTEGER PRIMARY KEY AUTOINCREMENT,
    camera_id INTEGER NOT NULL DEFAULT 0,
    profile_name TEXT NOT NULL,
    description TEXT,
    is_default BOOLEAN NOT NULL DEFAULT 0,
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL,
    UNIQUE(camera_id, profile_name)
);

CREATE TABLE config_profile_params (
    param_id INTEGER PRIMARY KEY AUTOINCREMENT,
    profile_id INTEGER NOT NULL,
    param_name TEXT NOT NULL,
    param_value TEXT NOT NULL,
    FOREIGN KEY(profile_id) REFERENCES config_profiles(profile_id) ON DELETE CASCADE,
    UNIQUE(profile_id, param_name)
);

-- Indexes and triggers for performance and single-default enforcement
```

**Database Location**: `<target_dir>/config_profiles.db` or `<config_dir>/config_profiles.db`

---

### 🔄 Phase 2: API Endpoints (IN PROGRESS)

#### Status
- Method declarations added to `webu_json.hpp` ✅
- Method implementations needed in `webu_json.cpp` ⏳
- Route wiring needed in `webu_ans.cpp` ⏳
- Build system update needed (`src/Makefile.am`) ⏳

#### Required API Endpoints

1. **GET /0/api/profiles?camera_id=X** (`api_profiles_list()`)
   - List all profiles for specified camera
   - Returns: Array of profile metadata with param counts
   - No CSRF required (read-only)

2. **GET /0/api/profiles/{id}** (`api_profiles_get()`)
   - Get specific profile with all parameters
   - Returns: Profile metadata + params object
   - No CSRF required (read-only)

3. **POST /0/api/profiles** (`api_profiles_create()`)
   - Create new profile from current config or provided params
   - Body: `{name, description, camera_id, snapshot_current?, params?}`
   - Returns: Created profile with ID
   - CSRF required

4. **PATCH /0/api/profiles/{id}** (`api_profiles_update()`)
   - Update profile parameters
   - Body: `{params: {...}}`
   - Returns: Success status
   - CSRF required

5. **DELETE /0/api/profiles/{id}** (`api_profiles_delete()`)
   - Delete profile (cascades to params)
   - Returns: Success status
   - CSRF required

6. **POST /0/api/profiles/{id}/apply** (`api_profiles_apply()`)
   - Apply profile to camera config
   - Returns: Success + requires_restart array
   - CSRF required

7. **POST /0/api/profiles/{id}/default** (`api_profiles_set_default()`)
   - Set as default profile for camera
   - Returns: Success status
   - CSRF required

#### Route Wiring Pattern (webu_ans.cpp)

**For GET requests** (around line 1025-1054):
```cpp
} else if (uri_cmd2 == "profiles") {
    if (uri_cmd3.empty()) {
        webu_json->api_profiles_list();  // GET /0/api/profiles
    } else if (uri_cmd4.empty()) {
        webu_json->api_profiles_get();   // GET /0/api/profiles/{id}
    } else {
        bad_request();
    }
    mhd_send();
```

**For POST requests** (add to POST section around line 1175-1207):
```cpp
} else if (uri_cmd1 == "api" && uri_cmd2 == "profiles") {
    if (uri_cmd3.empty()) {
        // POST /0/api/profiles (create)
    } else if (uri_cmd4 == "apply") {
        // POST /0/api/profiles/{id}/apply
    } else if (uri_cmd4 == "default") {
        // POST /0/api/profiles/{id}/default
    }
```

**For PATCH requests** (add to PATCH section around line 1210-1237):
```cpp
} else if (uri_cmd1 == "api" && uri_cmd2 == "profiles" && !uri_cmd3.empty()) {
    webu_json->api_profiles_update();  // PATCH /0/api/profiles/{id}
```

**For DELETE requests** (add to DELETE section around line 945-980):
```cpp
} else if (uri_cmd1 == "api" && uri_cmd2 == "profiles" && !uri_cmd3.empty()) {
    webu_json->api_profiles_delete();  // DELETE /0/api/profiles/{id}
```

#### Build System Update

**File**: `src/Makefile.am`

Add to source list:
```makefile
conf_profile.cpp \
conf_profile.hpp \
```

---

### ⏳ Phase 3: Frontend Components (PENDING)

#### UI Integration Point
- **Location**: Dashboard → BottomSheet (Quick Settings) → ConfigurationPresets
- **Existing Placeholder**: `frontend/src/components/ConfigurationPresets.tsx`
- **Current State**: Disabled dropdown with "Coming Soon" message

#### Files to Create

1. **`frontend/src/api/profiles.ts`**
   - API client for all 7 endpoints
   - TypeScript interfaces for Profile, CreateProfileData, etc.
   - Uses fetch with proper headers (CSRF token)

2. **`frontend/src/hooks/useProfiles.ts`**
   - React Query hooks:
     - `useProfiles(cameraId)` - List profiles
     - `useProfile(profileId)` - Get specific profile
     - `useCreateProfile()` - Create mutation
     - `useUpdateProfile()` - Update mutation
     - `useDeleteProfile()` - Delete mutation
     - `useApplyProfile()` - Apply mutation
     - `useSetDefaultProfile()` - Set default mutation
   - Query key management
   - Cache invalidation on mutations

3. **`frontend/src/components/ProfileSaveDialog.tsx`**
   - Modal dialog for saving current settings as profile
   - Form fields: name (required), description (optional), set as default (checkbox)
   - Validation and error handling
   - Uses existing modal/dialog components

#### Files to Modify

1. **`frontend/src/components/ConfigurationPresets.tsx`**
   - Replace placeholder with functional implementation
   - Profile dropdown (loads from API)
   - Save button (opens ProfileSaveDialog)
   - Apply button (applies selected profile)
   - Optional: Manage button (delete, rename, set default)
   - Loading states, error states, success feedback

2. **`frontend/src/types/api.ts`** (if exists)
   - Add TypeScript interfaces for profiles

#### Visual Design

Current placeholder layout:
```
┌─────────────────────────────────────────────────┐
│ Configuration Preset                             │
│ ┌───────────────────┐ ┌──────┐ ┌───────┐        │
│ │ Select Profile ▼  │ │ Save │ │ Apply │        │
│ └───────────────────┘ └──────┘ └───────┘        │
│ Quick switch camera settings • 5 presets         │
└─────────────────────────────────────────────────┘
```

Functional implementation:
- Dropdown shows profile names (with ⭐ for default)
- Save button opens dialog to create new profile
- Apply button enabled when profile selected
- Show "Custom (modified)" when settings changed after apply
- Loading spinner during API calls
- Success/error toasts for user feedback

---

### ⏳ Phase 4: Integration & Testing (PENDING)

#### Build Verification
1. Verify `conf_profile.cpp` compiles with SQLite3 support
2. Check for missing includes or undefined references
3. Ensure Motion links with updated object files

#### Functional Testing
1. **Database Operations**:
   - Database created on first run
   - Profile CRUD operations work
   - Constraints enforced (unique names, single default)
   - Transactions rollback on error

2. **API Endpoints**:
   - All 7 endpoints accessible
   - CSRF validation works
   - Error responses formatted correctly
   - Profile operations reflected in database

3. **Frontend Integration**:
   - Profiles load in dropdown
   - Save dialog creates profiles
   - Apply updates settings in Quick Settings panel
   - Delete prompts confirmation
   - Loading states display correctly

4. **End-to-End Workflow** (on Pi):
   - Create "Daytime" profile with bright settings
   - Create "Nighttime" profile with low-light settings
   - Switch between profiles
   - Verify camera applies settings correctly
   - Verify motion detection uses new thresholds
   - Restart camera after framerate change

#### Performance Testing (Pi)
- Profile load time < 200ms
- No camera glitches during switch
- No memory leaks after 100 switches
- Database size reasonable (<1MB for 50 profiles)

---

## Implementation Metrics

### Code Written
- **Backend**: ~850 lines (conf_profile.hpp + conf_profile.cpp)
- **Integration**: ~10 lines (motion.hpp, motion.cpp, webu_json.hpp)
- **Frontend**: 0 lines (pending)
- **Total**: ~860 lines

### Code Remaining
- **Backend API**: ~500 lines (webu_json.cpp implementations)
- **Backend Routing**: ~100 lines (webu_ans.cpp routing)
- **Frontend**: ~600 lines (API client, hooks, components)
- **Total Remaining**: ~1200 lines

### Estimated Time
- **Phase 2 Completion**: 1-2 hours
- **Phase 3 Completion**: 2-3 hours
- **Phase 4 Testing**: 1-2 hours
- **Total Remaining**: 4-7 hours

---

## Known Issues / TODOs

### Backend ✅ (Phase 2 COMPLETE)
- [x] Add Makefile.am entries for new source files
- [x] Implement 7 API endpoint handlers in webu_json.cpp
- [x] Wire up routes in webu_ans.cpp for all HTTP methods
- [x] Verify compilation with SQLite3 support (local Mac build successful)
- [x] Add logging for profile operations (NTC level) - implemented in all handlers
- [ ] Test database initialization on first run (requires Pi deployment)
- [ ] Verify parameterized SQL prevents injection (code review shows proper usage, needs runtime testing)

### Frontend ✅ (Phase 3 COMPLETE)
- [x] Create TypeScript interfaces matching backend structures
- [x] Implement API client with proper error handling
- [x] Create React Query hooks with cache management
- [x] Replace ConfigurationPresets placeholder with functional component
- [x] Create ProfileSaveDialog modal
- [x] Verify frontend build successful (Vite build passed, assets in data/webui/)
- [ ] Add visual feedback (toasts, loading spinners) - basic feedback implemented, toast notifications pending
- [ ] Handle restart warning for framerate changes - console warning implemented, UI notification pending

### Testing ⏳ (Phase 4 PENDING)
- [ ] Test database initialization on first run on Pi
- [ ] Test all CRUD operations via curl/Postman on Pi
- [ ] Test frontend profile workflows in browser
- [ ] Deploy to Pi and test end-to-end
- [ ] Performance testing (load time, memory usage)
- [ ] Edge case testing (long names, many profiles, concurrent access)
- [ ] Test CSRF token refresh after Motion restart
- [ ] Test profile apply with settings that require camera restart

### Future Enhancements (Post-MVP)
- [ ] Toast notifications for success/error feedback (currently console only)
- [ ] Profile management UI (edit name/description, delete confirmation dialog)
- [ ] Profile export/import for backup and sharing
- [ ] Show which settings changed when applying a profile
- [ ] Profile diff view to compare profiles
- [ ] Batch apply profiles to multiple cameras

---

## Dependencies

### Backend
- SQLite3 library (`HAVE_SQLITE3DB` flag)
- Existing config system (`cls_config`)
- Existing API framework (`cls_webu_json`)

### Frontend
- React 18 + TypeScript
- TanStack Query (already in use)
- Existing BottomSheet component
- Existing form components (FormSlider, FormToggle)

---

## References

- **Implementation Plan**: `docs/plans/20260103-0230-configuration-profiles.md`
- **Research Document**: `docs/scratchpads/20260103-config-profiles-research.md`
- **Settings Sliders Analysis**: `docs/analysis/settings-sliders.md`

---

## Next Session Priorities

1. **Immediate**: Deploy to Pi for Phase 4 testing (**Ask if Pi is powered on first!**)
2. **High**: Verify database initialization and all CRUD operations work on Pi
3. **High**: Test full UI workflow (save → apply → switch profiles)
4. **Medium**: Test edge cases and performance on Pi hardware
5. **Low**: Implement toast notifications for better user feedback
6. **Low**: Add profile management features (edit, delete confirmation)

---

**Status**: ✅ **Phases 2 & 3 COMPLETE**. Backend API (7 endpoints) and Frontend UI (components + hooks) fully implemented and compiled successfully. Ready for deployment and end-to-end testing on Raspberry Pi.
