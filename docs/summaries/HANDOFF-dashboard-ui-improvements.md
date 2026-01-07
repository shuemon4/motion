# Dashboard UI Improvements - Implementation Handoff

**Date**: 2026-01-05
**Status**: Phase 1 Backend Complete, Frontend Remaining
**Plan Document**: `docs/plans/20260104-dashboard-ui-improvements.md`

## Quick Start

Continue implementing the Dashboard UI improvements plan. Phase 0 (deployment) and Phase 1 backend (dual-user authentication) are complete. Proceed with Phase 1 frontend integration.

## What Has Been Completed

### Phase 0: Deployment ✅

- Synced profiles API code to Pi (192.168.1.176)
- Fixed rsync timestamp issue (source files weren't recompiling)
- Fixed query string routing bug in `src/webu_ans.cpp:126-129`
- Verified profiles API endpoint working: `/0/api/profiles?camera_id=1` returns `{"status":"ok","profiles":[]}`

### Phase 1 Backend: Dual-User Authentication ✅

**Files Modified:**
- `src/conf.cpp` (lines 201, 863-870, 1410-1418)
- `src/conf.hpp` (line 258)
- `src/parm_structs.hpp` (line 67)
- `src/conf_file.cpp` (lines 297-305)
- `src/webu_ans.hpp` (lines 34, 75-76, 79)
- `src/webu_ans.cpp` (lines 685-711, 713-781, 1422-1429, 1477-1480)
- `src/webu_json.cpp` (lines 769-795)

**Backend Changes Implemented:**

1. **New Config Parameter**: `webcontrol_user_authentication`
   - Added to config system with proper validation
   - Secured with log redaction
   - Format: `user:pass` (same as webcontrol_authentication)

2. **Dual-Credential Authentication**:
   - `mhd_auth_parse()` now parses both admin and user credentials
   - `mhd_basic()` checks admin first, falls back to user
   - `mhd_digest()` determines role based on username match
   - Both support HA1 hash passwords (32 hex chars)

3. **Role Tracking**:
   - Added `auth_role` member (public) to `cls_webu_ans`
   - Set to `"admin"` or `"user"` during authentication
   - Initialized to empty string in constructor

4. **API Response**:
   - `/api/auth/me` now returns: `{"authenticated":true,"auth_method":"digest","role":"admin"}`
   - Backward compatible - defaults to "admin" if role not set

**Current Backend State:**
- Binary compiled and deployed on Pi
- Motion process running with new auth code
- `/api/auth/me` endpoint verified returning role field
- Ready for frontend integration

## What Needs to Be Done

### Phase 1 Frontend: Role-Based UI (NEXT)

**Files to Modify:**
1. `frontend/src/contexts/AuthContext.tsx`
2. `frontend/src/components/Layout.tsx`
3. `frontend/src/pages/Dashboard.tsx`
4. `frontend/src/pages/Settings.tsx`

**Implementation Steps:**

1. **Update AuthContext** (`frontend/src/contexts/AuthContext.tsx`):
   ```typescript
   interface AuthState {
     isAuthenticated: boolean;
     role: 'admin' | 'user' | null;  // ADD THIS
     login: (username: string, password: string) => Promise<void>;
     logout: () => void;
   }

   // Update checkAuth() to fetch and store role from /api/auth/me
   const checkAuth = async () => {
     const response = await fetch('/0/api/auth/me');
     const data = await response.json();
     setAuthState({
       isAuthenticated: data.authenticated,
       role: data.role || null  // ADD THIS
     });
   };
   ```

2. **Hide Settings for Non-Admin** (`frontend/src/components/Layout.tsx`):
   ```typescript
   const { role } = useAuthContext();

   {/* Hide Settings nav link for user role */}
   {role === 'admin' && (
     <NavLink to="/settings">Settings</NavLink>
   )}
   ```

3. **Hide Cog Wheel for Non-Admin** (`frontend/src/pages/Dashboard.tsx`):
   ```typescript
   const { role } = useAuthContext();

   {/* Only show settings cog for admin */}
   {role === 'admin' && <SettingsButton cameraId={camera.id} />}
   ```

4. **Protect Settings Page** (`frontend/src/pages/Settings.tsx`):
   ```typescript
   const { role } = useAuthContext();

   if (role !== 'admin') {
     return (
       <div className="p-4">
         <h1>Admin Access Required</h1>
         <p>You must be logged in as an administrator to access settings.</p>
       </div>
     );
   }
   ```

### Phase 2: Profile Management

**Tasks:**
1. Fix ConfigurationPresets error handling (show informative messages)
2. Add ProfileManager to Settings page (load + save functionality)
3. Make Dashboard bottom sheet load-only (remove save button)
4. Add password management UI to System settings

**Key Files:**
- `frontend/src/components/ConfigurationPresets.tsx`
- `frontend/src/pages/Settings.tsx`
- `frontend/src/api/profiles.ts` (already exists)

### Phase 3: Stream Enhancements

**Tasks:**
1. Implement `useFpsTracker` hook (client-side FPS calculation)
2. Add FPS display to camera headers (`1920x1080 @ 15 fps`)
3. Add fullscreen button to headers (keep click-to-expand)
4. Move loading indicators to corners (not center)

**Key Files:**
- `frontend/src/hooks/useFpsTracker.ts` (NEW)
- `frontend/src/components/CameraStream.tsx`
- `frontend/src/pages/Dashboard.tsx`

## Technical Context

### Motion Backend Architecture

- **Camera IDs**: 1-based (0 = "all cameras" context)
- **URL Format**: `/{camera_id}/api/{endpoint}?{params}`
- **Path Segments**: Parsed into `uri_cmd0`, `uri_cmd1`, `uri_cmd2`, etc.
- **Query Params**: Accessed via `MHD_lookup_connection_value()`

### Authentication Flow

```
Request → mhd_auth() → mhd_basic() or mhd_digest()
                           ↓
                    Check admin credentials
                           ↓
                    If fail, check user credentials
                           ↓
                    Set auth_role ("admin" or "user")
                           ↓
                    Return MHD_YES or MHD_NO
```

### Profiles API Endpoints

- `GET /0/api/profiles?camera_id={id}` - List profiles
- `GET /0/api/profiles/{profile_id}` - Get profile with params
- `POST /0/api/profiles` - Create profile
- `PATCH /0/api/profiles/{id}` - Update profile
- `DELETE /0/api/profiles/{id}` - Delete profile
- `POST /0/api/profiles/{id}/apply` - Apply profile to camera
- `POST /0/api/profiles/{id}/default` - Set as default

### Deployment Commands

```bash
# Sync frontend changes
cd frontend && npm run build
rsync -avz dist/ admin@192.168.1.176:~/motion-motioneye/data/webui/

# Sync backend changes (if needed)
rsync -avz --exclude='.git' --exclude='node_modules' \
  /Users/tshuey/Documents/GitHub/motion-motioneye/src/ \
  admin@192.168.1.176:~/motion-motioneye/src/

# Rebuild on Pi
ssh admin@192.168.1.176 "cd ~/motion-motioneye && \
  touch src/[modified files].cpp && make -j4 && \
  sudo make install && sudo pkill -9 motion && \
  sleep 2 && sudo /usr/local/bin/motion -c /etc/motion/motion.conf &"
```

### Testing Endpoints

```bash
# Check auth/me endpoint
curl -s http://192.168.1.176:7999/0/api/auth/me

# Check profiles API
curl -s http://192.168.1.176:7999/0/api/profiles?camera_id=1

# Test with credentials
curl -u admin:password http://192.168.1.176:7999/0/api/config
```

## Critical Issues Resolved

### Issue 1: Query String Routing Bug
**Problem**: `/0/api/profiles?camera_id=1` returned "Bad Request"
**Root Cause**: `parseurl()` didn't strip query strings; `uri_cmd2` was `"profiles?camera_id=1"`
**Fix**: Added query string stripping in `webu_ans.cpp:126-129` before path parsing
**Status**: ✅ Fixed and deployed

### Issue 2: Rsync Timestamp Preservation
**Problem**: Source changes not recompiling after rsync
**Root Cause**: rsync preserves timestamps; make sees .o files as newer than sources
**Fix**: `touch` modified source files before `make`
**Status**: ✅ Documented in `docs/project/MOTION_RESTART.md`

## Configuration Example

To test dual-user auth, update `/etc/motion/motion.conf` on Pi:

```conf
# Admin credentials (full access)
webcontrol_authentication admin:adminpass

# User credentials (view-only)
webcontrol_user_authentication viewer:viewpass
```

Then restart Motion and test with both credential sets.

## Next Action

Start with **Phase 1 Frontend** by updating `AuthContext` to track and expose the `role` field. This is a straightforward change that enables all subsequent role-based UI hiding.

## References

- **Plan**: `docs/plans/20260104-dashboard-ui-improvements.md`
- **Profiles API**: `frontend/src/api/profiles.ts`
- **Restart Procedures**: `docs/project/MOTION_RESTART.md`
- **Pi Access**: `ssh admin@192.168.1.176` (passwordless)
