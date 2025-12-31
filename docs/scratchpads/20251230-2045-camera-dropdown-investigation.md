# Camera Dropdown Investigation - Settings Page

## Issue
User reports that a dropdown appears on the settings page for selecting cameras. The dropdown is visible but doesn't appear to do anything.

## Investigation Findings

### Location
`frontend/src/pages/Settings.tsx:163-180`

### Code Analysis

```tsx
<select
  value={selectedCamera}
  onChange={(e) => setSelectedCamera(e.target.value)}
  className="px-3 py-1.5 bg-surface-elevated border border-gray-700 rounded-lg text-sm"
>
  <option value="0">Global Settings</option>
  {config.cameras && Object.keys(config.cameras).map((key) => {
    if (key !== 'count') {
      return (
        <option key={key} value={key}>
          Camera {key}
        </option>
      )
    }
    return null
  })}
</select>
```

### Purpose
The dropdown is designed to switch between:
- **"Global Settings" (camera ID 0)**: Configuration that applies to all cameras
- **"Camera 1", "Camera 2", etc.**: Per-camera specific configuration

This is consistent with Motion's configuration system where:
- Camera 0 = global/default configuration
- Camera 1+ = individual camera configurations

### Current State Management
The component uses `selectedCamera` state (line 40):
```tsx
const [selectedCamera, setSelectedCamera] = useState('0')
```

And clears changes when camera selection changes (lines 53-56):
```tsx
useEffect(() => {
  setChanges({})
  setValidationErrors({})
}, [selectedCamera])
```

### The Problem
**The dropdown currently does nothing meaningful because:**

1. **All settings components receive global config only** (lines 209-262):
   ```tsx
   <DeviceSettings
     config={config.configuration.default}  // Always uses "default" (global)
     onChange={handleChange}
     getError={getError}
   />
   ```

2. **Save operation uses selectedCamera but reads from wrong source**:
   - Line 98: `const camId = parseInt(selectedCamera, 10)` ✓ Correct
   - Lines 102-105: Saves to correct camera ID ✓ Correct
   - But lines 209-262: Settings components always display global config ✗ Wrong

3. **API structure supports per-camera config**:
   The `MotionConfig` interface shows the structure supports it:
   ```tsx
   interface MotionConfig {
     version: string
     cameras: Record<string, unknown>  // Individual camera configs here
     configuration: {
       default: Record<string, {...}>  // Global config here
     }
     categories: Record<string, { name: string; display: string }>
   }
   ```

### Root Cause
The settings page was designed with camera selection in mind (the state and useEffect are present), but the implementation is incomplete. The settings components need to:

1. Accept the selected camera ID as a prop
2. Display the appropriate camera's configuration (from `config.cameras[selectedCamera]` when camera ≠ "0")
3. Fall back to global config when "Global Settings" is selected

### Expected Behavior
When user selects "Camera 1":
- All settings should show Camera 1's specific configuration values
- If a setting is not defined for Camera 1, show the global default value
- Save button should save changes to Camera 1's configuration

When user selects "Global Settings":
- All settings should show global default values
- Save button should save to global configuration (camera 0)

### Impact
Currently:
- Dropdown appears functional but is non-functional
- Users can only view/edit global settings regardless of selection
- No way to configure individual cameras through the UI
- This is a feature gap compared to the original MotionEye

## Recommendation
This is an **incomplete feature implementation** that needs to be completed. The camera selector was added but the data binding was never implemented.

### Implementation Needed
1. Update all settings components to accept `selectedCamera` prop
2. Modify `getValue()` function to read from correct config location based on `selectedCamera`
3. Handle fallback logic (camera-specific → global default)
4. Update TypeScript interfaces to properly type camera configurations
5. Test with actual multi-camera setup

### Complexity
**Medium** - The infrastructure is in place (state management, UI), but requires:
- Updates to ~10 settings components
- Logic to handle config hierarchy (camera-specific overrides global)
- Testing with multi-camera configuration
