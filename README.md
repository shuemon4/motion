Motion
=============

## Description

Motion is a program that monitors the signal from video cameras and detects changes in the images.

## Authentication Setup

Motion requires authentication to be configured before production use.

### Quick Setup (CLI Tool)

For custom passwords, use the setup tool:

```bash
sudo motion-setup
```

Follow prompts to create admin and viewer accounts. Passwords will be hashed with bcrypt (work factor 12).

### Password Reset

If you forget your password:

```bash
sudo motion-setup --reset
```

### Changing Passwords

After logging in as admin:
1. Navigate to Settings > System
2. Enter new passwords
3. Click Save
4. Passwords will be hashed automatically

### Manual Password Hash Generation

If you need to generate a password hash manually:

```bash
# Using Python
pip3 install bcrypt
python3 -c "import bcrypt; print(bcrypt.hashpw(b'yourpassword', bcrypt.gensalt(12)).decode())"
```

Then edit `/usr/local/etc/motion/motion.conf`:

```conf
webcontrol_authentication admin:$2b$12$abcdefghijk...xyz
```

Restart Motion:

```bash
sudo systemctl restart motion
```

## Security

- Admin username is fixed as "admin" (prevents enumeration)
- Passwords hashed with bcrypt (work factor 12)
- Sessions expire after 1 hour (configurable via `webcontrol_session_timeout`)
- Use HTTPS in production (reverse proxy recommended)

## Configuration

**Authentication Parameters**:

```conf
# Admin account (full access)
webcontrol_authentication admin:$2b$12$hash...

# Viewer account (read-only access)
webcontrol_user_authentication viewer:$2b$12$hash...

# Session timeout (seconds, default 3600)
webcontrol_session_timeout 3600
```

## Resources

Please see the [Motion home page](https://motion-project.github.io/) for information regarding building the Motion code from source, documentation of the current and prior releases as well as recent news associated with the application.  Review the [releases page](https://github.com/Motion-Project/motion/releases) for packaged deb files and release notes.  The [issues page](https://github.com/Motion-Project/motion/issues) provides a method to report code bugs while the [discussions page](https://github.com/Motion-Project/motion/discussions) can be used for general questions.

Additionally, there is [Motion User Group List](https://lists.sourceforge.net/lists/listinfo/motion-user) that you can sign up for and submit your question or the [IRC #motion](ircs://irc.libera.chat:6697/motion) on Libera Chat

## License

Motion version 5.0 and later is distributed under the GNU GENERAL PUBLIC LICENSE (GPL) version 3 or later.


