# React UI Architecture for Motion

**Created**: 2025-12-27
**Status**: Design approved
**Goal**: Replace MotionEye with a lightweight React frontend served by Motion's built-in web server

---

## Design Overview

### Current State (MotionEye Python Stack)
```
User → MotionEye (Python/Tornado) → Motion Daemon (C++)
       ├─ HTML/jQuery UI (heavy)
       ├─ Python middleware (~100MB RAM)
       └─ Proxy for Motion streams
```

### Target State (React Direct Integration)
```
User → Motion Daemon (C++)
       ├─ React SPA (served as static files)
       ├─ JSON API (existing + enhanced)
       └─ Direct MJPEG streams
```

**Benefits**:
- **RAM**: ~30MB vs ~100MB (70% reduction)
- **Complexity**: No Python layer, single binary
- **Maintenance**: Modern React vs legacy jQuery
- **Performance**: Direct streaming, no proxy overhead

---

## Architecture Components

### 1. Motion C++ Web Server (libmicrohttpd)

**Existing capabilities**:
- Serves static files
- JSON API for configuration
- MJPEG/MPEG-TS camera streams
- Basic/Digest authentication
- CSRF protection

**Required enhancements**:
- **SPA routing**: Serve `index.html` for unmatched routes
- **Configurable build path**: Where to find React build files
- **Enhanced JSON API**: Additional endpoints for UI features

### 2. React Frontend

**Tech stack**:
- **Framework**: React 18 + TypeScript
- **Bundler**: Vite (fast dev/build)
- **Styling**: TailwindCSS (dark theme)
- **State**: Zustand (lightweight, 1KB)
- **Data fetching**: TanStack Query (caching, sync)
- **Forms**: React Hook Form + Zod validation

**Build output**:
- Static files → `data/webui/`
- Motion serves from configured `webcontrol_html_path`

---

## API Endpoints

### Existing (Motion built-in)

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/0/config/list` | GET | List all config parameters |
| `/0/config/get` | GET | Get specific parameter value |
| `/0/config/set` | POST | Update parameter |
| `/0/config/write` | POST | Persist to motion.conf |
| `/0/detection/start` | POST | Start motion detection |
| `/0/detection/pause` | POST | Pause detection |
| `/0/detection/status` | GET | Get detection state |
| `/{cam}/stream` | GET | MJPEG stream (video) |
| `/{cam}/current` | GET | Latest snapshot (JPEG) |
| `/{cam}/action/snapshot` | POST | Trigger snapshot |
| `/{cam}/action/eventstart` | POST | Force event start |

### New Endpoints (To Add)

| Endpoint | Method | Purpose | Location |
|----------|--------|---------|----------|
| `/api/auth/me` | GET | Current user info | webu_json.cpp |
| `/api/media/pictures/{cam}` | GET | List snapshots | webu_json.cpp |
| `/api/media/pictures/{id}` | DELETE | Delete snapshot | webu_json.cpp |
| `/api/media/movies/{cam}` | GET | List video recordings | webu_json.cpp |
| `/api/system/temperature` | GET | CPU temp (Pi only) | webu_json.cpp |
| `/api/system/stats` | GET | Uptime, memory, CPU | webu_json.cpp |

---

## Configuration Parameters

### New Parameters (Add to `parm_structs.hpp` / `conf.cpp`)

```cpp
// In ctx_parm_app (parm_structs.hpp ~line 76)
std::string     webcontrol_html_path;   // Path to React build
bool            webcontrol_spa_mode;    // Enable SPA fallback routing
```

**Defaults**:
- `webcontrol_html_path`: `/usr/share/motion/webui` (or `./data/webui` for dev)
- `webcontrol_spa_mode`: `true`

**Configuration file** (`motion.conf`):
```conf
webcontrol_html_path /usr/share/motion/webui
webcontrol_spa_mode on
```

---

## File Serving Strategy

### Current Implementation (`webu_file.cpp`)
- Serves files from database query (movie files only)
- Uses path validation for security

### Enhanced Implementation

```cpp
// Pseudocode for cls_webu_file::main()

1. Check if request is for database media (/0/movie/...)
   → Use existing database file serving

2. Check if request is for React static files
   a. Construct path: webcontrol_html_path + requested_path
   b. Validate path (prevent traversal attacks)
   c. If file exists:
      → Serve with appropriate MIME type
      → Set cache headers (1 year for /assets/*, none for index.html)
   d. If file NOT found AND webcontrol_spa_mode is on:
      → Serve index.html (SPA fallback for client-side routing)
   e. Otherwise:
      → 404 Not Found

3. Set security headers:
   - Content-Type based on file extension
   - Cache-Control based on path
   - X-Content-Type-Options: nosniff
```

### MIME Type Mapping

| Extension | Content-Type |
|-----------|-------------|
| `.html` | text/html; charset=utf-8 |
| `.js` | text/javascript; charset=utf-8 |
| `.css` | text/css; charset=utf-8 |
| `.json` | application/json; charset=utf-8 |
| `.png` | image/png |
| `.jpg`, `.jpeg` | image/jpeg |
| `.svg` | image/svg+xml |
| `.ico` | image/x-icon |
| `.woff`, `.woff2` | font/woff, font/woff2 |

---

## Security Considerations

### Path Traversal Prevention
- **Existing**: `validate_file_path()` in webu_file.cpp
- Use `realpath()` to resolve symlinks and `..` components
- Verify resolved path starts with allowed base directory

### Authentication Flow
1. User accesses React app (public, served as static files)
2. React makes API calls → Motion checks auth headers
3. If not authenticated → 401 → React redirects to login
4. Login form POSTs to existing auth endpoint
5. Motion sets session cookie (existing mechanism)
6. Subsequent API calls include cookie

### CSRF Protection
- React reads `X-CSRF-Token` from HTML meta tag (Motion injects)
- Include token in POST/PUT/DELETE headers
- Motion validates via `cls_webu::csrf_validate()`

---

## Component Architecture (React)

### Route Structure
```
/                       → Dashboard (camera grid)
/camera/:id             → Single camera view
/camera/:id/config      → Camera configuration
/settings               → Application settings
/settings/cameras       → Multi-camera config
/media                  → Media browser (snapshots/videos)
/system                 → System status & logs
```

### Key Components

**Core**:
- `App.tsx` - Router, auth provider
- `Layout.tsx` - Header, sidebar, main content
- `AuthProvider.tsx` - Authentication context

**Camera**:
- `CameraGrid.tsx` - Multi-camera dashboard
- `CameraStream.tsx` - MJPEG stream viewer
- `CameraControls.tsx` - Start/stop/snapshot buttons
- `CameraCard.tsx` - Single camera tile

**Configuration**:
- `ConfigForm.tsx` - Dynamic form from JSON schema
- `ConfigSection.tsx` - Category-based sections
- `ParamInput.tsx` - Smart input (string/int/bool/list)

**Media**:
- `MediaGallery.tsx` - Grid of snapshots/videos
- `MediaItem.tsx` - Single media file (thumbnail + actions)
- `VideoPlayer.tsx` - Playback for recorded videos

**System**:
- `SystemStatus.tsx` - Uptime, CPU, memory, temp
- `LogViewer.tsx` - Motion logs (if exposed via API)

---

## Data Flow

### Camera Stream Display
```
React Component
  ↓
<img src="/1/stream" />
  ↓
Motion: webu_stream.cpp
  ↓
Camera buffer (MJPEG frames)
  ↓
Direct to browser (no intermediate proxy)
```

### Configuration Update
```
React Form (react-hook-form)
  ↓
POST /0/config/set { "param": "value" }
  ↓
Motion: webu_json.cpp → cls_config::edit()
  ↓
Update in-memory config
  ↓
Return success JSON
  ↓
React: Invalidate query cache, show success toast
```

### Media Browser
```
React: useQuery('/api/media/pictures/1')
  ↓
Motion: webu_json.cpp → app->dbse->filelist_get()
  ↓
Query database for files
  ↓
Return JSON: [{ id, filename, timestamp, size, ... }]
  ↓
React: Render thumbnails
```

---

## Development Workflow

### Local Development
1. **Start Motion daemon** (serves backend API)
   ```bash
   cd /path/to/motion
   ./motion -c motion.conf -n
   ```

2. **Start Vite dev server** (React with HMR)
   ```bash
   cd frontend
   npm run dev
   # Proxies API calls to Motion port 7999
   ```

3. Access: `http://localhost:5173` (Vite dev server)

### Production Build
1. **Build React app**
   ```bash
   cd frontend
   npm run build
   # Output: ../data/webui/
   ```

2. **Configure Motion** (`motion.conf`)
   ```conf
   webcontrol_html_path /usr/share/motion/webui
   webcontrol_spa_mode on
   ```

3. **Access**: `http://pi-address:7999/` (Motion serves React)

---

## File Structure

```
motion-motioneye/
├── src/
│   ├── webu.cpp/hpp            # Web server core (no changes)
│   ├── webu_file.cpp/hpp       # MODIFY: Add SPA routing
│   ├── webu_json.cpp/hpp       # EXTEND: New API endpoints
│   ├── webu_ans.cpp/hpp        # MODIFY: Route new APIs
│   ├── conf.cpp/hpp            # ADD: webcontrol_html_path, etc.
│   └── parm_structs.hpp        # ADD: New params to ctx_parm_app
│
├── frontend/                   # NEW: React application
│   ├── src/
│   │   ├── components/
│   │   ├── hooks/
│   │   ├── pages/
│   │   ├── api/
│   │   └── App.tsx
│   ├── package.json
│   ├── vite.config.ts
│   └── tsconfig.json
│
├── data/
│   └── webui/                  # Build output (gitignored)
│       ├── index.html
│       ├── assets/
│       │   ├── index-[hash].js
│       │   └── index-[hash].css
│       └── ...
│
└── doc/
    └── plans/react-ui/
        ├── 01-architecture-20251227.md (this file)
        └── 02-detailed-execution-20251227.md (next)
```

---

## Deployment

### Raspberry Pi Installation

1. **Build on development machine**
   ```bash
   cd frontend && npm run build
   ```

2. **Sync to Pi**
   ```bash
   rsync -avz --exclude='node_modules' --exclude='.git' \
     /Users/tshuey/Documents/GitHub/motion-motioneye/ \
     admin@192.168.1.176:~/motion-motioneye/
   ```

3. **Install React build**
   ```bash
   ssh admin@192.168.1.176 "sudo cp -r ~/motion-motioneye/data/webui /usr/share/motion/"
   ```

4. **Update motion.conf**
   ```bash
   ssh admin@192.168.1.176
   sudo nano /etc/motion/motion.conf
   # Set: webcontrol_html_path /usr/share/motion/webui
   # Set: webcontrol_spa_mode on
   sudo systemctl restart motion
   ```

---

## Success Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| RAM usage | < 30MB | `ps aux | grep motion` |
| React bundle size | < 500KB gzipped | Vite build output |
| Initial load time | < 2s | Chrome DevTools |
| Stream latency | < 500ms | Visual comparison |
| API response time | < 100ms | Network tab |

---

## Migration Path

**Phase 1: C++ Backend** (Weeks 1-2)
- Add config parameters
- Implement SPA routing in webu_file.cpp
- Add new JSON API endpoints

**Phase 2: React Setup** (Week 2)
- Initialize React + Vite project
- Configure proxy, build output
- Implement authentication flow

**Phase 3: Core UI** (Weeks 3-4)
- Camera grid dashboard
- Stream display
- Basic controls (start/stop/snapshot)

**Phase 4: Configuration** (Week 5)
- Dynamic config form generation
- Parameter validation
- Save/reload functionality

**Phase 5: Media & System** (Week 6)
- Media browser (snapshots/videos)
- System status page
- Polish and testing

**Phase 6: Deployment** (Week 7)
- Pi testing
- Documentation
- Production deployment

---

## Risks & Mitigations

| Risk | Mitigation |
|------|-----------|
| Path traversal vulnerabilities | Reuse existing `validate_file_path()` |
| Breaking existing API clients | Only add new endpoints, don't modify existing |
| CORS issues in dev | Vite proxy configured for localhost |
| React bundle too large | Code splitting, tree shaking, lazy loading |
| Motion crashes serving static files | Extensive error handling, fallback to 404 |

---

## Open Questions

1. **Multi-camera support**: How to handle 10+ cameras in UI? (Pagination, virtualization)
2. **Real-time updates**: Should we add WebSocket for events? (Or stick with polling)
3. **Mobile responsive**: Touch-friendly controls for tablet/phone?
4. **Dark mode only**: Or support light mode toggle?
5. **Internationalization**: Support multiple languages like Motion does?

**Decision**: Start with simple polling, dark mode only, responsive design. Add complexity as needed.

---

## Next Steps

See `02-detailed-execution-20251227.md` for:
- File-by-file modification plan
- Code templates
- Testing strategy
- Component implementation details
