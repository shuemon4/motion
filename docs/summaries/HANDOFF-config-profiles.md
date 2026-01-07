# Configuration Profiles Implementation - Handoff Prompt

## Context

This is a continuation of the configuration profiles feature implementation for the Motion-MotionEye project. Phase 1 (backend foundation) is complete. This prompt will guide you to complete Phases 2-4.

**Read these files first to understand the implementation:**
1. `docs/summaries/20260103-0330-config-profiles-status.md` - Current status and what's done
2. `docs/plans/20260103-0230-configuration-profiles.md` - Complete implementation plan
3. `src/conf_profile.hpp` and `src/conf_profile.cpp` - Backend implementation (review to understand API)

## What's Already Done (Phase 1)

✅ Backend profile manager class (`cls_config_profile`) with full CRUD operations
✅ SQLite database schema with profiles and params tables
✅ Integration into `cls_motapp` (initialization and cleanup)
✅ API method declarations in `webu_json.hpp`
✅ Support for 31 profileable parameters (libcamera, motion detection, framerate)

## Your Task: Complete Phases 2-4

### Phase 2: API Endpoints (Priority 1 - Start Here)

**Task 2.1**: Implement 7 API endpoint handlers in `src/webu_json.cpp`

Add these implementations **before the `void cls_webu_json::main()` function** (around line 1776):

**Implementation Pattern** (follow existing API handlers like `api_config_patch()`, `api_cameras()`, `api_media_pictures()`):

1. **`api_profiles_list()`** - GET /0/api/profiles?camera_id=X
   ```cpp
   void cls_webu_json::api_profiles_list()
   {
       webua->resp_type = WEBUI_RESP_JSON;

       // Get camera_id from query params (webua->uri_query or default to 0)
       int camera_id = 0;  // Parse from query string

       // Call app->profiles->list_profiles(camera_id)
       // Build JSON response with profile array
       // Include: profile_id, camera_id, name, description, is_default, created_at, updated_at, param_count
   }
   ```

2. **`api_profiles_get()`** - GET /0/api/profiles/{id}
   ```cpp
   void cls_webu_json::api_profiles_get()
   {
       webua->resp_type = WEBUI_RESP_JSON;

       // Parse profile_id from webua->uri_cmd3
       int profile_id = atoi(webua->uri_cmd3.c_str());

       // Call app->profiles->get_profile_info() and load_profile()
       // Build JSON with metadata + params object
   }
   ```

3. **`api_profiles_create()`** - POST /0/api/profiles
   ```cpp
   void cls_webu_json::api_profiles_create()
   {
       webua->resp_type = WEBUI_RESP_JSON;

       // Validate CSRF token (see api_config_patch for pattern)
       // Parse JSON body (webua->raw_body) using JsonParser
       // Extract: name, description, camera_id, snapshot_current, params

       // If snapshot_current: get params from app->profiles->snapshot_config()
       // Call app->profiles->create_profile()
       // Return: {"status":"ok", "profile_id": X}
   }
   ```

4. **`api_profiles_update()`** - PATCH /0/api/profiles/{id}
   ```cpp
   void cls_webu_json::api_profiles_update()
   {
       webua->resp_type = WEBUI_RESP_JSON;

       // Validate CSRF
       // Parse profile_id from uri_cmd3
       // Parse JSON body for params
       // Call app->profiles->update_profile()
   }
   ```

5. **`api_profiles_delete()`** - DELETE /0/api/profiles/{id}
   ```cpp
   void cls_webu_json::api_profiles_delete()
   {
       webua->resp_type = WEBUI_RESP_JSON;

       // Validate CSRF
       // Parse profile_id
       // Call app->profiles->delete_profile()
   }
   ```

6. **`api_profiles_apply()`** - POST /0/api/profiles/{id}/apply
   ```cpp
   void cls_webu_json::api_profiles_apply()
   {
       webua->resp_type = WEBUI_RESP_JSON;

       // Validate CSRF
       // Parse profile_id from uri_cmd3
       // Get camera config (webua->cam->cfg or app->cfg)
       // Call app->profiles->apply_profile(cfg, profile_id)
       // Returns vector of params needing restart
       // Return: {"status":"ok", "requires_restart": ["framerate"]}
   }
   ```

7. **`api_profiles_set_default()`** - POST /0/api/profiles/{id}/default
   ```cpp
   void cls_webu_json::api_profiles_set_default()
   {
       webua->resp_type = WEBUI_RESP_JSON;

       // Validate CSRF
       // Parse profile_id
       // Call app->profiles->set_default_profile()
   }
   ```

**Helper Notes**:
- CSRF validation pattern: See `api_config_patch()` line 1350-1357
- JSON parsing: Use `JsonParser` class (see `api_config_patch()` line 1360-1367)
- Profile manager access: `app->profiles->method_name()`
- Camera config access: `webua->cam->cfg` or `app->cfg`
- Response format: `{"status":"ok"}` or `{"status":"error","message":"..."}`

**Task 2.2**: Wire up routes in `src/webu_ans.cpp`

**For GET requests** - Add to the `else if (uri_cmd1 == "api")` section (around line 1025-1054):

```cpp
} else if (uri_cmd2 == "profiles") {
    if (uri_cmd3.empty()) {
        // GET /0/api/profiles?camera_id=X
        webu_json->api_profiles_list();
    } else {
        // GET /0/api/profiles/{id}
        webu_json->api_profiles_get();
    }
    mhd_send();
```

**For POST requests** - Add to POST handling section (around line 1175-1207):

```cpp
} else if (uri_cmd1 == "api" && uri_cmd2 == "profiles") {
    if (*upload_data_size > 0) {
        raw_body.append(upload_data, *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
    }
    if (webu_json == nullptr) {
        webu_json = new cls_webu_json(this);
    }
    if (uri_cmd3.empty()) {
        // POST /0/api/profiles (create)
        webu_json->api_profiles_create();
    } else if (uri_cmd4 == "apply") {
        // POST /0/api/profiles/{id}/apply
        webu_json->api_profiles_apply();
    } else if (uri_cmd4 == "default") {
        // POST /0/api/profiles/{id}/default
        webu_json->api_profiles_set_default();
    }
    mhd_send();
```

**For PATCH requests** - Add to PATCH section (around line 1210-1237):

```cpp
} else if (uri_cmd1 == "api" && uri_cmd2 == "profiles" && !uri_cmd3.empty()) {
    if (webu_json == nullptr) {
        webu_json = new cls_webu_json(this);
    }
    webu_json->api_profiles_update();
    mhd_send();
```

**For DELETE requests** - Add to DELETE section (around line 945-980):

```cpp
} else if (uri_cmd1 == "api" && uri_cmd2 == "profiles" && !uri_cmd3.empty()) {
    if (webu_json == nullptr) {
        webu_json = new cls_webu_json(this);
    }
    // CSRF validation done inside api_profiles_delete()
    webu_json->api_profiles_delete();
    mhd_send();
```

**Task 2.3**: Update build system

Add to `src/Makefile.am` in the source list:
```makefile
conf_profile.cpp \
conf_profile.hpp \
```

**Task 2.4**: Test compilation

```bash
autoreconf -fiv
./configure --with-libcam --with-sqlite3
make -j4
```

Fix any compilation errors. Check for:
- Missing includes
- Undefined references
- SQLite3 linking issues

---

### Phase 3: Frontend Components (Priority 2)

**Task 3.1**: Create API client `frontend/src/api/profiles.ts`

```typescript
export interface Profile {
  profile_id: number;
  camera_id: number;
  name: string;
  description?: string;
  is_default: boolean;
  created_at: number;
  updated_at: number;
  param_count: number;
}

export interface ProfileWithParams extends Profile {
  params: Record<string, string>;
}

export interface CreateProfileData {
  name: string;
  description?: string;
  camera_id: number;
  snapshot_current?: boolean;
  params?: Record<string, string>;
}

export const profilesApi = {
  list: async (cameraId: number): Promise<Profile[]> => {
    const response = await fetch(`/0/api/profiles?camera_id=${cameraId}`);
    if (!response.ok) throw new Error('Failed to fetch profiles');
    return response.json();
  },

  get: async (profileId: number): Promise<ProfileWithParams> => {
    const response = await fetch(`/0/api/profiles/${profileId}`);
    if (!response.ok) throw new Error('Failed to fetch profile');
    return response.json();
  },

  create: async (data: CreateProfileData): Promise<{ profile_id: number }> => {
    const csrf = getCsrfToken(); // Import from existing csrf module
    const response = await fetch('/0/api/profiles', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'X-CSRF-Token': csrf,
      },
      body: JSON.stringify(data),
    });
    if (!response.ok) throw new Error('Failed to create profile');
    return response.json();
  },

  apply: async (profileId: number): Promise<{ requires_restart: string[] }> => {
    const csrf = getCsrfToken();
    const response = await fetch(`/0/api/profiles/${profileId}/apply`, {
      method: 'POST',
      headers: { 'X-CSRF-Token': csrf },
    });
    if (!response.ok) throw new Error('Failed to apply profile');
    return response.json();
  },

  delete: async (profileId: number): Promise<void> => {
    const csrf = getCsrfToken();
    const response = await fetch(`/0/api/profiles/${profileId}`, {
      method: 'DELETE',
      headers: { 'X-CSRF-Token': csrf },
    });
    if (!response.ok) throw new Error('Failed to delete profile');
  },

  setDefault: async (profileId: number): Promise<void> => {
    const csrf = getCsrfToken();
    const response = await fetch(`/0/api/profiles/${profileId}/default`, {
      method: 'POST',
      headers: { 'X-CSRF-Token': csrf },
    });
    if (!response.ok) throw new Error('Failed to set default');
  },
};
```

**Task 3.2**: Create React Query hooks `frontend/src/hooks/useProfiles.ts`

```typescript
import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query';
import { profilesApi, type CreateProfileData } from '@/api/profiles';

export function useProfiles(cameraId: number) {
  return useQuery({
    queryKey: ['profiles', cameraId],
    queryFn: () => profilesApi.list(cameraId),
  });
}

export function useCreateProfile() {
  const queryClient = useQueryClient();
  return useMutation({
    mutationFn: profilesApi.create,
    onSuccess: (_, variables) => {
      queryClient.invalidateQueries({ queryKey: ['profiles', variables.camera_id] });
    },
  });
}

export function useApplyProfile() {
  const queryClient = useQueryClient();
  return useMutation({
    mutationFn: profilesApi.apply,
    onSuccess: () => {
      // Invalidate config cache to trigger re-fetch
      queryClient.invalidateQueries({ queryKey: ['config-quick'] });
    },
  });
}

export function useDeleteProfile() {
  const queryClient = useQueryClient();
  return useMutation({
    mutationFn: profilesApi.delete,
    onSuccess: (_, profileId) => {
      // Invalidate all profile lists
      queryClient.invalidateQueries({ queryKey: ['profiles'] });
    },
  });
}

export function useSetDefaultProfile() {
  const queryClient = useQueryClient();
  return useMutation({
    mutationFn: profilesApi.setDefault,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['profiles'] });
    },
  });
}
```

**Task 3.3**: Replace placeholder in `frontend/src/components/ConfigurationPresets.tsx`

Key features:
- Dropdown to select from available profiles
- Save button opens ProfileSaveDialog
- Apply button applies selected profile
- Show loading/error states
- Display "Custom (modified)" when settings changed
- Show star icon for default profile

Use existing `FormSlider`, `FormToggle` patterns from QuickSettings.

**Task 3.4**: Create `frontend/src/components/ProfileSaveDialog.tsx`

Modal dialog with:
- Text input for profile name (required, validate unique)
- Textarea for description (optional)
- Checkbox for "Set as default"
- Save/Cancel buttons
- Error handling

Use existing modal/dialog component pattern from the project.

**Task 3.5**: Build frontend

```bash
cd frontend
npm run build
```

Copy built assets to `data/webui/`.

---

### Phase 4: Testing (Priority 3)

**Task 4.1**: Backend API Testing (use curl or Postman)

```bash
# List profiles
curl http://192.168.1.176:7999/0/api/profiles?camera_id=0

# Create profile
curl -X POST http://192.168.1.176:7999/0/api/profiles \
  -H "Content-Type: application/json" \
  -H "X-CSRF-Token: <token>" \
  -d '{"name":"Daytime","description":"Bright settings","camera_id":0,"snapshot_current":true}'

# Apply profile
curl -X POST http://192.168.1.176:7999/0/api/profiles/1/apply \
  -H "X-CSRF-Token: <token>"

# Delete profile
curl -X DELETE http://192.168.1.176:7999/0/api/profiles/1 \
  -H "X-CSRF-Token: <token>"
```

**Task 4.2**: Frontend Testing

1. Open Dashboard in browser
2. Click gear icon on camera → Quick Settings opens
3. ConfigurationPresets shows at top
4. Test workflow:
   - Adjust some sliders (brightness, threshold, etc.)
   - Click Save → Dialog opens
   - Enter name "Test Profile" → Save
   - Select different settings
   - Select "Test Profile" from dropdown → Click Apply
   - Verify sliders update to saved values

**Task 4.3**: End-to-End Testing on Pi

**Ask user if Pi is powered on before attempting connection!**

1. Create "Daytime" profile (bright settings, low threshold)
2. Create "Nighttime" profile (low brightness, high ISO, high threshold)
3. Switch between profiles and verify:
   - Camera brightness changes
   - Motion detection sensitivity changes
   - Settings persist in Quick Settings sliders
4. Test framerate change → Verify restart warning

---

## Important Notes

### Code Style
- Follow existing patterns in `webu_json.cpp` for API handlers
- Use `MOTION_LOG()` for logging (NTC, INF, ERR levels)
- Validate CSRF for all write operations
- Use `pthread_mutex_lock(&app->mutex_post)` when modifying config

### Security
- Always validate CSRF tokens for POST/PATCH/DELETE
- Use parameterized SQL (already done in conf_profile.cpp)
- Validate profile IDs exist before operations
- Don't expose SQL errors to client

### Error Handling
- Return proper HTTP status codes
- Provide helpful error messages
- Log errors with context
- Handle missing SQLite3 gracefully (stub implementations exist)

### Testing Strategy
1. Test backend API endpoints individually with curl
2. Test frontend components in isolation
3. Test full workflow end-to-end
4. Test edge cases (empty profiles list, invalid IDs, concurrent access)

---

## Success Criteria

✅ All 7 API endpoints accessible and working
✅ Profiles can be created, loaded, applied, deleted
✅ Frontend dropdown shows profiles
✅ Apply button updates Quick Settings sliders
✅ Database persists across daemon restarts
✅ No memory leaks or crashes
✅ Profile operations complete in < 500ms
✅ Works on Pi 4/5 with actual camera

---

## If You Get Stuck

**Compilation errors**:
- Check `src/Makefile.am` includes new files
- Verify `--with-sqlite3` configure flag
- Check for missing `#include` statements

**API not routing**:
- Verify route wiring in `webu_ans.cpp`
- Check log output: `sudo journalctl -u motion -f`
- Test with curl to isolate frontend vs backend issues

**Frontend not working**:
- Check browser console for errors
- Verify API responses with Network tab
- Check CSRF token is being sent

**Database issues**:
- Check database file created: `ls -l <target_dir>/config_profiles.db`
- Use `sqlite3` CLI to inspect schema
- Check permissions on directory

---

## Commands for Quick Reference

**Build backend**:
```bash
cd /Users/tshuey/Documents/GitHub/motion-motioneye
autoreconf -fiv && ./configure --with-libcam --with-sqlite3 && make -j4
```

**Build frontend**:
```bash
cd frontend && npm run build
```

**Deploy to Pi** (ask if powered on first!):
```bash
rsync -avz --exclude='.git' /Users/tshuey/Documents/GitHub/motion-motioneye/ admin@192.168.1.176:~/motion-motioneye/
ssh admin@192.168.1.176 "cd ~/motion-motioneye && autoreconf -fiv && ./configure --with-libcam --with-sqlite3 && make -j4"
ssh admin@192.168.1.176 "sudo systemctl restart motion"
ssh admin@192.168.1.176 "sudo journalctl -u motion -f"
```

---

## Final Checklist

Before marking complete, verify:

- [ ] All 7 API endpoints implemented and tested
- [ ] Routes wired up for GET/POST/PATCH/DELETE
- [ ] Makefile.am updated
- [ ] Backend compiles without errors
- [ ] API endpoints return proper JSON
- [ ] CSRF validation working
- [ ] Frontend API client created
- [ ] React Query hooks created
- [ ] ConfigurationPresets component functional
- [ ] ProfileSaveDialog component created
- [ ] Frontend builds without errors
- [ ] Full workflow tested (create → apply → delete)
- [ ] Works on Pi with real camera
- [ ] Documentation updated

---

Good luck! The hard part (backend foundation) is done. Phases 2-4 are mostly pattern-following and wiring things together.
