# HANDOFF: React UI Implementation for Motion

**Created**: 2025-12-27
**Status**: Ready for execution
**Repository**: `/Users/tshuey/Documents/GitHub/motion-motioneye/`

---

## Mission

Implement a modern React frontend for Motion (C++ video surveillance daemon). The React app will be served directly by Motion's built-in web server, eliminating the need for any Python/MotionEye layer.

---

## Repository State

```
motion-motioneye/
├── src/                    # Motion C++ source (35,807 lines, 68 files)
│   ├── webu*.cpp/hpp       # Web server components (to be extended)
│   ├── conf.cpp/hpp        # Configuration system (~600 parameters)
│   └── ...                 # Core Motion functionality (DO NOT MODIFY)
├── frontend/               # EMPTY - React app goes here
├── docs/
│   ├── plans/react-ui/     # Implementation plans (READ FIRST)
│   │   ├── 01-architecture-20251227.md
│   │   └── 02-detailed-execution-20251227.md
│   └── sub-agent-summaries/  # Sub-agent work summaries (REQUIRED)
└── ...
```

---

## Critical Rules

### 1. Sub-Agent Summary Requirement

**Every sub-agent MUST create a summary file** in:
```
/Users/tshuey/Documents/GitHub/motion-motioneye/docs/sub-agent-summaries/
```

**Naming format**: `YYYYMMDD-HHMM-{agent-task-description}.md`

**Required content**:
```markdown
# Sub-Agent Summary: {Task Description}

**Date**: YYYY-MM-DD HH:MM
**Agent Type**: {Explore | Plan | Implementation}
**Task**: {Brief description}

## Work Performed
- {List of actions taken}

## Files Created/Modified
- {List of files with brief description of changes}

## Key Findings
- {Important discoveries or decisions made}

## Status
- {Complete | Partial | Blocked}
- {Any follow-up needed}
```

### 2. DO NOT Use Existing Motion UI

Motion has an embedded HTML UI in `src/webu_html.cpp`. **DO NOT USE IT**.

We are building a completely new React UI from scratch. The existing UI is:
- Embedded C++ string generation (hard to maintain)
- jQuery-based (outdated)
- Not the target architecture

### 3. Core Motion Files - DO NOT MODIFY

Unless explicitly required by the plan, do not modify:
- `src/alg.cpp/hpp` - Motion detection algorithm
- `src/camera.cpp/hpp` - Camera processing loop
- `src/libcam.cpp/hpp` - Pi camera integration
- `src/movie.cpp/hpp` - Video recording
- `src/netcam.cpp/hpp` - Network camera support

### 4. Files TO MODIFY (Phase 1)

These C++ files need extensions:
- `src/webu_file.cpp` - Add SPA routing support
- `src/webu_json.cpp` - Add new API endpoints
- `src/webu_ans.cpp` - Add API routing
- `src/conf.cpp/hpp` - Add new configuration parameters

---

## Implementation Plans

**READ THESE FIRST**:

1. **Architecture**: `docs/plans/react-ui/01-architecture-20251227.md`
   - System design
   - API endpoints needed
   - C++ modification overview

2. **Detailed Execution**: `docs/plans/react-ui/02-detailed-execution-20251227.md`
   - Task breakdown
   - Implementation order
   - Code templates

---

## Phase 1: C++ Web Server Extensions

### 1.1 Add Configuration Parameters

**File**: `src/conf.cpp`, `src/conf.hpp`

Add these parameters:
```cpp
// In conf.hpp - add to cls_config class
std::string webcontrol_html_path;  // Path to React build (default: /usr/share/motion/webui)
bool webcontrol_spa_mode;          // Enable SPA routing (default: true)

// In conf.cpp - add to config_parms[]
{"webcontrol_html_path", PARM_TYP_STRING, PARM_CAT_WEB, PARM_LEVEL_LIMITED},
{"webcontrol_spa_mode", PARM_TYP_BOOL, PARM_CAT_WEB, PARM_LEVEL_LIMITED},
```

### 1.2 Add SPA Support to webu_file.cpp

Modify `cls_webu_file::main()` to:
1. Serve files from `webcontrol_html_path`
2. If file not found and `webcontrol_spa_mode` is on, serve `index.html`
3. Set appropriate MIME types and cache headers

### 1.3 Extend JSON API

Add to `src/webu_json.cpp`:
- `GET /api/auth/me` - Current user info
- `GET /api/media/pictures/{cam}` - List snapshots
- `DELETE /api/media/pictures/{id}` - Delete snapshot
- `GET /api/system/temperature` - CPU temperature

---

## Phase 2: React Application Setup

### 2.1 Initialize in `/frontend`

```bash
cd /Users/tshuey/Documents/GitHub/motion-motioneye/frontend
npm create vite@latest . -- --template react-ts
npm install
npm install -D tailwindcss postcss autoprefixer
npm install zustand @tanstack/react-query react-hook-form zod @hookform/resolvers
npx tailwindcss init -p
```

### 2.2 Configure for Motion Integration

**vite.config.ts**:
```typescript
export default defineConfig({
  plugins: [react()],
  build: {
    outDir: '../data/webui',  // Motion will serve from here
  },
  server: {
    proxy: {
      '/api': 'http://localhost:7999',
      '/0': 'http://localhost:7999',
      '/1': 'http://localhost:7999',
      // Add more camera IDs as needed
    },
  },
});
```

### 2.3 Dark Theme (Tailwind)

```javascript
// tailwind.config.js
export default {
  content: ['./index.html', './src/**/*.{js,ts,jsx,tsx}'],
  theme: {
    extend: {
      colors: {
        surface: {
          DEFAULT: '#1a1a1a',
          elevated: '#262626',
        },
      },
    },
  },
};
```

---

## Phase 3-4: Component Implementation

See `docs/plans/react-ui/02-detailed-execution-20251227.md` for:
- Component inventory (20+ components)
- Camera streaming hooks
- Configuration form architecture
- Media gallery implementation

---

## Testing on Raspberry Pi

### SSH Access

| Device | IP | Command |
|--------|-----|---------|
| Pi 5 | 192.168.1.176 | `ssh admin@192.168.1.176` |
| Pi 4 | 192.168.1.246 | `ssh admin@192.168.1.246` |

### Deployment

```bash
# Sync code to Pi
rsync -avz --exclude='node_modules' --exclude='.git' \
  /Users/tshuey/Documents/GitHub/motion-motioneye/ \
  admin@192.168.1.176:~/motion-motioneye/

# Build on Pi
ssh admin@192.168.1.176 "cd ~/motion-motioneye && autoreconf -fiv && ./configure --with-libcam && make -j4"

# Run in foreground for testing
ssh admin@192.168.1.176 "cd ~/motion-motioneye && ./motion -c /etc/motion/motion.conf -n"
```

---

## Success Criteria

1. **React build served by Motion** - No separate server
2. **Camera streams display** - Direct MJPEG from Motion
3. **Configuration works** - Read/write via JSON API
4. **RAM < 30MB** - Significant reduction from MotionEye
5. **All sub-agents documented** - Summaries in designated folder

---

## Getting Started

1. **Read the plans**:
   ```bash
   cat docs/plans/react-ui/01-architecture-20251227.md
   cat docs/plans/react-ui/02-detailed-execution-20251227.md
   ```

2. **Understand existing web code**:
   ```bash
   # Key files to analyze
   cat src/webu.hpp          # Web server class
   cat src/webu_file.cpp     # Static file serving (modify this)
   cat src/webu_json.cpp     # JSON API (extend this)
   ```

3. **Start with Phase 1.1** - Add config parameters first

4. **Document everything** - Every sub-agent creates a summary

---

## Questions?

If unclear on any aspect:
1. Check the plan documents first
2. Analyze the existing C++ code patterns
3. Ask the user for clarification

Do not guess on architectural decisions. The plans have been reviewed and approved.
