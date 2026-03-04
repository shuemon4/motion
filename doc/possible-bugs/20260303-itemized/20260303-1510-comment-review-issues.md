# Code Review Issues — /comment run

Files reviewed: `src/webu_ans.cpp`

## Potential Issues

### is_trusted_proxy() claims CIDR support but only does exact IP match
- **File**: `src/webu_ans.cpp`
- **Line**: ~271 (function comment) and ~284 (implementation)
- **Severity**: medium
- **Description**: The function's doc comment says "Supports comma-separated list of IPs or CIDR ranges", but the implementation only compares strings with `==`. CIDR notation like `192.168.1.0/24` would never match any real IP. A misconfigured `trusted_proxies` entry using CIDR notation would silently fail to trust the proxy, and the comment creates a false expectation of CIDR support.
- **Code snippet**:
  ```cpp
  /**
   * Check if an IP address is in the trusted proxies list
   * Supports comma-separated list of IPs or CIDR ranges   // ← claims CIDR support
   */
  static bool is_trusted_proxy(const std::string &ip, const std::string &trusted_list)
  {
      ...
      if (trusted == ip) {   // ← exact string match only, no CIDR parsing
          return true;
      }
  }
  ```

### clients_mtx held while calling util_exec_command() in failauth_check()
- **File**: `src/webu_ans.cpp`
- **Line**: ~554 (lock acquired) and ~570 (exec call)
- **Severity**: medium
- **Description**: `failauth_check()` holds `webu->clients_mtx` for the entire function, including the call to `util_exec_command()` which spawns an external lock script. If the script is slow (firewall rule insertion, network lookup), all threads calling `client_connect()` or `failauth_log()` block for the duration. This could cause visible connection delays for legitimate clients during a brute-force attack — exactly when responsiveness matters.
- **Code snippet**:
  ```cpp
  std::lock_guard<std::mutex> lock(webu->clients_mtx);  // held for full function
  ...
  if (app->cfg->webcontrol_lock_script != "") {
      tmp = app->cfg->webcontrol_lock_script + " " + ...;
      util_exec_command(cam, tmp.c_str(), NULL);  // external process while mutex held
  }
  return MHD_NO;
  ```
