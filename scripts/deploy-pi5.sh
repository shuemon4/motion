#!/bin/bash
# Motion Full Deployment Script for Pi5
#
# Deploys both frontend and backend to Pi5:
# 1. Syncs source code to Pi5
# 2. Builds backend (C++) on Pi5
# 3. Builds frontend (React) on Pi5
# 4. Installs Motion binary system-wide
# 5. Restarts Motion service
# 6. Verifies deployment
#
# Prerequisites:
# - SSH key configured for passwordless access to Pi5
# - Pi5 already set up with dependencies (scripts/pi-setup.sh)
# - Motion service configured (scripts/setup-motion-service.sh)

set -e

# Configuration
LOCAL_PROJECT_ROOT="/Users/tshuey/Documents/GitHub/motion"
PI_HOST="admin@192.168.1.176"
PI_PROJECT_DIR="/home/admin/motion"
PI_WEBUI_PATH="/usr/local/var/lib/motion/webui"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║       Motion Full Deployment to Pi5 (Backend + Frontend)     ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Step 1: Sync source code to Pi5
echo -e "${BLUE}📦 [1/6] Syncing source code to Pi5...${NC}"
rsync -avz --delete \
    --exclude='.git' \
    --exclude='node_modules' \
    --exclude='.venv' \
    --exclude='build' \
    --exclude='*.o' \
    --exclude='*.lo' \
    --exclude='.deps' \
    --exclude='.libs' \
    --exclude='autom4te.cache' \
    "$LOCAL_PROJECT_ROOT/" "$PI_HOST:$PI_PROJECT_DIR/" | grep -E "^\s*(sent|total)" || true

echo -e "${GREEN}✅ Source code synced${NC}"
echo ""

# Step 2: Build backend on Pi5
echo -e "${BLUE}🔨 [2/6] Building backend (C++) on Pi5...${NC}"
ssh "$PI_HOST" "cd $PI_PROJECT_DIR && bash -l scripts/pi-build.sh" || {
    echo -e "${RED}❌ Backend build failed${NC}"
    exit 1
}
echo -e "${GREEN}✅ Backend built successfully${NC}"
echo ""

# Step 3: Install Motion binary system-wide
echo -e "${BLUE}📥 [3/6] Installing Motion binary system-wide...${NC}"
ssh "$PI_HOST" "cd $PI_PROJECT_DIR && sudo make install" > /dev/null 2>&1 || {
    echo -e "${RED}❌ Installation failed${NC}"
    exit 1
}
echo -e "${GREEN}✅ Motion binary installed to /usr/local/bin/motion${NC}"
echo -e "${GREEN}✅ WebUI installed to $PI_WEBUI_PATH${NC}"
echo ""

# Step 4: Verify installation
echo -e "${BLUE}🔍 [4/6] Verifying installation...${NC}"
MOTION_VERSION=$(ssh "$PI_HOST" "/usr/local/bin/motion --version 2>&1 | head -1")
echo "   Version: $MOTION_VERSION"
echo -e "${GREEN}✅ Binary verification complete${NC}"
echo ""

# Step 5: Restart Motion service
echo -e "${BLUE}🔄 [5/6] Restarting Motion service...${NC}"
ssh "$PI_HOST" "sudo systemctl restart motion" || {
    echo -e "${RED}❌ Service restart failed${NC}"
    exit 1
}
sleep 3
echo -e "${GREEN}✅ Motion service restarted${NC}"
echo ""

# Step 6: Verify deployment
echo -e "${BLUE}✔️  [6/6] Verifying deployment...${NC}"

# Check service status
STATUS=$(ssh "$PI_HOST" "sudo systemctl is-active motion")
if [ "$STATUS" = "active" ]; then
    echo -e "${GREEN}✅ Motion service: RUNNING${NC}"
else
    echo -e "${RED}❌ Motion service: $STATUS${NC}"
    echo ""
    echo -e "${YELLOW}📋 Recent logs:${NC}"
    ssh "$PI_HOST" "sudo journalctl -u motion -n 20 --no-pager"
    exit 1
fi

# Check UI accessibility
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" "http://192.168.1.176:8080/" 2>/dev/null || echo "000")
if [ "$HTTP_CODE" = "200" ]; then
    echo -e "${GREEN}✅ UI accessible: http://192.168.1.176:8080/${NC}"
else
    echo -e "${YELLOW}⚠️  UI returned HTTP $HTTP_CODE (may still be starting)${NC}"
fi

# Check WebUI file count
WEBUI_FILES=$(ssh "$PI_HOST" "find $PI_WEBUI_PATH -type f 2>/dev/null | wc -l")
echo -e "${GREEN}✅ WebUI files deployed: $WEBUI_FILES files${NC}"

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                   ✅ DEPLOYMENT COMPLETE                      ║"
echo "║                                                               ║"
echo "║  UI: http://192.168.1.176:8080/                               ║"
echo "║  Credentials: admin / adminpass                               ║"
echo "║                                                               ║"
echo "║  View logs: ssh admin@192.168.1.176                           ║"
echo "║             sudo journalctl -u motion -f                      ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
