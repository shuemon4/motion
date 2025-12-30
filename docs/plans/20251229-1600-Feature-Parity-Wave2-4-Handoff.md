# Feature Parity Waves 2-4 - Implementation Handoff

**Date**: 2025-12-29 16:00
**Prerequisites**: Wave 1 Complete ✅
**Plan Reference**: `docs/plans/20251229-0300-MotionEye-Feature-Parity.md`
**Execution Strategy**: `docs/scratchpads/20251229-1445-Feature-Parity-Execution.md`

## Context

Wave 1 is complete with foundation components and utilities in place. The Settings page now has:
- DeviceSettings, LibcameraSettings, OverlaySettings, StreamSettings components
- Parameter mapping and translation utilities
- Batch PATCH API integration
- Frontend builds successfully

## Objective

Implement Waves 2-4 to achieve feature parity with MotionEye's ~150 configuration options.

## Wave 2: Motion Detection & Output Settings

**Goal**: Implement motion detection controls and picture/movie output configuration

### Tasks

1. **Create MotionSettings Component** (`frontend/src/components/settings/MotionSettings.tsx`)
   - Threshold with percentage helper (use `percentToPixels`/`pixelsToPercent` from translations.ts)
   - Threshold maximum
   - Auto-tuning toggles (threshold_tune, noise_tune)
   - Noise level (1-255)
   - Light switch detection percentage
   - Despeckle filter selector
   - Smart mask speed
   - Locate motion mode (off/on/preview/both)
   - Event timing: gap, pre_capture, post_capture
   - Minimum motion frames

2. **Create PictureSettings Component** (`frontend/src/components/settings/PictureSettings.tsx`)
   - Capture mode selector with presets (use `captureModeToMotion`/`motionToCaptureMode`)
   - Picture quality (1-100)
   - Filename pattern with format code helper
   - Snapshot interval
   - Show conversion between capture modes and Motion parameters

3. **Create MovieSettings Component** (`frontend/src/components/settings/MovieSettings.tsx`)
   - Recording mode selector (use `recordingModeToMotion`/`motionToRecordingMode`)
   - Movie quality (1-100)
   - Filename pattern
   - Container format (mkv/mp4/3gp)
   - Max duration
   - Passthrough toggle

4. **Integration**
   - Add components to Settings.tsx
   - Test threshold percentage conversion
   - Verify capture/recording mode translations work correctly

### Success Criteria
- [ ] Threshold shown as percentage, saved as pixel count
- [ ] Motion detection enable/disable works
- [ ] Picture capture modes correctly map to Motion params
- [ ] Movie recording modes work
- [ ] All settings save via batch PATCH API

### Files to Create
- `frontend/src/components/settings/MotionSettings.tsx`
- `frontend/src/components/settings/PictureSettings.tsx`
- `frontend/src/components/settings/MovieSettings.tsx`

### Files to Modify
- `frontend/src/pages/Settings.tsx` (add new components)

---

## Wave 3: Advanced Features (Masks & Notifications)

**Goal**: Implement mask editor and notification system

### Part A: Mask Editor

**Backend Changes Required:**

1. **Add mask endpoints** to `src/webu_json.cpp`:
   ```cpp
   // GET /api/mask/{type} - Retrieve current mask (type: motion or privacy)
   // POST /api/mask/{type} - Save mask as PGM file
   // DELETE /api/mask/{type} - Remove mask
   ```

2. **Implement PGM generation** from polygon coordinates
   - Accept JSON array of polygon points
   - Generate PGM file at correct resolution
   - Save to mask_file or mask_privacy path

**Frontend Tasks:**

1. **Create PGM utility** (`frontend/src/utils/pgm.ts`)
   - Function to convert polygon coordinates to PGM format
   - Resolution-aware coordinate scaling

2. **Create MaskEditor Component** (`frontend/src/components/settings/MaskEditor.tsx`)
   - Canvas overlay on video stream preview
   - Polygon drawing tools (click to add points, close polygon)
   - Separate tabs for motion mask vs privacy mask
   - Save/load mask functionality
   - Upload existing PGM file option
   - Clear mask button

**Success Criteria:**
- [ ] Draw motion mask in browser
- [ ] Mask saves as PGM file
- [ ] Backend loads mask correctly
- [ ] Privacy mask separate from motion mask
- [ ] Can upload existing PGM files

### Part B: Notification System

**Backend Changes Required:**

1. **Add notification params** to Motion config (or use separate config file)
   - Email settings (SMTP server, from/to, subject template)
   - Telegram settings (API token, chat ID)
   - Webhook settings (URL, method, headers)
   - Store these in motion.conf or separate notify.conf

2. **Create notification script** (`data/scripts/motion-notify.sh`)
   ```bash
   #!/bin/bash
   # Email via sendmail/msmtp
   # Telegram via curl to API
   # Webhook POST/GET
   # Command execution wrapper
   ```

**Frontend Tasks:**

1. **Create NotificationSettings Component** (`frontend/src/components/settings/NotificationSettings.tsx`)
   - Email configuration panel
     - Enable toggle
     - SMTP server, port, auth
     - From/to addresses
     - Subject template
     - Test email button
   - Telegram configuration panel
     - Enable toggle
     - API token input
     - Chat ID input
     - Test notification button
   - Webhook configuration panel
     - Enable toggle
     - URL input
     - HTTP method selector
     - Custom headers (key-value pairs)
     - Test webhook button
   - Command execution panel
     - Enable toggle
     - Command input
     - Args/environment variables

2. **Wire up to Motion params:**
   - `on_event_start` = path to motion-notify.sh
   - `on_event_end` = path to motion-notify.sh (with --end flag)
   - Pass config to script via environment or config file

**Success Criteria:**
- [ ] Notification config saves to parameters
- [ ] Script executes on motion events
- [ ] Test buttons send real notifications
- [ ] Email, Telegram, Webhook all work

### Files to Create (Wave 3)
- `src/webu_json.cpp` modifications (mask endpoints)
- `frontend/src/utils/pgm.ts`
- `frontend/src/components/settings/MaskEditor.tsx`
- `frontend/src/components/settings/NotificationSettings.tsx`
- `data/scripts/motion-notify.sh`

---

## Wave 4: Storage & Polish

**Goal**: Storage management, upload configuration, schedules, and final polish

### Part A: Storage Management

**Backend Changes Required:**

1. **Add storage endpoint** to `src/webu_json.cpp`:
   ```cpp
   // GET /api/storage/usage - Return disk usage for target_dir
   // POST /api/storage/cleanup - Trigger manual cleanup
   ```

2. **Implement cleandir_params** if not present
   - Parse retention settings
   - Cleanup old files based on age/size

**Frontend Tasks:**

1. **Create StorageSettings Component** (`frontend/src/components/settings/StorageSettings.tsx`)
   - Target directory input
   - Disk usage display (used/free/total)
   - File retention settings
     - Max file age (days)
     - Max directory size (GB)
     - Keep minimum files count
   - Manual cleanup button
   - Network storage documentation link

**Success Criteria:**
- [ ] Storage usage visible
- [ ] Retention settings save
- [ ] Cleanup runs on schedule
- [ ] Manual cleanup works

### Part B: Cloud Upload

**Backend Changes Required:**

1. **Create upload script** (`data/scripts/motion-upload.sh`)
   ```bash
   #!/bin/bash
   # Use rclone for uploads
   # Support GDrive, Dropbox, S3, etc.
   # Read config from environment or file
   ```

**Frontend Tasks:**

1. **Create UploadSettings Component** (`frontend/src/components/settings/UploadSettings.tsx`)
   - Enable toggle
   - Service selector (GDrive, Dropbox, S3, FTP, etc.)
   - Credentials input (secure, masked)
   - Target path/bucket
   - Upload on event (picture/movie)
   - Test upload button

2. **Wire up to Motion params:**
   - `on_picture_save` = path to motion-upload.sh
   - `on_movie_end` = path to motion-upload.sh
   - Pass credentials securely (env vars, not in logs)

**Success Criteria:**
- [ ] Upload config saves
- [ ] Test upload works
- [ ] Files upload on events
- [ ] Credentials stored securely

### Part C: Schedules

**Frontend Tasks:**

1. **Create ScheduleSettings Component** (`frontend/src/components/settings/ScheduleSettings.tsx`)
   - Enable toggle
   - Schedule type (during/outside hours)
   - Per-day time range selectors
     - Monday from/to
     - Tuesday from/to
     - ... Sunday from/to
   - Convert to `schedule_params` format

2. **Implement schedule format conversion**
   - MotionEye: `from-to|from-to|...` pipe-separated
   - Motion: Verify exact format and implement conversion

**Success Criteria:**
- [ ] Schedule enables/disables correctly
- [ ] Time ranges save
- [ ] Motion respects schedule

### Part D: UI Preferences

**Frontend Tasks:**

1. **Create PreferencesSettings Component** (`frontend/src/components/settings/PreferencesSettings.tsx`)
   - Layout options (stored in localStorage)
     - Columns/rows for multi-camera grid
     - Fit frames vertically toggle
   - Playback options
     - Framerate factor
     - Resolution factor
   - Language selection (i18n prep)
   - Theme selection (if applicable)

2. **Implement localStorage persistence**
   - Save/load preferences
   - Apply on page load

**Success Criteria:**
- [ ] Preferences save to localStorage
- [ ] Preferences apply on load
- [ ] No backend storage needed

### Files to Create (Wave 4)
- `src/webu_json.cpp` modifications (storage endpoints)
- `frontend/src/components/settings/StorageSettings.tsx`
- `frontend/src/components/settings/UploadSettings.tsx`
- `frontend/src/components/settings/ScheduleSettings.tsx`
- `frontend/src/components/settings/PreferencesSettings.tsx`
- `data/scripts/motion-upload.sh`

---

## Integration & Testing

After all waves complete:

1. **Build and Deploy**
   ```bash
   cd frontend && npm run build
   rsync -avz --exclude='.git' --exclude='node_modules' \
     /Users/tshuey/Documents/GitHub/motion-motioneye/ admin@192.168.1.176:~/motion-motioneye/
   ssh admin@192.168.1.176 "cd ~/motion-motioneye && make -j4 && sudo make install"
   ```

2. **Restart Motion**
   ```bash
   ssh admin@192.168.1.176 "sudo systemctl restart motion"
   ```

3. **Test Checklist**
   - [ ] All settings visible in UI
   - [ ] Resolution presets work
   - [ ] Threshold percentage conversion accurate
   - [ ] libcamera controls functional (on Pi)
   - [ ] Text overlay presets work
   - [ ] Mask editor draws and saves
   - [ ] Notifications send successfully
   - [ ] Storage usage displays correctly
   - [ ] Upload to cloud works
   - [ ] Schedule activates/deactivates
   - [ ] Batch saves work (single HTTP request)
   - [ ] No console errors
   - [ ] Mobile responsive

---

## Execution Strategy

### Recommended Approach

**Use sub-agents for complex tasks:**

1. **Backend endpoint development**: Use general-purpose agent for C++ work
   - Mask endpoints implementation
   - Storage usage calculation
   - Any new webu_json.cpp additions

2. **Complex UI components**: Direct implementation
   - MaskEditor (canvas manipulation)
   - Use existing component patterns

3. **Script creation**: Direct implementation
   - Bash scripts are straightforward
   - Use templates from plan

4. **Integration testing**: Manual testing on Pi
   - Deploy built frontend
   - Verify end-to-end flows

### Parallel Execution

Where possible, parallelize work:
- Wave 2 components can be built in parallel
- Wave 3A (masks) and 3B (notifications) are independent
- Wave 4 components are independent

### Validation Points

After each wave:
1. Frontend builds without errors (`npm run build`)
2. TypeScript types are correct
3. Components integrate into Settings page
4. No duplicate code or settings
5. CPU efficiency considered (especially for Pi)

---

## Resources

**Existing Files to Reference:**
- `frontend/src/components/settings/DeviceSettings.tsx` - Component pattern
- `frontend/src/utils/translations.ts` - Conversion functions
- `frontend/src/utils/parameterMappings.ts` - Constants
- `src/webu_json.cpp` - Existing API endpoints
- `docs/plans/20251229-0300-MotionEye-Feature-Parity.md` - Full parameter mapping

**Translation Functions Available:**
- `percentToPixels()` / `pixelsToPercent()` - Threshold conversion
- `parseResolution()` / `formatResolution()` - Resolution strings
- `captureModeToMotion()` / `motionToCaptureMode()` - Picture modes
- `recordingModeToMotion()` / `motionToRecordingMode()` - Movie modes
- `presetToMotionText()` / `motionTextToPreset()` - Text overlays

**Pattern to Follow:**
```typescript
// Component structure
export interface XxxSettingsProps {
  config: Record<string, { value: string | number | boolean }>;
  onChange: (param: string, value: string | number | boolean) => void;
  getError?: (param: string) => string | undefined;
}

export function XxxSettings({ config, onChange, getError }: XxxSettingsProps) {
  const getValue = (param: string, defaultValue: string | number | boolean = '') => {
    return config[param]?.value ?? defaultValue;
  };

  return (
    <FormSection
      title="Section Title"
      description="Description"
      collapsible
      defaultOpen={false}
    >
      {/* Form controls here */}
    </FormSection>
  );
}
```

---

## Success Metrics

**Feature Parity**: 90%+ of MotionEye options available
**Performance**: Settings page loads <500ms
**Usability**: All settings saveable without page reload
**Reliability**: No data loss on concurrent edits
**Code Quality**: TypeScript builds without errors, follows existing patterns

---

## Final Deliverables

1. All components from Waves 2-4 implemented
2. Backend endpoints for masks and storage
3. Notification and upload scripts
4. Frontend builds successfully
5. All settings integrated into Settings page
6. Documentation updated
7. Testing completed on Pi hardware
8. Scratchpad updated with completion status

---

## Notes

- **CPU Efficiency**: Always consider Pi CPU impact (documented in code comments)
- **Security**: Never log credentials, use environment variables
- **Error Handling**: Validate inputs, display clear error messages
- **Mobile**: Ensure responsive design (Tailwind utilities)
- **Accessibility**: Follow existing form component patterns
- **Hot Reload**: Parameters that support hot reload should still work

---

## Quick Start Command

To begin execution:

```
Read this handoff document, review the existing Wave 1 implementation for patterns,
then start with Wave 2 by creating the MotionSettings component. Use sub-agents
for backend C++ work if needed. Build and test after each wave.
```
