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
