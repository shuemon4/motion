# Code Review Issues — /comment run

Files reviewed: `src/dbse.cpp`

## Potential Issues

### 1. CLOCK_MONOTONIC passed to localtime_r in handler()
- **File**: `src/dbse.cpp`
- **Line**: ~1399-1401
- **Severity**: low
- **Description**: `handler()` uses `CLOCK_MONOTONIC` to get the current time, then passes `ts2.tv_sec` to `localtime_r()`. `CLOCK_MONOTONIC` returns seconds since an arbitrary epoch (typically system boot), while `localtime_r` expects seconds since the Unix epoch (Jan 1, 1970). The resulting `tm_hour` will not represent the actual wall-clock hour. The cleanup still runs roughly once per hour since the monotonic clock advances at real-time rate, but the hourly boundary won't align with real clock hours. Should use `CLOCK_REALTIME` or `time()` instead.
- **Code snippet**:
  ```cpp
  clock_gettime(CLOCK_MONOTONIC, &ts2);
  localtime_r(&ts2.tv_sec, &lcl_tm);
  hr_cur = lcl_tm.tm_hour;
  ```

### 2. PostgreSQL connection string values not escaped
- **File**: `src/dbse.cpp`
- **Line**: ~1015-1019
- **Severity**: medium
- **Description**: `pgsqldb_init()` builds a libpq connection string by concatenating config values inside single quotes, but does not escape single quotes within those values. If `database_password`, `database_dbname`, `database_host`, or `database_user` contains a single quote, the connection string will be malformed and the connection will fail. Should use `PQconnectdbParams()` or escape quotes in the values.
- **Code snippet**:
  ```cpp
  constr = "dbname='" + app->cfg->database_dbname + "' ";
  constr += " host='" + app->cfg->database_host + "' ";
  constr += " user='" + app->cfg->database_user + "' ";
  constr += " password='" + app->cfg->database_password + "' ";
  ```

### 3. mysql_init return value not checked
- **File**: `src/dbse.cpp`
- **Line**: ~744-745
- **Severity**: low
- **Description**: In `mariadb_init()`, `mymalloc` allocates a `MYSQL` struct, then `mysql_init()` is called on it. `mysql_init()` can return `NULL` on allocation failure, but the return value is not checked. If it fails, subsequent `mysql_real_connect()` would use an uninitialized structure.
- **Code snippet**:
  ```cpp
  database_mariadb = (MYSQL *) mymalloc(sizeof(MYSQL));
  mysql_init(database_mariadb);
  ```
