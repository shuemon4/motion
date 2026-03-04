# Code Review Issues — /comment run

Files reviewed: `src/json_parse.cpp`

## Potential Issues

### \uXXXX Unicode escape sequences not handled
- **File**: `src/json_parse.cpp`
- **Line**: ~215 (default case in `parseString()` escape switch)
- **Severity**: medium
- **Description**: The JSON spec requires parsers to accept `\uXXXX` four-hex-digit Unicode escapes inside strings. The `parseString()` switch handles `\"`, `\\`, `\/`, `\b`, `\f`, `\n`, `\r`, `\t` but falls through to `setError("Invalid escape sequence")` for any other escape character. Any JSON sent by a browser or other compliant producer that contains `\uXXXX` (e.g., `\u0022` for a quote) will fail to parse and produce an error. If the parser is only expected to handle ASCII config fields this may be acceptable, but it is a silent incompatibility with RFC 8259.
- **Code snippet**:
  ```cpp
  case 't':  result += '\t'; break;
  default:
      setError("Invalid escape sequence");   // rejects \uXXXX
      return "";
  ```

### parseBool() returns false on both "false" and parse error — callers cannot distinguish
- **File**: `src/json_parse.cpp`
- **Line**: ~305 (`parseBool()`)
- **Severity**: low
- **Description**: `parseBool()` returns `false` for both a legitimately parsed `"false"` literal and for a parse failure (with `setError()`). Callers that don't separately check `error_` cannot distinguish the two cases. This is unlikely to cause a real bug since `parseValue()` only calls `parseBool()` for tokens starting with 't' or 'f', but the ambiguity makes the function's contract unclear.
- **Code snippet**:
  ```cpp
  setError("Invalid boolean value");
  return false;   // same return as a valid "false"
  ```

### null, arrays, and nested objects silently rejected
- **File**: `src/json_parse.cpp`
- **Line**: ~248 (`parseValue()`)
- **Severity**: low
- **Description**: `parseValue()` only handles strings, booleans, and numbers. JSON `null` literals, arrays (`[...]`), and nested objects (`{...}`) trigger "Unexpected character in value". This is consistent with the file's stated purpose as a minimal parser, but there is no documented contract that callers are responsible for sending only flat objects. If the API surface grows and clients send arrays or null values the parser will silently reject them.
- **Code snippet**:
  ```cpp
  setError("Unexpected character in value");
  return nullptr;   // triggered by 'n' (null), '[', or nested '{'
  ```

### Duplicate key silently overwrites earlier value
- **File**: `src/json_parse.cpp`
- **Line**: ~184 (`parseKeyValue()`)
- **Severity**: low
- **Description**: `values_[key] = value` silently overwrites any previous value if the same key appears more than once in the JSON object. RFC 8259 says behavior for duplicate keys is undefined, so this is not a spec violation, but it means a malformed or adversarially crafted payload could shadow a parameter by repeating its key. No error or warning is produced.
- **Code snippet**:
  ```cpp
  values_[key] = value;   // last writer wins; no duplicate detection
  ```
