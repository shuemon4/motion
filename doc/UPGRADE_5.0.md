# Upgrading to Motion 5.0

**Last Updated:** 2026-01-27

This guide helps you upgrade from Motion 4.x to Motion 5.0, which includes significant improvements in FFmpeg compatibility, directory layout, and platform support.

---

## What's New in Motion 5.0

### FFmpeg 5.0+ Support
- **Minimum version**: FFmpeg 5.0 (released 2022)
- **Supported versions**: FFmpeg 5.x, 6.x, 7.x
- **Compatibility layer**: Automatic API version detection
- **Performance**: Improved hardware acceleration support

### Hybrid Directory Layout
- **System configs**: `/etc/motion/` or `/usr/local/etc/motion/`
- **User overrides**: `/var/lib/motion/user-config/` - **No sudo required!**
- **Profiles**: `/var/lib/motion/profiles/` - Day/night/away presets
- **Runtime data**: `/var/lib/motion/runtime/` - PID files, active profile

### Platform-Agnostic Design
- **Core Motion**: Works on all Linux platforms
- **Pi Tools**: Optional, auto-detected during installation
- **Modular approach**: Only installs what you need

---

## Prerequisites

### Check Your FFmpeg Version

```bash
ffmpeg -version | head -1
```

**Required:** FFmpeg 5.0 or newer (major version ≥ 5)

### Upgrade FFmpeg if Needed

#### Ubuntu 22.04 LTS / 24.04 LTS
```bash
# Check available version
apt-cache policy libavformat-dev

# If < 5.0, use backports or PPA
sudo add-apt-repository ppa:savoury1/ffmpeg5
sudo apt update
sudo apt install libavformat-dev libavcodec-dev libavutil-dev \
  libswscale-dev libavdevice-dev
```

#### Debian 11 (Bullseye) → Debian 12 (Bookworm)
```bash
# Debian 11 has FFmpeg 4.x
# Debian 12 has FFmpeg 5.x (recommended upgrade path)
sudo apt update
sudo apt install ffmpeg libavformat-dev libavcodec-dev libavutil-dev \
  libswscale-dev libavdevice-dev
```

#### Compile from Source
```bash
# See: https://motion-project.github.io/motion_build.html
wget https://ffmpeg.org/releases/ffmpeg-6.0.tar.xz
tar xf ffmpeg-6.0.tar.xz
cd ffmpeg-6.0
./configure --prefix=/usr/local --enable-shared
make -j4
sudo make install
sudo ldconfig
```

---

## Installation

### From Source

```bash
# 1. Clone or download Motion 5.0
git clone https://github.com/Motion-Project/motion.git
cd motion

# 2. Check FFmpeg version (configure will error if < 5.0)
autoreconf -fiv
./configure --with-libcam --with-sqlite3

# 3. Build
make -j4

# 4. Install
sudo make install

# 5. (Raspberry Pi only) Install Pi tools if prompted
sudo make install-pi-tools
```

### Post-Installation

```bash
# Verify installation
motion --version

# Check FFmpeg support
motion -h | grep -A5 FFmpeg
```

---

## Configuration Migration

### Your Existing Configs Work!

**Good news:** Motion 5.0 is backward compatible. Your existing configs in `/etc/motion/` will continue to work without changes.

### New Feature: User-Editable Configs (No Sudo!)

Motion 5.0 introduces layered configuration:

```
Priority: System → User → Profile → CLI
```

#### Config Loading Order

1. **System defaults**: `/etc/motion/motion.conf` (or `/usr/local/etc/motion/`)
2. **User overrides**: `/var/lib/motion/user-config/local.conf`
3. **Active profile**: `/var/lib/motion/profiles/{profile}.conf`
4. **Command-line**: `-c` flag or individual parameter overrides

#### Example: Override Settings Without Sudo

**System config** (`/etc/motion/motion.conf` - managed by admin):
```
threshold 2500
framerate 15
webcontrol_port 8080
```

**User override** (`/var/lib/motion/user-config/local.conf` - you can edit):
```
# I want higher sensitivity for my use case
threshold 3000

# My camera supports higher framerate
framerate 20
```

**Result**: Motion uses `threshold 3000` and `framerate 20`, with other settings from system config.

#### Profile-Based Configuration

Motion 5.0 includes pre-configured profiles for common scenarios:

```bash
# Day profile (high sensitivity)
/var/lib/motion/profiles/day.conf

# Night profile (noise reduction)
/var/lib/motion/profiles/night.conf

# Away profile (maximum detection)
/var/lib/motion/profiles/away.conf
```

**Activate a profile:**
```bash
# Option 1: Via config
echo "day" > /var/lib/motion/runtime/active-profile.txt
systemctl restart motion

# Option 2: Via UI (future feature)
# Navigate to Settings → Profiles → Select "Day"
```

---

## Directory Structure

### What Changed

| Purpose | Old (4.x) | New (5.0) |
|---------|-----------|-----------|
| System config | `/etc/motion/` | `/etc/motion/` ✅ Same |
| Web UI | Embedded HTML | `/var/lib/motion/webui/` (React) |
| User overrides | N/A | `/var/lib/motion/user-config/` **New!** |
| Profiles | N/A | `/var/lib/motion/profiles/` **New!** |
| Runtime data | `/var/run/motion/` | `/var/lib/motion/runtime/` |

### Migration Steps

```bash
# 1. Your existing configs automatically work - no migration needed!

# 2. (Optional) Move user customizations to new location
sudo cp /etc/motion/motion.conf /var/lib/motion/user-config/local.conf
sudo nano /var/lib/motion/user-config/local.conf
# Remove system-level settings, keep only your customizations

# 3. (Optional) Create custom profiles
sudo cp /var/lib/motion/profiles/day.conf /var/lib/motion/profiles/my-custom.conf
sudo nano /var/lib/motion/profiles/my-custom.conf
```

---

## Platform-Specific Features

### Raspberry Pi

Motion 5.0 detects Raspberry Pi automatically during installation and offers optional Pi-specific tools.

#### Install Pi Tools (Recommended for Pi Users)

```bash
sudo make install-pi-tools
```

**What you get:**
- `motion-setup-pi` - Interactive camera configuration wizard
- `pi-build.sh` - Optimized build script for Pi hardware
- `pi-configure.sh` - Camera auto-detection and setup
- Pi-specific examples and documentation

#### Pi Camera Support

Motion 5.0 works with:
- **Pi 5**: libcamera (required - legacy stack removed)
- **Pi 4**: libcamera or V4L2 (auto-detected)
- **Pi 3 and older**: V4L2

**Check your camera:**
```bash
# Pi 5 / Pi 4 with libcamera
rpicam-hello --list-cameras

# Pi 4 / Pi 3 with V4L2
v4l2-ctl --list-devices
```

### Other Linux Platforms

Motion 5.0 is fully platform-agnostic:
- **Ubuntu / Debian**: Standard package installation
- **Fedora / RHEL**: Build from source or RPM
- **Arch**: AUR package
- **Generic Linux**: V4L2 support works universally

---

## Testing Your Installation

### 1. Verify Build

```bash
# Check Motion version
motion --version

# Should show:
# motion Version 5.0.x
# FFmpeg support: 5.0+ (or your version)
```

### 2. Test Configuration Loading

```bash
# Run in foreground with debug output
motion -c /etc/motion/motion.conf -n -d

# Check for config loading messages:
# [NTC] Loading system config: /etc/motion/motion.conf
# [NTC] Loading user config: /var/lib/motion/user-config/local.conf
# [NTC] No active profile set
```

### 3. Verify Directory Structure

```bash
# Check all directories were created
ls -la /var/lib/motion/

# Should show:
# drwxr-xr-x webui/
# drwxrw-r-- user-config/
# drwxrw-r-- profiles/
# drwxrwxr-x runtime/
```

### 4. Test Camera Detection

```bash
# Run camera detection
motion --detect-cameras

# Or use Pi tool (if installed)
motion-setup-pi
```

### 5. Access Web UI

```bash
# Start Motion
sudo systemctl start motion

# Open browser
http://localhost:8080/

# Or on network
http://<motion-host>:8080/
```

---

## Troubleshooting

### FFmpeg Version Too Old

**Error:**
```
configure: error: Motion requires FFmpeg 5.0 or newer. Found: 4.4.2
```

**Solution:**
1. Ubuntu 22.04/24.04: Use backports (see Prerequisites section)
2. Compile FFmpeg from source
3. Upgrade to a newer distribution (Debian 12, Ubuntu 24.04)

### Build Fails: "Unknown type MY_CODEC_ID_H264"

**Cause:** FFmpeg headers not found or too old.

**Solution:**
```bash
# Install FFmpeg development headers
sudo apt install libavformat-dev libavcodec-dev libavutil-dev \
  libswscale-dev libavdevice-dev

# Clean and rebuild
make clean
./configure --with-libcam --with-sqlite3
make -j4
```

### "Unable to connect to Motion" in Web UI

**Cause:** Authentication not configured.

**Solution:**
```bash
# Option 1: Disable auth for testing
echo "webcontrol_auth_method none" >> /var/lib/motion/user-config/local.conf
sudo systemctl restart motion

# Option 2: Configure authentication
sudo motion-setup-pi  # Interactive setup (Pi only)

# Or manually edit:
sudo nano /etc/motion/motion.conf
# Add:
# webcontrol_auth_method basic
# webcontrol_user admin
# webcontrol_password yourpassword
```

### Pi Tools Won't Install on Non-Pi System

**Error:**
```
ERROR: This is not a Raspberry Pi.
Pi tools are only compatible with Raspberry Pi systems.
```

**Explanation:** This is expected! Pi tools are Raspberry Pi-specific. They won't work on other platforms and the installer prevents accidental installation.

### User Config Not Loading

**Debug:**
```bash
# Check file exists and is readable
ls -la /var/lib/motion/user-config/local.conf

# Check Motion logs for config loading
sudo journalctl -u motion -f | grep "config"

# Verify syntax
cat /var/lib/motion/user-config/local.conf
# Ensure: one setting per line, no extra spaces
```

### Directory Permissions

**If Motion can't write to runtime directory:**
```bash
# Fix permissions
sudo chown motion:motion /var/lib/motion/runtime
sudo chmod 775 /var/lib/motion/runtime

# If motion user doesn't exist:
sudo useradd -r -s /bin/false motion
sudo chown -R motion:motion /var/lib/motion
```

---

## Rollback to Motion 4.x

If you need to rollback:

### 1. Uninstall Motion 5.0

```bash
cd /path/to/motion/source
sudo make uninstall
```

### 2. Reinstall Motion 4.x

```bash
# From distribution packages
sudo apt install motion

# Or from Motion 4.x source
git checkout 4.x-branch
autoreconf -fiv
./configure
make -j4
sudo make install
```

### 3. Clean Up New Directories (Optional)

```bash
# Remove Motion 5.0 directories if not needed
sudo rm -rf /var/lib/motion/user-config
sudo rm -rf /var/lib/motion/profiles
sudo rm -rf /var/lib/motion/runtime
sudo rm -rf /var/lib/motion/webui
```

**Note:** Your original configs in `/etc/motion/` remain untouched.

---

## Getting Help

### Documentation
- Motion Guide: https://motion-project.github.io/motion_guide.html
- Motion Config: https://motion-project.github.io/motion_config.html
- Build Guide: https://motion-project.github.io/motion_build.html

### Community
- GitHub Issues: https://github.com/Motion-Project/motion/issues
- Forum: https://github.com/Motion-Project/motion/discussions

### Reporting Bugs

When reporting issues, include:
```bash
# Motion version
motion --version

# FFmpeg version
ffmpeg -version | head -1

# Platform info
uname -a
cat /etc/os-release

# Config (sanitized - remove passwords!)
cat /etc/motion/motion.conf

# Logs
sudo journalctl -u motion -n 100 --no-pager
```

---

## FAQ

**Q: Do I need to change my existing configs?**
A: No! Existing configs in `/etc/motion/` continue to work unchanged.

**Q: What if I don't have FFmpeg 5.0?**
A: Motion 5.0 requires FFmpeg 5.0+. See Prerequisites section for upgrade instructions.

**Q: Can I still use the old web UI?**
A: No, the legacy HTML UI was replaced with a modern React UI in Motion 5.0.

**Q: Is libcamera required?**
A: Only on Raspberry Pi 5. Pi 4 and other platforms can use V4L2.

**Q: What's the benefit of user-config/?**
A: You can customize Motion settings without sudo access - the user-config directory is writable by the motion user.

**Q: Do profiles override user-config?**
A: Yes. Loading order is: system → user-config → profile → CLI. Later settings override earlier ones.

**Q: Can I use Motion 5.0 on Ubuntu 20.04?**
A: Ubuntu 20.04 reaches EOL in April 2025 and ships FFmpeg 4.x. Upgrade to Ubuntu 22.04 or 24.04 is recommended.

**Q: What happened to the Pi-specific scripts in the main repo?**
A: They're now optional via `make install-pi-tools` to keep Motion platform-agnostic.

---

## Changelog Summary

### Breaking Changes
- Minimum FFmpeg version: 5.0 (was: 4.x)
- Legacy HTML web UI removed (replaced with React)
- Directory structure extended (backward compatible)

### New Features
- FFmpeg 5.x, 6.x, 7.x support with automatic version detection
- User-editable configs without sudo (`/var/lib/motion/user-config/`)
- Profile-based configuration (`/var/lib/motion/profiles/`)
- Platform detection and optional Pi tools
- Modern React web UI with comprehensive features

### Improvements
- Better hardware acceleration support
- Cleaner build system (m4 directory preserved)
- Platform-agnostic design
- Improved configure-time error messages

### Deprecated
- FFmpeg < 5.0 support
- Embedded HTML UI
- `webcontrol` naming (use `webui`)

---

**Happy motion detecting!**

For questions or issues, see the Getting Help section above.
