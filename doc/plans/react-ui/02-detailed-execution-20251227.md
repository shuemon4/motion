# React UI Implementation - Detailed Execution Plan

**Created**: 2025-12-27
**Status**: Ready for execution
**Prerequisites**: Read `01-architecture-20251227.md` first

---

## Implementation Phases

### Phase 1: C++ Backend Extensions
1. Add configuration parameters
2. Implement SPA routing
3. Add new JSON API endpoints
4. Test backend changes

### Phase 2: React Application Setup
1. Initialize React + Vite project
2. Configure build and proxy
3. Setup routing and authentication

### Phase 3: Component Implementation
1. Camera streaming components
2. Configuration forms
3. Media browser
4. System monitoring

---

## Phase 1: C++ Backend Extensions

### Step 1.1: Add Configuration Parameters

**Files to modify**:
- `src/parm_structs.hpp`
- `src/conf.cpp`
- `src/conf.hpp`

#### 1.1.1: Add to `parm_structs.hpp`

**Location**: ~line 76, in `struct ctx_parm_app`

```cpp
// Add after webcontrol_trusted_proxies (~line 76)
std::string     webcontrol_html_path;    /* Path to React build files */
bool            webcontrol_spa_mode;     /* Enable SPA fallback routing */
```

#### 1.1.2: Add to `conf.cpp` - Parameter definitions

**Location**: ~line 210, in `config_parms[]` array (after webcontrol_trusted_proxies)

```cpp
{"webcontrol_html_path",  PARM_TYP_STRING, PARM_CAT_13, PARM_LEVEL_ADVANCED, false},
{"webcontrol_spa_mode",   PARM_TYP_BOOL,   PARM_CAT_13, PARM_LEVEL_ADVANCED, false},
```

#### 1.1.3: Add to `conf.cpp` - Edit handlers

**Location**: ~line 641-770, in `cls_config::edit()` method

**For boolean** (near other webcontrol_* bools ~line 643):
```cpp
if (name == "webcontrol_spa_mode") return edit_generic_bool(webcontrol_spa_mode, parm, pact, true);
```

**For string** (near other webcontrol_* strings ~line 770):
```cpp
if (name == "webcontrol_html_path") return edit_generic_string(webcontrol_html_path, parm, pact, "./data/webui");
```

#### 1.1.4: Add to `conf.hpp` - Reference aliases

**Location**: After other webcontrol_* aliases (search for "webcontrol_" in conf.hpp)

Find where other `webcontrol_` references are defined and add:
```cpp
std::string&    webcontrol_html_path        = parm_app.webcontrol_html_path;
bool&           webcontrol_spa_mode         = parm_app.webcontrol_spa_mode;
```

---

### Step 1.2: Implement SPA Routing in `webu_file.cpp`

**Goal**: Serve React static files and implement SPA fallback

#### 1.2.1: Add helper functions at top of file

```cpp
/**
 * Get MIME type based on file extension
 */
static std::string get_mime_type(const std::string &filename)
{
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream";
    }

    std::string ext = filename.substr(dot_pos + 1);
    mylower(ext);

    if (ext == "html") return "text/html; charset=utf-8";
    if (ext == "htm")  return "text/html; charset=utf-8";
    if (ext == "js")   return "text/javascript; charset=utf-8";
    if (ext == "css")  return "text/css; charset=utf-8";
    if (ext == "json") return "application/json; charset=utf-8";
    if (ext == "png")  return "image/png";
    if (ext == "jpg")  return "image/jpeg";
    if (ext == "jpeg") return "image/jpeg";
    if (ext == "gif")  return "image/gif";
    if (ext == "svg")  return "image/svg+xml";
    if (ext == "ico")  return "image/x-icon";
    if (ext == "woff") return "font/woff";
    if (ext == "woff2") return "font/woff2";
    if (ext == "ttf")  return "font/ttf";
    if (ext == "eot")  return "application/vnd.ms-fontobject";

    return "application/octet-stream";
}

/**
 * Get cache control header based on path
 * - /assets/* files are hashed, cache aggressively
 * - index.html must not be cached (for SPA updates)
 */
static std::string get_cache_control(const std::string &path)
{
    if (path.find("/assets/") != std::string::npos) {
        return "public, max-age=31536000, immutable";  /* 1 year */
    }
    if (path.find("index.html") != std::string::npos) {
        return "no-cache, no-store, must-revalidate";
    }
    return "public, max-age=3600";  /* 1 hour for other files */
}
```

#### 1.2.2: Create new method `serve_static_file()`

Add to `cls_webu_file` class (in webu_file.hpp):
```cpp
void serve_static_file();
```

Implement in webu_file.cpp:
```cpp
/**
 * Serve static files from React build directory
 * Implements SPA routing: if file not found, serve index.html
 */
void cls_webu_file::serve_static_file()
{
    struct stat statbuf;
    std::string file_path;
    std::string index_path;
    FILE *file_handle = nullptr;
    struct MHD_Response *response;
    mhdrslt retcd;

    /* Construct file path from webcontrol_html_path + request URI */
    file_path = app->cfg->webcontrol_html_path;
    if (file_path.back() != '/') {
        file_path += '/';
    }

    /* Remove leading slash from URI if present */
    std::string uri = webua->uri_cmd1;
    if (uri.empty() || uri == "/") {
        uri = "index.html";
    } else if (uri[0] == '/') {
        uri = uri.substr(1);
    }

    file_path += uri;

    /* Security: Validate path to prevent traversal attacks */
    if (!validate_file_path(file_path, app->cfg->webcontrol_html_path)) {
        MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO,
            _("Path traversal attempt blocked: %s from %s"),
            file_path.c_str(), webua->clientip.c_str());
        webua->bad_request();
        return;
    }

    /* Try to serve the requested file */
    if (stat(file_path.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
        file_handle = myfopen(file_path.c_str(), "rbe");
    }

    /* If file not found and SPA mode enabled, serve index.html */
    if (file_handle == nullptr && app->cfg->webcontrol_spa_mode) {
        index_path = app->cfg->webcontrol_html_path;
        if (index_path.back() != '/') {
            index_path += '/';
        }
        index_path += "index.html";

        if (stat(index_path.c_str(), &statbuf) == 0) {
            file_handle = myfopen(index_path.c_str(), "rbe");
            file_path = index_path;  /* For MIME type detection */
        }
    }

    /* If still no file, return 404 */
    if (file_handle == nullptr) {
        MOTION_LOG(NTC, TYPE_STREAM, NO_ERRNO,
            _("Static file not found: %s from %s"),
            uri.c_str(), webua->clientip.c_str());
        webua->bad_request();
        return;
    }

    /* Create response from file */
    webua->req_file = file_handle;
    response = MHD_create_response_from_callback(
        (size_t)statbuf.st_size, 32 * 1024,
        &webu_file_reader,
        webua, NULL);

    if (response == NULL) {
        myfclose(webua->req_file);
        webua->req_file = nullptr;
        webua->bad_request();
        return;
    }

    /* Set Content-Type header */
    std::string mime_type = get_mime_type(file_path);
    MHD_add_response_header(response, "Content-Type", mime_type.c_str());

    /* Set Cache-Control header */
    std::string cache_control = get_cache_control(file_path);
    MHD_add_response_header(response, "Cache-Control", cache_control.c_str());

    /* Security headers */
    MHD_add_response_header(response, "X-Content-Type-Options", "nosniff");

    retcd = MHD_queue_response(webua->connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    if (retcd == MHD_NO) {
        MOTION_LOG(WRN, TYPE_STREAM, NO_ERRNO,
            _("Error queueing static file response"));
    }
}
```

#### 1.2.3: Modify routing in `webu_ans.cpp`

Find where routes are handled and add routing for static files.

Look for the routing logic in `webu_ans.cpp` that determines which handler to call based on the URI. Add a check to route to static file serving when appropriate.

---

### Step 1.3: Extend JSON API

**File**: `src/webu_json.cpp`

#### 1.3.1: Add to `cls_webu_json` class (webu_json.hpp)

```cpp
void api_auth_me();
void api_media_pictures();
void api_media_pictures_delete();
void api_system_temperature();
void api_system_stats();
```

#### 1.3.2: Implement auth/me endpoint

```cpp
/**
 * GET /api/auth/me
 * Return current authenticated user information
 */
void cls_webu_json::api_auth_me()
{
    webua->resp_page = "{";

    if (webua->authenticated) {
        webua->resp_page += "\"authenticated\":true,";
        webua->resp_page += "\"username\":\"" + escstr(webua->username) + "\"";
    } else {
        webua->resp_page += "\"authenticated\":false";
    }

    webua->resp_page += "}";
    webua->resp_type = WEBUI_RESP_JSON;
}
```

#### 1.3.3: Implement media/pictures endpoint

```cpp
/**
 * GET /api/media/pictures/{cam_id}
 * Return list of snapshot files for a camera
 */
void cls_webu_json::api_media_pictures()
{
    vec_files flst;
    int cam_id;
    std::string sql;

    if (webua->cam == nullptr) {
        webua->bad_request();
        return;
    }

    cam_id = webua->cam->cfg->device_id;

    sql  = " select * from motion ";
    sql += " where device_id = " + std::to_string(cam_id);
    sql += " and file_type = 1";  /* 1 = snapshot */
    sql += " order by file_dtl desc, file_tml desc";
    sql += " limit 100;";

    app->dbse->filelist_get(sql, flst);

    webua->resp_page = "{\"pictures\":[";

    for (size_t i = 0; i < flst.size(); i++) {
        if (i > 0) {
            webua->resp_page += ",";
        }
        webua->resp_page += "{";
        webua->resp_page += "\"id\":" + std::to_string(flst[i].file_id) + ",";
        webua->resp_page += "\"filename\":\"" + escstr(flst[i].file_nm) + "\",";
        webua->resp_page += "\"path\":\"" + escstr(flst[i].full_nm) + "\",";
        webua->resp_page += "\"date\":\"" + flst[i].file_dtl + "\",";
        webua->resp_page += "\"time\":\"" + flst[i].file_tml + "\",";
        webua->resp_page += "\"size\":" + std::to_string(flst[i].file_sz);
        webua->resp_page += "}";
    }

    webua->resp_page += "]}";
    webua->resp_type = WEBUI_RESP_JSON;
}
```

#### 1.3.4: Implement system/temperature endpoint

```cpp
/**
 * GET /api/system/temperature
 * Return CPU temperature (Raspberry Pi only)
 */
void cls_webu_json::api_system_temperature()
{
    FILE *temp_file;
    int temp_raw;
    double temp_celsius;

    webua->resp_page = "{";

    /* Try to read Raspberry Pi thermal zone */
    temp_file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (temp_file != nullptr) {
        if (fscanf(temp_file, "%d", &temp_raw) == 1) {
            temp_celsius = temp_raw / 1000.0;
            webua->resp_page += "\"celsius\":" + std::to_string(temp_celsius) + ",";
            webua->resp_page += "\"fahrenheit\":" + std::to_string(temp_celsius * 9.0 / 5.0 + 32.0);
        }
        fclose(temp_file);
    } else {
        webua->resp_page += "\"error\":\"Temperature not available\"";
    }

    webua->resp_page += "}";
    webua->resp_type = WEBUI_RESP_JSON;
}
```

#### 1.3.5: Add routing in `webu_ans.cpp`

Find the routing logic and add:

```cpp
/* API routes */
if (webua->uri_cmd1 == "api") {
    if (webua->uri_cmd2 == "auth" && webua->uri_cmd3 == "me") {
        cls_webu_json json(webua);
        json.api_auth_me();
        return MHD_YES;
    }
    if (webua->uri_cmd2 == "media" && webua->uri_cmd3 == "pictures") {
        cls_webu_json json(webua);
        json.api_media_pictures();
        return MHD_YES;
    }
    if (webua->uri_cmd2 == "system" && webua->uri_cmd3 == "temperature") {
        cls_webu_json json(webua);
        json.api_system_temperature();
        return MHD_YES;
    }
}
```

---

### Step 1.4: Test Backend Changes

**Compile and test**:
```bash
cd /Users/tshuey/Documents/GitHub/motion-motioneye
autoreconf -fiv
./configure --with-libcam --with-sqlite3
make -j4
```

**Test configuration parameters**:
```bash
./motion -h 2>&1 | grep webcontrol_html_path
./motion -h 2>&1 | grep webcontrol_spa_mode
```

**Test in motion.conf**:
```conf
webcontrol_html_path ./data/webui
webcontrol_spa_mode on
```

**Test API endpoints** (after starting Motion):
```bash
curl http://localhost:7999/api/auth/me
curl http://localhost:7999/api/system/temperature
```

---

## Phase 2: React Application Setup

### Step 2.1: Initialize React + Vite

```bash
cd /Users/tshuey/Documents/GitHub/motion-motioneye
mkdir -p frontend
cd frontend
npm create vite@latest . -- --template react-ts
```

**Answer prompts**:
- Current directory? Yes
- Overwrite? Yes (if needed)

### Step 2.2: Install Dependencies

```bash
npm install
npm install -D tailwindcss postcss autoprefixer
npm install zustand @tanstack/react-query
npm install react-hook-form zod @hookform/resolvers
npm install react-router-dom
```

### Step 2.3: Configure Tailwind CSS

**Initialize**:
```bash
npx tailwindcss init -p
```

**tailwind.config.js**:
```javascript
/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        surface: {
          DEFAULT: '#1a1a1a',
          elevated: '#262626',
          hover: '#2e2e2e',
        },
        primary: {
          DEFAULT: '#3b82f6',
          hover: '#2563eb',
        },
        danger: {
          DEFAULT: '#ef4444',
          hover: '#dc2626',
        },
      },
    },
  },
  plugins: [],
}
```

**src/index.css**:
```css
@tailwind base;
@tailwind components;
@tailwind utilities;

@layer base {
  html {
    @apply bg-surface text-white;
  }

  body {
    @apply font-sans antialiased;
  }
}
```

### Step 2.4: Configure Vite

**vite.config.ts**:
```typescript
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import path from 'path'

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [react()],
  build: {
    outDir: '../data/webui',
    emptyOutDir: true,
  },
  server: {
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://localhost:7999',
        changeOrigin: true,
      },
      '/0': {
        target: 'http://localhost:7999',
        changeOrigin: true,
      },
      '/1': {
        target: 'http://localhost:7999',
        changeOrigin: true,
      },
      '/2': {
        target: 'http://localhost:7999',
        changeOrigin: true,
      },
      '/3': {
        target: 'http://localhost:7999',
        changeOrigin: true,
      },
      '/4': {
        target: 'http://localhost:7999',
        changeOrigin: true,
      },
    },
  },
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src'),
    },
  },
})
```

### Step 2.5: Setup React Router

**src/main.tsx**:
```typescript
import React from 'react'
import ReactDOM from 'react-dom/client'
import { BrowserRouter } from 'react-router-dom'
import { QueryClient, QueryClientProvider } from '@tanstack/react-query'
import App from './App.tsx'
import './index.css'

const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      staleTime: 30000,  // 30 seconds
      refetchOnWindowFocus: false,
    },
  },
})

ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <QueryClientProvider client={queryClient}>
      <BrowserRouter>
        <App />
      </BrowserRouter>
    </QueryClientProvider>
  </React.StrictMode>,
)
```

---

## Phase 3: Core Components

### Step 3.1: API Client

**src/api/client.ts**:
```typescript
export async function apiGet<T>(endpoint: string): Promise<T> {
  const response = await fetch(endpoint)

  if (!response.ok) {
    throw new Error(`API error: ${response.statusText}`)
  }

  return response.json()
}

export async function apiPost<T>(
  endpoint: string,
  data: Record<string, unknown>
): Promise<T> {
  const response = await fetch(endpoint, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(data),
  })

  if (!response.ok) {
    throw new Error(`API error: ${response.statusText}`)
  }

  return response.json()
}
```

### Step 3.2: Camera Stream Hook

**src/hooks/useCameraStream.ts**:
```typescript
import { useState, useEffect } from 'react'

export function useCameraStream(cameraId: number) {
  const [streamUrl, setStreamUrl] = useState<string>('')
  const [isLoading, setIsLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    // MJPEG stream URL
    const url = `/${cameraId}/stream`

    // Test if stream is available
    const img = new Image()
    img.onload = () => {
      setStreamUrl(url)
      setIsLoading(false)
    }
    img.onerror = () => {
      setError('Stream not available')
      setIsLoading(false)
    }
    img.src = url

    return () => {
      img.onload = null
      img.onerror = null
    }
  }, [cameraId])

  return { streamUrl, isLoading, error }
}
```

### Step 3.3: Camera Stream Component

**src/components/CameraStream.tsx**:
```typescript
import React from 'react'
import { useCameraStream } from '@/hooks/useCameraStream'

interface CameraStreamProps {
  cameraId: number
  className?: string
}

export function CameraStream({ cameraId, className = '' }: CameraStreamProps) {
  const { streamUrl, isLoading, error } = useCameraStream(cameraId)

  if (error) {
    return (
      <div className={`bg-surface-elevated rounded-lg p-4 ${className}`}>
        <p className="text-red-500">Error: {error}</p>
      </div>
    )
  }

  if (isLoading) {
    return (
      <div className={`bg-surface-elevated rounded-lg p-4 animate-pulse ${className}`}>
        <div className="h-64 bg-surface-hover rounded"></div>
      </div>
    )
  }

  return (
    <div className={`relative ${className}`}>
      <img
        src={streamUrl}
        alt={`Camera ${cameraId} stream`}
        className="w-full h-auto rounded-lg"
      />
    </div>
  )
}
```

### Step 3.4: Basic Layout

**src/components/Layout.tsx**:
```typescript
import React from 'react'
import { Outlet, Link } from 'react-router-dom'

export function Layout() {
  return (
    <div className="min-h-screen bg-surface">
      <header className="bg-surface-elevated border-b border-gray-800">
        <div className="container mx-auto px-4 py-4">
          <nav className="flex items-center justify-between">
            <h1 className="text-2xl font-bold">Motion</h1>
            <div className="flex gap-4">
              <Link to="/" className="hover:text-primary">Dashboard</Link>
              <Link to="/settings" className="hover:text-primary">Settings</Link>
              <Link to="/media" className="hover:text-primary">Media</Link>
            </div>
          </nav>
        </div>
      </header>

      <main className="container mx-auto px-4 py-8">
        <Outlet />
      </main>
    </div>
  )
}
```

### Step 3.5: App Routes

**src/App.tsx**:
```typescript
import { Routes, Route } from 'react-router-dom'
import { Layout } from './components/Layout'
import { Dashboard } from './pages/Dashboard'
import { Settings } from './pages/Settings'
import { Media } from './pages/Media'

function App() {
  return (
    <Routes>
      <Route path="/" element={<Layout />}>
        <Route index element={<Dashboard />} />
        <Route path="settings" element={<Settings />} />
        <Route path="media" element={<Media />} />
      </Route>
    </Routes>
  )
}

export default App
```

### Step 3.6: Dashboard Page

**src/pages/Dashboard.tsx**:
```typescript
import React from 'react'
import { CameraStream } from '@/components/CameraStream'

export function Dashboard() {
  // TODO: Get camera list from API
  const cameras = [0, 1]

  return (
    <div>
      <h2 className="text-3xl font-bold mb-6">Camera Dashboard</h2>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {cameras.map((camId) => (
          <div key={camId} className="bg-surface-elevated rounded-lg p-4">
            <h3 className="text-xl font-semibold mb-4">Camera {camId}</h3>
            <CameraStream cameraId={camId} />
          </div>
        ))}
      </div>
    </div>
  )
}
```

---

## Testing Workflow

### Backend Testing

1. **Build Motion with changes**:
   ```bash
   cd /Users/tshuey/Documents/GitHub/motion-motioneye
   autoreconf -fiv
   ./configure --with-libcam --with-sqlite3
   make -j4
   ```

2. **Test static file serving**:
   ```bash
   # Create test HTML file
   mkdir -p data/webui
   echo "<h1>Test</h1>" > data/webui/index.html

   # Start Motion
   ./motion -c motion.conf -n

   # In another terminal
   curl http://localhost:7999/
   # Should return the test HTML
   ```

3. **Test SPA routing**:
   ```bash
   curl http://localhost:7999/nonexistent
   # Should return index.html if webcontrol_spa_mode is on
   ```

### Frontend Testing

1. **Start Motion daemon** (one terminal):
   ```bash
   cd /Users/tshuey/Documents/GitHub/motion-motioneye
   ./motion -c motion.conf -n
   ```

2. **Start Vite dev server** (another terminal):
   ```bash
   cd /Users/tshuey/Documents/GitHub/motion-motioneye/frontend
   npm run dev
   ```

3. **Access**: http://localhost:5173

4. **Test API calls**: Open browser console, check network tab

### Production Testing

1. **Build React**:
   ```bash
   cd frontend
   npm run build
   ```

2. **Update motion.conf**:
   ```conf
   webcontrol_html_path ./data/webui
   webcontrol_spa_mode on
   ```

3. **Start Motion**:
   ```bash
   ./motion -c motion.conf -n
   ```

4. **Access**: http://localhost:7999

---

## Deployment to Raspberry Pi

### Build Locally

```bash
# Build React
cd /Users/tshuey/Documents/GitHub/motion-motioneye/frontend
npm run build

# Verify output
ls -lh ../data/webui/
```

### Transfer to Pi

```bash
# Sync code
rsync -avz --exclude='node_modules' --exclude='.git' \
  /Users/tshuey/Documents/GitHub/motion-motioneye/ \
  admin@192.168.1.176:~/motion-motioneye/

# Build Motion on Pi
ssh admin@192.168.1.176 "cd ~/motion-motioneye && autoreconf -fiv && ./configure --with-libcam --with-sqlite3 && make -j4"

# Install web UI
ssh admin@192.168.1.176 "sudo mkdir -p /usr/share/motion && sudo cp -r ~/motion-motioneye/data/webui /usr/share/motion/"

# Update config
ssh admin@192.168.1.176 "sudo nano /etc/motion/motion.conf"
# Add:
# webcontrol_html_path /usr/share/motion/webui
# webcontrol_spa_mode on

# Restart
ssh admin@192.168.1.176 "sudo systemctl restart motion"
```

### Verify

```bash
# Check Motion status
ssh admin@192.168.1.176 "sudo systemctl status motion"

# Check logs
ssh admin@192.168.1.176 "sudo journalctl -u motion -n 50 --no-pager"

# Test from browser
# http://192.168.1.176:7999/
```

---

## Next Components (Future Work)

### Configuration Forms
- Dynamic form generation from config schema
- Validation with Zod
- Save/revert functionality

### Media Browser
- Gallery view of snapshots/videos
- Thumbnail generation
- Delete functionality
- Download functionality

### System Status
- CPU/memory/temperature monitoring
- Motion detection events log
- Camera statistics

### Advanced Features
- Multi-camera grid configuration
- Motion zone editing (canvas overlay)
- Real-time event notifications
- Video playback with seek

---

## Success Criteria Checklist

- [ ] Motion compiles without errors
- [ ] New config parameters appear in `motion -h`
- [ ] Static files served from `webcontrol_html_path`
- [ ] SPA routing works (404s serve index.html)
- [ ] New API endpoints return valid JSON
- [ ] React builds without errors
- [ ] Dev server proxies API calls correctly
- [ ] Camera streams display in React
- [ ] Production build served by Motion
- [ ] Works on Raspberry Pi 5

---

## Troubleshooting

### Motion won't compile
- Check for syntax errors in C++ files
- Verify all includes are present
- Run `make clean && make` to rebuild

### Static files not served
- Check `webcontrol_html_path` in motion.conf
- Verify files exist at that path
- Check Motion logs for path validation errors

### API calls fail with CORS
- In dev: Ensure Vite proxy is configured
- In prod: Should not happen (same origin)

### React build fails
- Check Node version (need 18+)
- Delete `node_modules` and reinstall
- Check for TypeScript errors

### Streams don't display
- Verify Motion is capturing from cameras
- Check camera IDs match (0, 1, etc.)
- Open browser DevTools network tab to see errors

---

## File Structure (Final)

```
motion-motioneye/
├── src/
│   ├── parm_structs.hpp          [MODIFIED]
│   ├── conf.hpp                  [MODIFIED]
│   ├── conf.cpp                  [MODIFIED]
│   ├── webu_file.hpp             [MODIFIED]
│   ├── webu_file.cpp             [MODIFIED]
│   ├── webu_json.hpp             [MODIFIED]
│   ├── webu_json.cpp             [MODIFIED]
│   ├── webu_ans.cpp              [MODIFIED]
│   └── ...
│
├── frontend/                     [NEW]
│   ├── src/
│   │   ├── api/
│   │   │   └── client.ts
│   │   ├── components/
│   │   │   ├── Layout.tsx
│   │   │   └── CameraStream.tsx
│   │   ├── hooks/
│   │   │   └── useCameraStream.ts
│   │   ├── pages/
│   │   │   ├── Dashboard.tsx
│   │   │   ├── Settings.tsx
│   │   │   └── Media.tsx
│   │   ├── App.tsx
│   │   ├── main.tsx
│   │   └── index.css
│   ├── package.json
│   ├── vite.config.ts
│   ├── tailwind.config.js
│   └── tsconfig.json
│
├── data/
│   └── webui/                    [BUILD OUTPUT]
│       ├── index.html
│       └── assets/
│           ├── index-[hash].js
│           └── index-[hash].css
│
└── doc/
    ├── plans/react-ui/
    │   ├── 01-architecture-20251227.md
    │   └── 02-detailed-execution-20251227.md
    └── sub-agent-summaries/
        └── (sub-agent work documented here)
```

---

## Estimated Timeline

| Phase | Tasks | Duration |
|-------|-------|----------|
| 1.1 | Config parameters | 1 hour |
| 1.2 | SPA routing | 2-3 hours |
| 1.3 | JSON API | 2-3 hours |
| 1.4 | Backend testing | 1 hour |
| 2.1-2.5 | React setup | 2 hours |
| 3.1-3.6 | Core components | 3-4 hours |
| Testing | Dev + Pi deployment | 2 hours |
| **Total** | | **13-16 hours** |

---

## Ready to Execute

All prerequisite planning is complete. Begin with Phase 1.1: Add configuration parameters.
