# Code Review Issues — /comment run

Files reviewed: `src/motion.cpp`

## Potential Issues

### 1. Double mutex lock instead of lock/unlock in check_restart()
- **File**: `src/motion.cpp`
- **Line**: ~466 (database restart block)
- **Severity**: high
- **Description**: The database restart block locks `dbse->mutex_dbse` twice instead of locking then unlocking. The second call should be `pthread_mutex_unlock`. This will deadlock the calling thread on the second lock attempt (POSIX default mutexes are not recursive).
- **Code snippet**:
  ```cpp
  if (dbse->restart == true) {
      MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO, _("Restarting database"));
      pthread_mutex_lock(&dbse->mutex_dbse);
          dbse->shutdown();
          cfg->parms_copy(conf_src, PARM_CAT_15);
          dbse->startup();
      pthread_mutex_lock(&dbse->mutex_dbse);   // <-- should be pthread_mutex_unlock
      dbse->restart = false;
      MOTION_LOG(NTC, TYPE_ALL, NO_ERRNO, _("Restarted database"));
  }
  ```
