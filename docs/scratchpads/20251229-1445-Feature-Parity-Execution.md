# Feature Parity Execution Strategy

**Date**: 2025-12-29 14:45
**Plan**: `docs/plans/20251229-0300-MotionEye-Feature-Parity.md`
**Prerequisites**: JSON API Architecture (COMPLETE)

## Analysis

### What We Have
1. ✅ JSON API with batch PATCH support (`/api/config`)
2. ✅ React UI with TanStack Query
3. ✅ Backend with 213 Motion parameters
4. ✅ Basic Settings page structure

### What We Need
Implement UI for ~150 MotionEye options mapped to Motion backend parameters.

## Execution Strategy: Agile Waves

### Why Agile Approach
- **Iterative value delivery**: Each wave delivers working features
- **Risk reduction**: Test integration early and often
- **Feedback incorporation**: Can adjust based on Pi hardware testing
- **Complexity management**: 8 phases is too serial; parallelize where possible

### Wave Architecture

**Wave 1: Core Settings Foundation** (Days 1-2)
- Establish component patterns
- Implement high-value, low-risk features
- Validate end-to-end flow
- **Deliverable**: Working device and streaming settings

**Wave 2: Detection & Output** (Days 3-4)
- Motion detection controls
- Picture and movie configuration
- **Deliverable**: Full recording control

**Wave 3: Advanced Features** (Days 5-6)
- Mask editor (complex UI)
- Notification system (script integration)
- **Deliverable**: Event handling complete

**Wave 4: Storage & Polish** (Days 7-8)
- Storage management
- Upload configuration
- Schedules and preferences
- **Deliverable**: Feature parity achieved

## Detailed Wave Plans

### Wave 1: Core Settings Foundation

**Backend Changes**: NONE (all params exist)

**Frontend Components**:
1. `components/settings/SettingsLayout.tsx` - Tabbed layout shell
2. `components/settings/DeviceSettings.tsx`
3. `components/settings/LibcameraSettings.tsx`
4. `components/settings/OverlaySettings.tsx`
5. `components/settings/StreamSettings.tsx`

**Translation Utilities**:
1. `utils/parameterMappings.ts` - Direct mappings object
2. `utils/translations.ts` - Resolution, text preset helpers

**Integration**:
- Update `pages/Settings.tsx` to use tabbed layout
- Use batch PATCH for all saves
- Add validation and error handling

**Success Criteria**:
- [ ] Device name, framerate, rotation editable
- [ ] Resolution selector with presets (640x480, 1280x720, 1920x1080)
- [ ] libcamera controls visible and functional (Pi only)
- [ ] Text overlay with presets working
- [ ] Stream quality/framerate adjustable
- [ ] Single save button updates all changed params

**Testing**:
```bash
# After deployment to Pi
curl -s http://192.168.1.176:7999/0/api/config | jq '.device_name, .framerate, .width, .height'
```

### Wave 2: Detection & Output

**Backend Changes**: NONE

**Frontend Components**:
1. `components/settings/MotionSettings.tsx`
   - Threshold with percentage helper
   - Auto-tuning toggles
   - Noise and light switch detection
   - Event timing controls
2. `components/settings/PictureSettings.tsx`
   - Capture mode selector
   - Quality, interval
   - Filename pattern
3. `components/settings/MovieSettings.tsx`
   - Recording mode
   - Format, quality, duration
   - Passthrough option

**Translation Utilities**:
1. `utils/threshold.ts` - Percentage ↔ pixel conversion
2. `utils/captureMode.ts` - Mode presets to params

**Success Criteria**:
- [ ] Motion detection enable/disable
- [ ] Threshold shown as percentage, saved as pixels
- [ ] Picture capture modes working
- [ ] Movie recording functional
- [ ] Hot reload for applicable params

### Wave 3: Advanced Features

**Backend Changes**:
- New endpoint: `POST /api/mask` for PGM upload
- New endpoint: `GET /api/mask/{type}` for mask retrieval

**Frontend Components**:
1. `components/settings/MaskEditor.tsx`
   - Canvas overlay on live stream
   - Polygon drawing tools
   - PGM generation from coordinates
   - Separate motion/privacy masks
2. `components/settings/NotificationSettings.tsx`
   - Email configuration
   - Telegram settings
   - Webhook URL/method
   - Command execution
3. `utils/pgm.ts` - PGM file format generation

**Script Creation**:
1. `data/scripts/motion-notify.sh`
   - Email via sendmail/msmtp
   - Telegram API calls
   - Webhook POST/GET
   - Command execution wrapper

**Success Criteria**:
- [ ] Draw motion mask in browser
- [ ] Mask saves as PGM and loads on backend
- [ ] Notification config saves to parameters
- [ ] Test notification button works

### Wave 4: Storage & Polish

**Backend Changes**:
- New endpoint: `GET /api/storage/usage`
- Implement cleandir_params if not present

**Frontend Components**:
1. `components/settings/StorageSettings.tsx`
   - Target directory
   - Usage display
   - Cleanup settings
2. `components/settings/UploadSettings.tsx`
   - Service selector
   - Credentials (secure input)
   - Test upload button
3. `components/settings/ScheduleSettings.tsx`
   - Day/time range selectors
   - Schedule enable/disable
4. `components/settings/PreferencesSettings.tsx`
   - Layout options (localStorage)
   - Language selection (i18n prep)

**Script Creation**:
1. `data/scripts/motion-upload.sh`
   - rclone integration
   - S3/GDrive/Dropbox support

**Success Criteria**:
- [ ] Storage usage visible
- [ ] Upload config saves
- [ ] Schedule format converts correctly
- [ ] All 8 phases complete

## Risk Mitigation

### Mask Editor Complexity
- **Start simple**: Rectangles only, add polygons later
- **Fallback**: Manual PGM upload if drawing fails

### Pi Hardware Dependency
- **Mock data**: Use mock config responses for development
- **Conditional rendering**: Hide libcamera settings on non-Pi

### Performance on Pi
- **Debounce**: All input changes debounced 500ms
- **Batch saves**: Single PATCH per save action
- **Lazy load**: Settings panels load on tab switch

### CSRF Token Handling
- **Already implemented**: JSON API provides token
- **Refresh logic**: Handle token expiry gracefully

## Implementation Order

### Day 1 (Today)
1. Create scratchpad (this file) ✅
2. Setup settings layout structure
3. Implement DeviceSettings component
4. Implement translation utilities

### Day 2
1. Complete LibcameraSettings
2. Complete OverlaySettings
3. Complete StreamSettings
4. Test on Pi hardware

### Days 3-4
1. Motion detection controls
2. Picture/movie settings
3. Integration testing

### Days 5-6
1. Mask editor (biggest complexity)
2. Notification system
3. Scripts creation

### Days 7-8
1. Storage management
2. Upload configuration
3. Schedules
4. Final testing and documentation

## Technical Decisions

### Component Pattern
```typescript
// Standard settings panel
interface SettingsPanelProps {
  config: CameraConfig;
  onChange: (changes: Record<string, any>) => void;
  errors?: Record<string, string>;
}
```

### State Management
- Use Zustand for draft changes (uncommitted edits)
- TanStack Query for server state
- Save button commits draft to server

### Validation
- Frontend validation for ranges/formats
- Backend validation returns specific errors
- Display errors inline per field

### Mobile Responsive
- Tabs → Accordion on mobile
- Full-width controls
- Touch-friendly inputs

## Files to Create (Summary)

**Backend**:
- `src/webu_json.cpp` additions for mask/storage endpoints

**Frontend Components** (12 new):
- `components/settings/SettingsLayout.tsx`
- `components/settings/DeviceSettings.tsx`
- `components/settings/LibcameraSettings.tsx`
- `components/settings/OverlaySettings.tsx`
- `components/settings/StreamSettings.tsx`
- `components/settings/MotionSettings.tsx`
- `components/settings/PictureSettings.tsx`
- `components/settings/MovieSettings.tsx`
- `components/settings/MaskEditor.tsx`
- `components/settings/NotificationSettings.tsx`
- `components/settings/StorageSettings.tsx`
- `components/settings/UploadSettings.tsx`

**Frontend Utils** (5 new):
- `utils/parameterMappings.ts`
- `utils/translations.ts`
- `utils/threshold.ts`
- `utils/captureMode.ts`
- `utils/pgm.ts`

**Scripts** (2 new):
- `data/scripts/motion-notify.sh`
- `data/scripts/motion-upload.sh`

**Modified**:
- `frontend/src/pages/Settings.tsx`

## Wave 1 Status: COMPLETE ✅

### Completed Tasks
1. ✅ Created parameter mappings (`utils/parameterMappings.ts`)
2. ✅ Created translation utilities (`utils/translations.ts`)
3. ✅ Built DeviceSettings component
4. ✅ Built LibcameraSettings component
5. ✅ Built OverlaySettings component
6. ✅ Built StreamSettings component
7. ✅ Integrated components into Settings page
8. ✅ Frontend builds successfully without errors

### What Was Delivered

**4 New Settings Components:**
- DeviceSettings: Camera name, resolution with presets, framerate, rotation
- LibcameraSettings: All 14 libcamera parameters with conditional rendering
- OverlaySettings: Text overlay presets with custom text support
- StreamSettings: Quality, framerate, scale, motion boxes, authentication

**2 Utility Files:**
- `parameterMappings.ts`: Direct mappings, presets, constants
- `translations.ts`: Format conversion functions (resolution, threshold, text presets, capture modes, etc.)

**Benefits:**
- Settings organized into collapsible sections
- Resolution picker with common presets + custom option
- Text overlay with presets (camera name, timestamp, custom)
- AWB and autofocus controls show/hide based on mode
- All settings use batch PATCH API (efficient)

## Next Steps (Wave 2+)

The foundation is in place. Next waves will add:
- **Wave 2**: Motion detection, picture, and movie settings
- **Wave 3**: Mask editor and notification system
- **Wave 4**: Storage management, upload, and schedules

## Files Created

**Frontend Components:**
- `frontend/src/components/settings/DeviceSettings.tsx`
- `frontend/src/components/settings/LibcameraSettings.tsx`
- `frontend/src/components/settings/OverlaySettings.tsx`
- `frontend/src/components/settings/StreamSettings.tsx`

**Frontend Utils:**
- `frontend/src/utils/parameterMappings.ts`
- `frontend/src/utils/translations.ts`

**Modified:**
- `frontend/src/pages/Settings.tsx` (integrated new components)

**Built Assets:**
- `data/webui/index.html`
- `data/webui/assets/index-nf6tqYm9.css` (17.74 kB)
- `data/webui/assets/index-Ce62sOFN.js` (363.44 kB)
