#!/bin/bash
# Motion Notification Dispatcher
# Called by Motion via on_event_* hooks
#
# Usage: motion-notify.sh <event_type> <camera_id> [file_path]
# Events: start, end, picture, movie_start, movie_end
#
# Configuration: /etc/motion/notify.conf
# Environment variables can also be used.

SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
CONFIG_FILE="${MOTION_NOTIFY_CONF:-/etc/motion/notify.conf}"

# Load configuration
if [[ -f "$CONFIG_FILE" ]]; then
    source "$CONFIG_FILE"
fi

EVENT_TYPE="${1:-unknown}"
CAMERA_ID="${2:-0}"
FILE_PATH="${3:-}"
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

# Logging helper
log() {
    logger -t "motion-notify" "$@"
}

# Email notification (uses sendmail/msmtp)
send_email() {
    if [[ "$NOTIFY_EMAIL_ENABLED" != "true" ]]; then
        return
    fi

    local subject="Motion Alert - Camera $CAMERA_ID"
    local body="Event: $EVENT_TYPE\nTime: $TIMESTAMP\nCamera: $CAMERA_ID"

    if [[ -n "$FILE_PATH" ]]; then
        body="$body\nFile: $FILE_PATH"
    fi

    if command -v msmtp &> /dev/null; then
        echo -e "Subject: $subject\n\n$body" | msmtp "$NOTIFY_EMAIL_TO"
    elif command -v sendmail &> /dev/null; then
        echo -e "Subject: $subject\n\n$body" | sendmail "$NOTIFY_EMAIL_TO"
    else
        log "Email not sent - no mail client found"
    fi
}

# Telegram notification
send_telegram() {
    if [[ "$NOTIFY_TELEGRAM_ENABLED" != "true" ]]; then
        return
    fi

    if [[ -z "$NOTIFY_TELEGRAM_TOKEN" || -z "$NOTIFY_TELEGRAM_CHAT" ]]; then
        log "Telegram not configured"
        return
    fi

    local message="🚨 *Motion Detected*%0A"
    message+="Camera: $CAMERA_ID%0A"
    message+="Event: $EVENT_TYPE%0A"
    message+="Time: $TIMESTAMP"

    curl -s -X POST \
        "https://api.telegram.org/bot${NOTIFY_TELEGRAM_TOKEN}/sendMessage" \
        -d "chat_id=${NOTIFY_TELEGRAM_CHAT}" \
        -d "text=${message}" \
        -d "parse_mode=Markdown" \
        > /dev/null 2>&1 &
}

# Webhook notification
send_webhook() {
    if [[ "$NOTIFY_WEBHOOK_ENABLED" != "true" ]]; then
        return
    fi

    if [[ -z "$NOTIFY_WEBHOOK_URL" ]]; then
        log "Webhook not configured"
        return
    fi

    local payload=$(cat <<EOF
{
    "event": "$EVENT_TYPE",
    "camera_id": "$CAMERA_ID",
    "timestamp": "$TIMESTAMP",
    "file_path": "$FILE_PATH"
}
EOF
)

    curl -s -X POST \
        -H "Content-Type: application/json" \
        -d "$payload" \
        "$NOTIFY_WEBHOOK_URL" \
        > /dev/null 2>&1 &
}

# Main dispatch
main() {
    log "Event: $EVENT_TYPE, Camera: $CAMERA_ID, File: $FILE_PATH"

    case "$EVENT_TYPE" in
        start)
            send_email
            send_telegram
            send_webhook
            ;;
        picture)
            # Optionally send with attachment
            send_webhook
            ;;
        movie_end)
            send_webhook
            ;;
        *)
            send_webhook
            ;;
    esac
}

main
