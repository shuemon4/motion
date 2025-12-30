# Legacy Code Removal - Analysis and Execution

**Date**: 2025-12-29 14:00
**Objective**: Remove all legacy config API code per Handoff plan requirement
**Status**: Planning

## Target Files for Removal

### 1. Backend - `src/webu_post.cpp`

#### Location 1: `processor_init()` lines 884-898
**Code to Remove**:
```cpp
if (post_processor == NULL) {
    /* POST processor requires Content-Type: application/x-www-form-urlencoded
     * or multipart/form-data. For API endpoints using query strings (like
     * /config/set?param=value), we don't need a POST body, so continue anyway.
     * This allows React/JSON clients to use Content-Type: application/json */
    MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
        _("POST processor failed - uri_cmd1='%s' uri_cmd2='%s'"),
        webua->uri_cmd1.c_str(), webua->uri_cmd2.c_str());
    if ((webua->uri_cmd1 == "config") &&
        (webua->uri_cmd2.length() >= 3) &&
        (webua->uri_cmd2.substr(0, 3) == "set")) {
        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
            _("POST processor not needed for config/set (query string API)"));
        return MHD_YES;
    }
    MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
        _("POST processor init failed for non-config/set request"));
    return MHD_NO;
}
```

**Impact**: This workaround allowed `/config/set` to work with JSON Content-Type. Removing this means POST processor creation failure will return MHD_NO immediately (proper behavior).

**Dependencies**: None - this is a workaround for legacy endpoint

#### Location 2: `processor_start()` lines 910-918
**Code to Remove**:
```cpp
/* Only process POST data if we have a valid processor.
 * For config/set endpoints using query strings with JSON Content-Type,
 * the processor is NULL but we still need to consume the data. */
if (post_processor != nullptr) {
    retcd = MHD_post_process (post_processor, upload_data, *upload_data_size);
} else {
    retcd = MHD_YES;  /* Continue without processing - data will be ignored */
}
```

**Impact**: Removes workaround for NULL processor. After removal, code will only call MHD_post_process if processor exists (standard behavior).

**Dependencies**: None - workaround for legacy endpoint

#### Location 3: `processor_start()` lines 950-967
**Code to Remove**:
```cpp
/* Check if this is a hot reload request (POST /config/set?param=value) */
if ((webua->uri_cmd1 == "config") &&
    (webua->uri_cmd2.length() >= 3) &&
    (webua->uri_cmd2.substr(0, 3) == "set")) {
    /* Route to JSON hot reload handler instead of full config update */
    cls_webu_json *webu_json = new cls_webu_json(webua);
    webu_json->config_set();
    delete webu_json;

    /* Ensure resp_page has content before sending */
    if (webua->resp_page.empty()) {
        MOTION_LOG(ERR, TYPE_STREAM, NO_ERRNO,
            _("config_set returned empty response for %s"), webua->uri_cmd2.c_str());
        webua->resp_page = "{\"status\":\"error\",\"error\":\"Internal error: empty response\"}";
        webua->resp_type = WEBUI_RESP_JSON;
    }
    webua->mhd_send();
    retcd = MHD_YES;
} else {
```

**Impact**: Removes routing to legacy `config_set()` method. All POST requests will go through standard HTML form processing.

**Dependencies**: Calls `webu_json->config_set()` which we'll remove

### 2. Backend - `src/webu_json.hpp`

#### Declaration to Remove: Line 28
```cpp
void config_set();  /* Hot reload API: Set single parameter at runtime */
```

**Impact**: Removes declaration of legacy method

**Dependencies**: None - just a declaration

### 3. Backend - `src/webu_json.cpp`

**Need to Research**: Find and analyze the `config_set()` implementation

**Questions**:
- Where is it defined?
- What helper functions does it use?
- Are any helpers used only by config_set()?
- What about validate_hot_reload(), apply_hot_reload(), build_response()?

### 4. Frontend - `frontend/src/api/queries.ts`

#### Mutation to Remove: Lines 98-122
```typescript
// Update config parameter (legacy - single parameter)
// Motion's API uses POST /{camId}/config/set?param=value
export function useUpdateConfig() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: async ({
      camId,
      param,
      value,
    }: {
      camId: number;
      param: string;
      value: string;
    }) => {
      // Motion expects query parameters, not JSON body
      return apiPost(`/${camId}/config/set?${param}=${encodeURIComponent(value)}`, {});
    },
    onSuccess: (_, { camId }) => {
      // Invalidate config cache to refetch fresh data
      queryClient.invalidateQueries({ queryKey: queryKeys.config(camId) });
      queryClient.invalidateQueries({ queryKey: queryKeys.cameras });
    },
  });
}
```

**Impact**: Removes legacy mutation from frontend

**Dependencies**: Need to check if any components still use `useUpdateConfig()`

## Research Completed ✅

### 1. config_set() implementation - Lines 711-800
**Location**: `src/webu_json.cpp:711-800`

**What it does**:
- Parses query string from uri_cmd2: `set?param=value`
- URL decodes the value
- Validates parameter (blocks SQL params for security)
- Calls `validate_hot_reload()` to check if param exists and is hot-reloadable
- Gets old value from config
- Calls `apply_hot_reload()` to apply change
- Calls `build_response()` to format JSON response
- Adds libcam ignored controls list if applicable

### 2. Helper functions analysis

#### validate_hot_reload() - Lines 580-596
**Used by**:
- ❌ `config_set()` (legacy - line 754)
- ✅ `api_config_patch()` (new batch API - line 1322)

**Verdict**: ⚠️ **KEEP** - Used by new batch API

#### apply_hot_reload() - Lines 601-701
**Used by**:
- ❌ `config_set()` (legacy - line 774)
- ✅ `api_config_patch()` (new batch API - line 1338)

**Verdict**: ⚠️ **KEEP** - Used by new batch API

#### build_response() - Lines 563-574
**Used by**:
- ❌ `config_set()` (legacy - lines 724, 748, 755, 777)
- ❌ NOT used by `api_config_patch()` (builds response inline)

**Verdict**: ❌ **REMOVE** - Only used by legacy config_set()

### 3. Frontend usage of useUpdateConfig()
**Result**: ✅ Only defined in `frontend/src/api/queries.ts`, not used anywhere else

**Verdict**: Safe to remove

## Execution Plan

1. ✅ Create scratchpad for tracking
2. ✅ Research config_set() implementation details
3. ✅ Research frontend usage of useUpdateConfig()
4. ✅ Identify all helper functions to remove
5. ✅ Create removal plan with exact line numbers
6. ✅ Execute removals in safe order
7. ✅ Verify compilation
8. ⏳ Update analysis document

## Execution Summary

### Files Modified

#### Backend (C++)
1. ✅ `src/webu_json.cpp` - Removed config_set() method (lines 703-800) and build_response() helper (lines 560-573)
2. ✅ `src/webu_json.hpp` - Removed config_set() declaration (line 28)
3. ✅ `src/webu_post.cpp` - Removed legacy routing (lines 950-967) and workarounds (lines 884-901, 910-918)
4. ✅ `src/webu_ans.cpp` - Removed legacy GET routing to config_set (lines 1053-1066)

#### Frontend (TypeScript)
1. ✅ `frontend/src/api/queries.ts` - Removed useUpdateConfig() mutation (lines 98-122) and unused apiPost import

### Functions Preserved (Used by Batch API)
- ✅ `validate_hot_reload()` - Used by api_config_patch()
- ✅ `apply_hot_reload()` - Used by api_config_patch()

### Compilation Verification
- ✅ Backend: `make clean && make -j4` successful
- ✅ Frontend: `npm run build` successful (built in 805ms)

### Total Lines Removed
- Backend: ~155 lines
- Frontend: ~25 lines
- **Total: ~180 lines of legacy code removed**

## Detailed Removal Plan

### Order of Operations (Safe Deletion Order)

#### 1. Remove Frontend Legacy Code (No Dependencies)
**File**: `frontend/src/api/queries.ts`
**Lines**: 98-122 (25 lines)
**Remove**: Entire `useUpdateConfig()` function
**Reason**: Not used anywhere, safe to delete first

#### 2. Remove Backend Routing to config_set()
**File**: `src/webu_post.cpp`
**Lines**: 950-967 (18 lines)
**Remove**: Entire `if` block that routes to config_set()
**Note**: This will orphan config_set() but won't break compilation yet

#### 3. Remove config_set() Method
**File**: `src/webu_json.cpp`
**Lines**: 703-800 (98 lines including comments)
**Remove**: Entire `config_set()` method
**Note**: This will orphan build_response() but won't break compilation yet

#### 4. Remove build_response() Helper
**File**: `src/webu_json.cpp`
**Lines**: 560-574 (15 lines including comments)
**Remove**: Entire `build_response()` method
**Reason**: Only used by config_set()

#### 5. Remove config_set() Declaration
**File**: `src/webu_json.hpp`
**Line**: 28
**Remove**: `void config_set();  /* Hot reload API: Set single parameter at runtime */`

#### 6. Remove Workarounds in processor_init()
**File**: `src/webu_post.cpp`
**Lines**: 884-901 (18 lines)
**Remove**: Entire `if (post_processor == NULL)` block
**Replace with**: Simple `return MHD_NO;` on processor creation failure

#### 7. Simplify processor_start()
**File**: `src/webu_post.cpp`
**Lines**: 910-918
**Remove**: NULL processor workaround
**Replace with**: Direct call to MHD_post_process (no NULL check)

### Keep These (Used by Batch API)
- ✅ `validate_hot_reload()` - Used by api_config_patch()
- ✅ `apply_hot_reload()` - Used by api_config_patch()
- ✅ All helper methods in webu_json.hpp lines 61-65

## Notes

- Must maintain CSRF validation logic (not part of legacy)
- Must keep api_config_patch() and all its dependencies
- Must not break other POST endpoints (actions, etc.)
