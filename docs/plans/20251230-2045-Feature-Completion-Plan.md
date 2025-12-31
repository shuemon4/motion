# Feature Completion Plan

**Date**: 2025-12-30
**Status**: Active
**Priority**: High

## Executive Summary

Analysis of the codebase against the original implementation plans reveals several incomplete features despite "completion" status in sprint summaries. This plan identifies all gaps and provides a detailed, parallelizable execution strategy.

---

## Current State Analysis

### What Was Completed (Sprints 1-5)

| Component | Status | Notes |
|-----------|--------|-------|
| JSON API Architecture | ✅ Done | Batch PATCH, JSON parsing |
| SPA Static File Serving | ✅ Done | React served from Motion |
| Basic Settings Page | ✅ Done | 10 settings components created |
| Media Gallery | ✅ Done | Pictures/movies browsing, lightbox |
| System Status Display | ✅ Done | Temperature, RAM, disk in header |
| Form Components | ✅ Done | FormInput, FormSelect, FormToggle, FormSection |
| Authentication Context | ✅ Done | AuthContext, LoginModal, ProtectedRoute |

### Backend API Endpoints Implemented

| Endpoint | Method | Status |
|----------|--------|--------|
| `/api/config` | GET | ✅ |
| `/api/config` | PATCH | ✅ |
| `/api/cameras` | GET | ✅ |
| `/api/auth/me` | GET | ✅ |
| `/api/media/pictures` | GET | ✅ |
| `/api/media/movies` | GET | ✅ |
| `/api/media/picture/{id}` | DELETE | ✅ |
| `/api/media/movie/{id}` | DELETE | ✅ |
| `/api/system/temperature` | GET | ✅ |
| `/api/system/status` | GET | ✅ |
| `/api/system/reboot` | POST | ✅ |
| `/api/system/shutdown` | POST | ✅ |
| `/api/mask` | GET/POST/DELETE | ✅ |

---

## Incomplete Features (Critical Issues)

### Issue 1: Camera Selector Non-Functional (HIGH PRIORITY)

**Location**: `frontend/src/pages/Settings.tsx:163-180`

**Problem**: Settings page has a dropdown to select Global vs Camera-specific settings, but:
- All settings components always receive `config.configuration.default`
- Camera selection changes state but doesn't affect displayed values
- Users cannot configure individual cameras

**Root Cause**: Infrastructure exists (state, useEffect, dropdown) but data binding never completed.

**Impact**: Major feature gap - no per-camera configuration possible.

**Fix Required**:
1. Update Settings.tsx to pass `selectedCamera` to all settings components
2. Modify `getValue()` function to read from camera-specific config when applicable
3. Implement fallback: camera-specific → global default
4. Update TypeScript interfaces for camera configuration structure
5. Update each settings component to accept and use `selectedCamera` prop

**Files to Modify**:
- `frontend/src/pages/Settings.tsx`
- `frontend/src/components/settings/DeviceSettings.tsx`
- `frontend/src/components/settings/LibcameraSettings.tsx`
- `frontend/src/components/settings/OverlaySettings.tsx`
- `frontend/src/components/settings/StreamSettings.tsx`
- `frontend/src/components/settings/MotionSettings.tsx`
- `frontend/src/components/settings/PictureSettings.tsx`
- `frontend/src/components/settings/MovieSettings.tsx`
- `frontend/src/components/settings/StorageSettings.tsx`
- `frontend/src/components/settings/ScheduleSettings.tsx`
- `frontend/src/api/types.ts`

---

### Issue 2: Mask Editor Not Implemented (MEDIUM PRIORITY)

**Plan Reference**: Wave 3 / Phase 4 of MotionEye-Feature-Parity.md

**Problem**: Backend mask API exists (`api_mask_get`, `api_mask_post`, `api_mask_delete`) but:
- No frontend UI for mask editing
- No canvas-based polygon drawing tool
- No PGM generation from browser

**Impact**: Users cannot visually define motion/privacy masks - must manually create PGM files.

**Fix Required**:
1. Create `MaskEditor.tsx` component
2. Implement canvas overlay on live stream
3. Add polygon/rectangle drawing tools
4. Create `utils/pgm.ts` for PGM file generation
5. Integrate with existing mask API endpoints

**Files to Create**:
- `frontend/src/components/settings/MaskEditor.tsx`
- `frontend/src/utils/pgm.ts`

---

### Issue 3: Notification System Not Implemented (MEDIUM PRIORITY)

**Plan Reference**: Wave 3 / Phase 5 of MotionEye-Feature-Parity.md

**Problem**: No UI for configuring notifications (email, Telegram, webhook, command).

**Impact**: Users cannot set up event notifications through the UI.

**Fix Required**:
1. Create `NotificationSettings.tsx` component
2. Create notification configuration UI (email, Telegram, webhook, command)
3. Create `motion-notify.sh` script
4. Add notification test functionality

**Files to Create**:
- `frontend/src/components/settings/NotificationSettings.tsx`
- `data/scripts/motion-notify.sh`

---

### Issue 4: Upload Settings Not Implemented (LOW PRIORITY)

**Plan Reference**: Wave 4 / Phase 6 of MotionEye-Feature-Parity.md

**Problem**: No UI for cloud upload configuration.

**Files to Create**:
- `frontend/src/components/settings/UploadSettings.tsx`
- `data/scripts/motion-upload.sh`

---

### Issue 5: Missing UI Polish (Sprint 6 Never Executed)

**Plan Reference**: Sprint 5 summary "Next Steps (Sprint 6)"

**Sprint 6 Was Planned But Not Executed**:
- Mobile responsiveness testing/fixes
- Performance optimization (lazy loading, code splitting)
- Error handling improvements
- Documentation

---

### Issue 6: Media Delete Functionality in UI

**Problem**: Backend delete endpoints exist but frontend doesn't expose them.

**Current State**:
- `useDeletePicture()` and `useDeleteMovie()` hooks exist
- No delete button in Media page UI

**Fix Required**:
1. Add delete button to media lightbox modal
2. Add selection mode for bulk delete
3. Add confirmation dialog

---

## Execution Plan

### Phase A: Camera Selector Fix (Highest Priority)

**Parallelizable**: No - must be done as single unit

**Tasks**:

1. **Update API Types** (`frontend/src/api/types.ts`)
   - Add proper typing for camera-specific configuration
   - Define structure: `configuration.cam1`, `configuration.cam2`, etc.

2. **Update Settings.tsx Core Logic**
   - Modify `getValue()` to accept `selectedCamera`
   - Add logic: if camera != "0", read from `config.cameras[selectedCamera]`
   - Implement fallback to `config.configuration.default`

3. **Update All Settings Components** (10 components)
   - Add `selectedCamera: string` prop to each
   - Pass config based on selected camera
   - No functional changes, just prop threading

4. **Testing**
   - Test with single camera (camera 1 should show values)
   - Test with Global Settings (camera 0)
   - Test save operation targets correct camera

**Estimated Effort**: 2-3 hours

---

### Phase B: Media Delete UI (Quick Win)

**Parallelizable**: Yes - can run with other phases

**Tasks**:

1. Update `frontend/src/pages/Media.tsx`
   - Add delete button to lightbox modal
   - Add confirmation dialog
   - Wire up `useDeletePicture()`/`useDeleteMovie()` hooks

**Estimated Effort**: 30 minutes

---

### Phase C: Mask Editor (Complex Feature)

**Parallelizable**: Yes - independent of other phases

**Tasks**:

1. **Create PGM Utilities** (`frontend/src/utils/pgm.ts`)
   - Function to generate PGM from polygon coordinates
   - Binary format generation

2. **Create MaskEditor Component**
   - Canvas overlay component
   - Rectangle drawing (MVP)
   - Polygon drawing (stretch goal)
   - Save/load/clear functionality

3. **Integrate into Settings**
   - Add MaskEditor to MotionSettings.tsx or separate tab
   - Wire up to `/api/mask` endpoints

**Estimated Effort**: 4-6 hours

---

### Phase D: Notification Settings (Medium Feature)

**Parallelizable**: Yes - independent of other phases

**Tasks**:

1. **Create NotificationSettings Component**
   - Email configuration section
   - Telegram configuration section
   - Webhook configuration section
   - Command execution section
   - Test notification button

2. **Create motion-notify.sh Script**
   - Email via msmtp/sendmail
   - Telegram API calls
   - Webhook POST/GET
   - Generic command execution

**Estimated Effort**: 3-4 hours

---

### Phase E: Upload Settings (Lower Priority)

**Parallelizable**: Yes

**Tasks**:

1. Create UploadSettings component
2. Create motion-upload.sh script (rclone-based)

**Estimated Effort**: 2-3 hours

---

### Phase F: Polish & Production Readiness (Sprint 6)

**Parallelizable**: Partially

**Tasks**:

1. **Mobile Responsiveness**
   - Test all pages on mobile viewports
   - Fix header on small screens
   - Stack system stats vertically on mobile

2. **Performance**
   - Add React.lazy() for route-level code splitting
   - Optimize media thumbnails

3. **Error Handling**
   - Network error recovery
   - Better error messages

**Estimated Effort**: 2-3 hours

---

## Recommended Execution Order

### For Sequential Execution (Single Agent)

```
1. Phase A: Camera Selector Fix (2-3 hours) - CRITICAL
2. Phase B: Media Delete UI (30 min) - Quick win
3. Phase F: Polish (2-3 hours) - Impacts all pages
4. Phase C: Mask Editor (4-6 hours) - Complex
5. Phase D: Notification Settings (3-4 hours)
6. Phase E: Upload Settings (2-3 hours)
```

**Total**: ~15-20 hours

### For Parallel Execution (Multiple Agents)

```
Agent 1: Phase A (Camera Selector) → Phase F (Polish)
Agent 2: Phase B (Media Delete) → Phase C (Mask Editor)
Agent 3: Phase D (Notifications) → Phase E (Upload)
```

**Wall Clock Time**: ~6-8 hours with 3 agents

---

## Sub-Agent Task Specifications

### Sub-Agent Task: Camera Selector Fix

```
CONTEXT:
The Settings page in frontend/src/pages/Settings.tsx has a dropdown for selecting
Global vs Camera-specific settings. The dropdown exists but is non-functional because
all settings components receive config.configuration.default regardless of selection.

TASK:
1. Read frontend/src/pages/Settings.tsx to understand current implementation
2. Read frontend/src/api/types.ts to understand config structure
3. Update Settings.tsx:
   - Modify getValue() to read from camera-specific config when selectedCamera != "0"
   - Pattern: config.cameras[selectedCamera] with fallback to config.configuration.default
4. Update all 10 settings components to accept and pass through selectedCamera prop
5. Test save operation uses correct camera ID (already implemented, just verify)
6. Build frontend (npm run build in frontend directory)
7. Report what files were modified and summary of changes

FILES TO MODIFY:
- frontend/src/pages/Settings.tsx
- frontend/src/components/settings/*.tsx (10 files)
- frontend/src/api/types.ts (if needed)

DO NOT:
- Create new files unless necessary
- Modify backend C++ code
- Change the dropdown UI itself (already correct)
```

### Sub-Agent Task: Media Delete UI

```
CONTEXT:
Backend delete endpoints exist. Frontend has useDeletePicture() and useDeleteMovie()
hooks in frontend/src/api/queries.ts. Media page (frontend/src/pages/Media.tsx) has
a lightbox modal but no delete button.

TASK:
1. Read frontend/src/pages/Media.tsx
2. Add a delete button to the lightbox modal
3. Add confirmation dialog before delete
4. Call appropriate delete hook based on media type (picture vs movie)
5. Handle loading state during delete
6. Invalidate query and close modal on success
7. Build and verify no TypeScript errors

DO NOT:
- Modify backend code
- Change media grid layout
```

### Sub-Agent Task: Mask Editor

```
CONTEXT:
Backend has /api/mask endpoints (GET/POST/DELETE) implemented in webu_json.cpp.
No frontend UI exists for mask editing.

TASK:
1. Create frontend/src/utils/pgm.ts with function to generate PGM from coordinates
2. Create frontend/src/components/settings/MaskEditor.tsx:
   - Canvas overlay component that sits on top of camera stream
   - Rectangle drawing tool (click and drag)
   - Generate PGM from drawn rectangles
   - Buttons: Save Mask, Clear Mask, Load Current Mask
   - Call /api/mask endpoints
3. Add MaskEditor to MotionSettings.tsx or create separate section
4. Build and test

PGM FORMAT:
- Header: "P5\n{width} {height}\n255\n"
- Binary data: width*height bytes, 0=masked, 255=active

REFERENCE:
- Backend implementation: src/webu_json.cpp lines 1461-1777
```

---

## Success Criteria

1. **Camera Selector**: Selecting "Camera 1" shows Camera 1's config, not global
2. **Media Delete**: Can delete pictures/movies from UI with confirmation
3. **Mask Editor**: Can draw mask in browser and save to backend
4. **Notifications**: Can configure email/webhook notifications
5. **Mobile**: All pages usable on phone-sized viewport
6. **Build**: Zero TypeScript errors, bundle < 150KB gzipped

---

## Files Summary

### Files to Modify

| File | Phase | Changes |
|------|-------|---------|
| `frontend/src/pages/Settings.tsx` | A | getValue() logic, prop threading |
| `frontend/src/components/settings/*.tsx` | A | Add selectedCamera prop (10 files) |
| `frontend/src/api/types.ts` | A | Camera config types |
| `frontend/src/pages/Media.tsx` | B | Delete button in modal |

### Files to Create

| File | Phase | Purpose |
|------|-------|---------|
| `frontend/src/components/settings/MaskEditor.tsx` | C | Canvas-based mask editing |
| `frontend/src/utils/pgm.ts` | C | PGM format generation |
| `frontend/src/components/settings/NotificationSettings.tsx` | D | Notification config UI |
| `frontend/src/components/settings/UploadSettings.tsx` | E | Cloud upload config |
| `data/scripts/motion-notify.sh` | D | Notification handler |
| `data/scripts/motion-upload.sh` | E | Upload handler |

---

## Notes

- Backend is fully functional - all API endpoints exist
- No C++ changes required for these features
- Priority is frontend completion
- Phase A is blocking for multi-camera users
