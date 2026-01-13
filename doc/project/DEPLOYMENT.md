# Deployment Guide

## Quick Deploy: Pi5

The fastest way to deploy UI changes to Pi5:

```bash
~/motion-deploy-pi5.sh
```

This script:
1. ✅ Builds React frontend (Vite)
2. ✅ Transfers assets to Pi5
3. ✅ Restarts Motion service
4. ✅ Verifies deployment

Takes ~2 minutes end-to-end.

---

## What Gets Deployed

### Frontend Assets
- React build output (production optimized)
- CSS, JS bundles (gzipped)
- Static files (HTML, SVG, etc.)
- **Location on Pi5**: `/usr/local/var/lib/motion/webui/`

### Backend (C++ Motion)
- Built from `src/`
- Uses existing build on Pi5 (not deployed by UI script)
- If C++ changes needed, see [Motion Build](../project/Motion-Build.md)

---

## Step-by-Step Manual Deployment

If you need more control or are troubleshooting:

### 1. Build Frontend
```bash
cd /Users/tshuey/Documents/GitHub/motion/frontend
npm run build
```

Output goes to: `../data/webui/`

### 2. Transfer Assets
```bash
rsync -avz /Users/tshuey/Documents/GitHub/motion/data/webui/ \
  admin@192.168.1.176:/tmp/webui_temp/
```

### 3. Install on Pi5
```bash
ssh admin@192.168.1.176 << 'EOF'
sudo cp -r /tmp/webui_temp/* /usr/local/var/lib/motion/webui/
sudo chown -R www-data:www-data /usr/local/var/lib/motion/webui/
rm -rf /tmp/webui_temp
EOF
```

### 4. Restart Service
```bash
ssh admin@192.168.1.176 "sudo systemctl restart motion"
```

### 5. Verify
```bash
# Check service
ssh admin@192.168.1.176 "sudo systemctl status motion"

# Check UI
curl http://192.168.1.176:8080/
```

---

## Troubleshooting

### UI Returns 404 or doesn't load

**Check file permissions:**
```bash
ssh admin@192.168.1.176 "ls -la /usr/local/var/lib/motion/webui/"
```

Should be owned by `www-data:www-data`

**Verify Motion is serving files:**
```bash
ssh admin@192.168.1.176 "curl -s http://localhost:8080/ | head -5"
```

### Service won't restart

**Check logs:**
```bash
ssh admin@192.168.1.176 "sudo journalctl -u motion -n 20 --no-pager"
```

**Manually kill orphaned processes:**
```bash
ssh admin@192.168.1.176 "sudo pkill -9 motion && sudo systemctl restart motion"
```

### API calls return 401

**Check authentication:**
```bash
curl -s http://192.168.1.176:8080/0/api/auth/status | jq .
```

Should return: `{"auth_required":true,"authenticated":false}`

**Login and retry:**
```bash
TOKEN=$(curl -s -X POST http://192.168.1.176:8080/0/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"adminpass"}' | jq -r '.session_token')

curl -H "X-Session-Token: $TOKEN" http://192.168.1.176:8080/0/api/config | jq .
```

---

## Backend Deployment (C++)

If you need to deploy C++ changes:

```bash
# From project root
rsync -avz --delete \
  --exclude='.git' \
  --exclude='*.o' \
  --exclude='.libs' \
  /Users/tshuey/Documents/GitHub/motion/ admin@192.168.1.176:~/motion/

ssh admin@192.168.1.176 << 'EOF'
cd ~/motion
autoreconf -fiv
./configure --with-libcam --with-sqlite3
make -j4
sudo make install
sudo systemctl restart motion
EOF
```

See [Motion Build](../project/Motion-Build.md) for details.

---

## Deployment Checklist

### Before Deploying
- [ ] Frontend builds without errors: `npm run build`
- [ ] No TypeScript errors: `cd frontend && npm run build`
- [ ] Changes are committed to git
- [ ] Pi5 is running and accessible: `ping 192.168.1.176`

### During Deployment
- [ ] Run script: `~/motion-deploy-pi5.sh`
- [ ] Watch for errors (script will fail if anything goes wrong)

### After Deployment
- [ ] Service is running: `ssh admin@192.168.1.176 "sudo systemctl status motion"`
- [ ] UI loads: `curl http://192.168.1.176:8080/`
- [ ] Can login: Try credentials `admin`/`adminpass`
- [ ] Check logs for any warnings: `ssh admin@192.168.1.176 "sudo journalctl -u motion -n 10"`

---

## Environment Info

### Deployment Target
```
Host:     admin@192.168.1.176
Hostname: pi5-motioneye
OS:       Raspberry Pi OS (64-bit)
Arch:     aarch64 (ARM64)
```

### Web Paths
```
UI:       http://192.168.1.176:8080/
API:      http://192.168.1.176:8080/0/api/
Streams:  http://192.168.1.176:8080/1/mjpg/stream
Config:   /usr/local/etc/motion/motion.conf
WebUI:    /usr/local/var/lib/motion/webui/
```

### Credentials
```
Admin:  admin / adminpass
Viewer: viewer / viewpass
```

---

## Performance Tips

1. **Gzipped assets** - Vite builds with gzip compression for faster transfer
2. **No cache busting** - Assets are versioned by hash in filename
3. **Minimal rebuild** - Only rebuild what changed: `npm run build`

---

## Related Documentation

- [Motion Build Guide](./Motion-Build.md) - Building C++ backend
- [Architecture](./ARCHITECTURE.md) - System design overview
- [Modification Guide](./MODIFICATION_GUIDE.md) - How to modify code safely
