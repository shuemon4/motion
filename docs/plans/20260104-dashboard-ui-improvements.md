# Dashboard UI Improvements Plan

**Date**: 2026-01-04
**Status**: Draft
**Author**: Claude Code

## Executive Summary

This plan addresses five interconnected issues with the Dashboard and Settings UI, plus role-based access control. Rather than quick patches, we'll implement architectural improvements that solve the root causes and create a more maintainable, feature-complete system.

---

## Critical Clarification: User Roles

**Current State**: Motion backend has single-credential auth (`webcontrol_authentication = user:pass`). Frontend only tracks `isAuthenticated` boolean.

**User Requirements**:
- **Admin role**: Full access (Settings, Quick Settings cog, Media, Video)
- **User role**: Limited access (Video, Media only - NO Settings page, NO cog wheel)

**Proposed Implementation** (simplest approach):
- Authenticated = Admin (full access)
- Unauthenticated = User (view-only access to video/media)

This leverages existing auth without backend changes. The stream must be accessible without auth (public) for User role to work.

**Selected Implementation** (Option B - dual credentials):
- Add `webcontrol_user_authentication` config parameter for view-only users
- Backend returns role (`admin` or `user`) in `/api/auth/me` response
- Admin can update both passwords in System settings on Settings page

---

## Deployment Prerequisite

**CRITICAL**: The profiles API code exists locally but hasn't been deployed to the Pi.

Uncommitted backend files:
- `src/webu_ans.cpp` - profiles routing
- `src/webu_json.cpp` - profiles API handlers
- `src/motion.cpp/hpp` - profiles manager initialization
- `src/conf_profile.cpp/hpp` - profiles SQLite implementation

**Before any testing, must:**
1. Sync code to Pi
2. Rebuild motion binary
3. Restart motion service

---

## Issue Analysis

### Issue 1: "Failed to load profiles" in Dashboard Bottom Sheet

**Current Behavior**: The ConfigurationPresets component in QuickSettings shows "Failed to load profiles" error.

**Root Cause Analysis**:
1. The profiles feature requires SQLite3 support (`HAVE_SQLITE3DB` compile flag)
2. The API endpoint `/0/api/profiles?camera_id=X` returns error if profiles are disabled
3. The frontend error handling is minimal - shows generic error without explaining why

**Evidence**:
- `src/conf_profile.cpp:52-55` - Profiles disabled if SQLite3 not compiled
- `src/conf_profile.cpp:44-50` - Database initialization can fail
- `frontend/src/components/ConfigurationPresets.tsx:42-48` - Minimal error UI

### Issue 2: Profile Configurations Missing from Settings Page

**Current Behavior**: Profiles only appear in Dashboard bottom sheet, not Settings page.

**Requirements**:
- Settings page: Load AND Save capability
- Dashboard bottom sheet: Load only (no Save)
- Both: Display currently active/selected configuration

**Root Cause**: Design decision - ConfigurationPresets was only added to QuickSettings component.

### Issue 3: FPS Display on Video Stream Header

**Current Behavior**: No FPS display. Original MotionEye showed FPS on hover.

**Requirement**: Show actual streaming FPS in the Video Streams header (not on hover).

**Technical Analysis**:
- Motion backend has `stream_maxrate` (configured max) but no actual FPS tracking endpoint
- Client-side FPS must be calculated by counting MJPEG frame arrivals
- MJPEG streams send frames as multipart responses; counting boundary markers gives actual FPS

### Issue 4: Fullscreen Icon in Video Streams Header

**Current Behavior**: Fullscreen is triggered by clicking anywhere on the stream.

**Requirement**: Add an explicit fullscreen icon button in the header while keeping click-to-expand.

### Issue 5: Buffering/Loading Animation Placement

**Current Behavior**: Loading spinner appears centered over the stream.

**Requirement**: Move buffering/loading indicators away from center of viewing area.

**Technical Notes**:
- This is about the custom React loading UI, not browser-native video controls
- MJPEG streams don't use HTML5 `<video>` element - they use `<img>` with streaming src
- The loading state is controlled in `CameraStream.tsx:58-69`

---

## Comprehensive Solutions

### Solution 1: Robust Profiles System with Graceful Degradation

**Approach**: Implement a complete profiles availability check with informative UI states.

#### Backend Changes (Minimal)
Add a profiles availability check endpoint or include status in existing config response:

```cpp
// In webu_json.cpp - add to api_config or create new endpoint
webua->resp_page += ",\"profiles_enabled\":" +
    std::string(app->config_profile && app->config_profile->enabled ? "true" : "false");
```

#### Frontend Changes

1. **New hook: `useProfilesAvailability`**
   - Check if profiles feature is enabled before attempting to load
   - Cache result to avoid repeated checks

2. **Update ConfigurationPresets component**:
   - Show informative message when profiles disabled ("Profiles require SQLite3 support")
   - Distinguish between "disabled feature" vs "no profiles saved yet" vs "network error"

3. **Error state hierarchy**:
   ```
   if (feature disabled) -> "Configuration profiles not available (SQLite3 required)"
   else if (empty list) -> "No presets saved - click Save to create one"
   else if (network error) -> "Failed to load profiles - check connection"
   ```

### Solution 2: Unified Profile Management Component

**Approach**: Create a reusable profile management system that adapts to context.

#### New Component Architecture

```
ProfileManager (container)
├── ProfileSelector (dropdown + apply button) - used in both places
├── ProfileSaveButton (triggers save dialog) - Settings only
└── ProfileSaveDialog (existing) - Settings only
```

#### Implementation

1. **Refactor ConfigurationPresets** → `ProfileManager`
   - Add `mode` prop: `'full'` (Settings) or `'load-only'` (Dashboard)
   - Full mode: Load dropdown + Save button + Apply button
   - Load-only mode: Load dropdown + Apply button only

2. **Add to Settings page**:
   - New section at top of Settings: "Configuration Profiles"
   - Shows currently active profile (if any)
   - Full profile management (load, save, delete, set default)

3. **Track active profile**:
   - Store last-applied profile ID in localStorage or backend
   - Display "Active: [profile name]" indicator in both places

### Solution 3: Real-Time FPS Display

**Approach**: Client-side FPS calculation from MJPEG frame boundaries.

#### How MJPEG FPS Tracking Works

MJPEG streams are multipart MIME responses. Each frame is delimited by a boundary string. By tracking when new frames arrive (via `onload` events on the `<img>` element), we can calculate actual FPS.

#### Implementation

1. **New hook: `useFpsTracker`**
   ```typescript
   function useFpsTracker() {
     const frameTimestamps = useRef<number[]>([])
     const [fps, setFps] = useState<number | null>(null)

     const recordFrame = useCallback(() => {
       const now = performance.now()
       frameTimestamps.current.push(now)
       // Keep only last 2 seconds of timestamps
       const cutoff = now - 2000
       frameTimestamps.current = frameTimestamps.current.filter(t => t > cutoff)

       if (frameTimestamps.current.length >= 2) {
         const elapsed = now - frameTimestamps.current[0]
         const frames = frameTimestamps.current.length - 1
         setFps(Math.round((frames / elapsed) * 1000))
       }
     }, [])

     return { fps, recordFrame }
   }
   ```

2. **Update CameraStream component**:
   - Add `onLoad` handler to stream `<img>` element
   - Each load event = new frame received
   - Calculate rolling average FPS over 2-second window

3. **Update Dashboard header**:
   - Display FPS next to resolution: `1920x1080 @ 15 fps`
   - Show "..." while calculating initial FPS
   - Update every second (not every frame) to avoid UI thrashing

#### Alternative: Backend FPS Tracking

If client-side proves inaccurate, add backend tracking:
- Track frames served per stream connection in `webu_stream.cpp`
- Add `/api/stream/stats` endpoint returning actual FPS
- Poll every 2 seconds from frontend

**Recommendation**: Start with client-side (simpler, no backend changes). Fall back to backend if needed.

### Solution 4: Fullscreen Button in Header

**Approach**: Add explicit fullscreen button while preserving click-to-expand UX.

#### Implementation

1. **Update Dashboard camera header**:
   ```tsx
   <div className="flex items-center gap-2">
     {camera.width && camera.height && (
       <span className="text-xs text-gray-500">
         {camera.width}x{camera.height}
         {fps && ` @ ${fps} fps`}
       </span>
     )}
     <button
       onClick={(e) => {
         e.stopPropagation()
         openFullscreen(camera.id)
       }}
       className="p-1.5 hover:bg-surface rounded-full"
       title="Fullscreen"
     >
       <ExpandIcon className="w-4 h-4 text-gray-400" />
     </button>
     <SettingsButton cameraId={camera.id} />
   </div>
   ```

2. **Modify CameraStream**:
   - Accept optional `onFullscreen` callback prop
   - If provided, use it instead of internal fullscreen state
   - Dashboard controls fullscreen state, CameraStream is presentational

3. **Icon consistency**:
   - Use expand icon (arrows pointing outward) in header
   - Keep existing expand overlay on hover for discoverability

### Solution 5: Non-Intrusive Loading Indicators

**Approach**: Move loading states to corners/edges, not center.

#### Loading State Redesign

1. **Initial load (stream connecting)**:
   - Small spinner in top-left corner of stream area
   - Subtle pulsing border on stream container
   - Dark semi-transparent overlay (not spinner in center)

2. **Stream error**:
   - Error icon + message in bottom-left corner
   - Red accent on container border

3. **Buffering (temporary frame drop)**:
   - Subtle "buffering" indicator in top-right corner
   - Only show if no frame received for >1 second

#### Implementation

```tsx
// Loading overlay - corner placement
{isLoading && (
  <div className="absolute top-2 left-2 flex items-center gap-2
                  bg-black/60 rounded-full px-2 py-1">
    <SpinnerIcon className="w-4 h-4 animate-spin" />
    <span className="text-xs">Connecting...</span>
  </div>
)}

// Error indicator - bottom placement
{error && (
  <div className="absolute bottom-2 left-2 right-2
                  bg-red-900/80 rounded px-3 py-2 text-sm">
    {error}
  </div>
)}
```

#### Chrome-Specific Considerations

Chrome's native video controls cannot be repositioned for `<video>` elements. However:
- MJPEG uses `<img>` elements, not `<video>` - no native controls
- All loading UI is custom React components - fully controllable
- The play/pause that appears is likely from browser extensions or misidentification

**Verification needed**: Confirm the user is seeing our custom loading states, not browser-injected controls.

---

## Implementation Order

### Phase 1: Foundation (Issues 1 & 2)
1. Add profiles availability check to backend config response
2. Refactor ConfigurationPresets → ProfileManager with mode support
3. Add ProfileManager to Settings page
4. Implement graceful degradation with informative messages

### Phase 2: Stream Enhancements (Issues 3, 4, 5)
1. Implement useFpsTracker hook
2. Add FPS display to camera headers
3. Add fullscreen button to headers
4. Refactor loading states to corner placement

### Phase 3: Polish
1. Test all states (profiles enabled/disabled, various errors)
2. Ensure mobile responsiveness
3. Add localStorage persistence for active profile
4. Performance optimization (debounce FPS updates)

---

## Files to Modify

### Backend (C++)
- `src/webu_json.cpp` - Add profiles_enabled to config response

### Frontend (TypeScript/React)
- `frontend/src/hooks/useProfiles.ts` - Add availability check
- `frontend/src/hooks/useFpsTracker.ts` - **NEW** - FPS tracking hook
- `frontend/src/components/ConfigurationPresets.tsx` → Refactor to ProfileManager
- `frontend/src/components/ProfileManager.tsx` - **NEW** - Unified profile component
- `frontend/src/components/CameraStream.tsx` - FPS tracking, loading states
- `frontend/src/pages/Dashboard.tsx` - FPS display, fullscreen button, header updates
- `frontend/src/pages/Settings.tsx` - Add ProfileManager section

---

## Testing Checklist

- [ ] Profiles disabled (no SQLite3): Shows informative message
- [ ] Profiles enabled, empty: Shows "No presets" message
- [ ] Profiles enabled, with profiles: Dropdown works
- [ ] Settings page Load: Applies profile correctly
- [ ] Settings page Save: Creates new profile
- [ ] Dashboard Load-only: No save button visible
- [ ] FPS display: Shows reasonable values (matches stream_maxrate approximately)
- [ ] FPS display: Updates smoothly without flickering
- [ ] Fullscreen button: Opens fullscreen on click
- [ ] Click on stream: Still opens fullscreen (backward compatible)
- [ ] Loading states: Appear in corner, not center
- [ ] Error states: Appear in corner, not center
- [ ] Mobile: All features work on small screens

---

## Solution 6: Dual-User Authentication System

**Approach**: Implement Admin and User roles with separate credentials.

### Backend Changes

1. **New config parameters** (`src/conf.cpp`, `src/parm_structs.hpp`):
   ```cpp
   // Existing - now for admin only
   {"webcontrol_authentication", PARM_TYP_STRING, PARM_CAT_13, PARM_LEVEL_RESTRICTED, false}

   // New - for view-only users
   {"webcontrol_user_authentication", PARM_TYP_STRING, PARM_CAT_13, PARM_LEVEL_RESTRICTED, false}
   ```

2. **Auth logic update** (`src/webu_ans.cpp`):
   - Try admin credentials first → role = "admin"
   - If fails, try user credentials → role = "user"
   - If both fail → unauthenticated (401)
   - Store role in connection context

3. **API response update** (`src/webu_json.cpp`):
   ```cpp
   void cls_webu_json::api_auth_me() {
       webua->resp_page = "{";
       webua->resp_page += "\"authenticated\":true,";
       webua->resp_page += "\"role\":\"" + webua->auth_role + "\",";  // NEW
       webua->resp_page += "\"auth_method\":\"digest\"";
       webua->resp_page += "}";
   }
   ```

4. **Password update endpoint** (admin only):
   ```
   PATCH /0/api/config
   Body: { "webcontrol_authentication": "newpass", "webcontrol_user_authentication": "newpass" }
   ```

### Frontend Changes

1. **AuthContext updates**:
   ```tsx
   interface AuthState {
     isAuthenticated: boolean
     role: 'admin' | 'user' | null  // NEW
     ...
   }
   ```

2. **Component visibility**:
   ```tsx
   // Dashboard header - hide cog for non-admin
   const { role } = useAuthContext()
   {role === 'admin' && <SettingsButton cameraId={camera.id} />}
   ```

3. **Route protection** (Settings page):
   - Show "Admin access required" for User role
   - Prompt login when accessing Settings

4. **Layout updates**:
   - Hide Settings nav link for User role
   - Show Media link for all authenticated users

5. **System Settings section** (Admin only):
   - Change Admin password
   - Change User password

---

## Decisions Made

1. **User authentication**: Option B - Dual credentials (Admin + User)
2. **Configuration presets storage**: Option B - Backend SQLite (existing implementation)

---

## Implementation Order (Updated)

### Phase 0: Deploy Existing Code
1. Sync current code to Pi (includes profiles API)
2. Rebuild motion binary on Pi
3. Restart motion service
4. Verify `/0/api/profiles?camera_id=1` returns JSON

### Phase 1: Dual-User Authentication
1. Add `webcontrol_user_authentication` config parameter
2. Update auth logic to check both credentials and determine role
3. Update `/api/auth/me` to return role
4. Update frontend AuthContext with role tracking
5. Add role-based UI visibility (hide cog/Settings from User)

### Phase 2: Profile Management
1. Fix ConfigurationPresets error handling
2. Add ProfileManager to Settings page
3. Make Dashboard bottom sheet load-only
4. Add password management to System settings

### Phase 3: Stream Enhancements
1. Add FPS tracking hook
2. Add FPS display to camera headers
3. Add fullscreen button to headers
4. Move loading indicators to corners

---

## Estimated Complexity

| Task | Complexity | Risk |
|------|------------|------|
| Deploy existing profiles code | Low | Low |
| Dual-user auth (backend) | Medium | Medium |
| Dual-user auth (frontend) | Medium | Low |
| ProfileManager refactor | Medium | Medium |
| Settings page integration | Low | Low |
| Password management UI | Low | Low |
| FPS tracking hook | Medium | Low |
| Fullscreen button | Low | Low |
| Loading state relocation | Low | Low |

**Total estimated effort**: Medium-High due to dual-user auth backend changes.
