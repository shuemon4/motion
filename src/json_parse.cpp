/*
 *    This file is part of Motion.
 *
 *    Motion is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Motion is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Motion.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

/*
 * json_parse.cpp - Lightweight JSON Parser
 *
 * This module implements a minimal dependency-free JSON parser for
 * parsing HTTP POST request bodies and configuration data, avoiding
 * external JSON library dependencies.
 *
 */

#include "json_parse.hpp"
#include <cctype>
#include <cstdlib>
#include <sstream>

/* Parse a JSON object string, populating values_. Returns false and sets error_ on failure. */
bool JsonParser::parse(const std::string& json) {
    json_ = json;
    pos_ = 0;
    values_.clear();
    error_.clear();

    skipWhitespace();
    if (!parseObject()) {
        return false;
    }

    skipWhitespace();
    if (pos_ < json_.length()) {
        setError("Unexpected content after JSON object");
        return false;
    }

    return true;
}

/* Return true if the parsed object contains the given key. */
bool JsonParser::has(const std::string& key) const {
    return values_.find(key) != values_.end();
}

/* Return the raw JsonValue variant for the given key. Throws std::out_of_range if missing. */
JsonParser::JsonValue JsonParser::get(const std::string& key) const {
    return values_.at(key);
}

/* Return all key-value pairs from the last successful parse. */
const std::map<std::string, JsonParser::JsonValue>& JsonParser::getAll() const {
    return values_;
}

/* Return the value for key as a string, coercing numbers and bools. Returns def if absent. */
std::string JsonParser::getString(const std::string& key, const std::string& def) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return def;
    }
    if (auto* str = std::get_if<std::string>(&it->second)) {
        return *str;
    }
    if (auto* num = std::get_if<double>(&it->second)) {
        std::ostringstream oss;
        oss << *num;
        return oss.str();
    }
    if (auto* b = std::get_if<bool>(&it->second)) {
        return *b ? "true" : "false";
    }
    return def;
}

/* Return the value for key as a double, coercing string values via strtod. Returns def if absent or not convertible. */
double JsonParser::getNumber(const std::string& key, double def) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return def;
    }
    if (auto* num = std::get_if<double>(&it->second)) {
        return *num;
    }
    if (auto* str = std::get_if<std::string>(&it->second)) {
        char* end;
        double val = std::strtod(str->c_str(), &end);
        if (end != str->c_str() && *end == '\0') {
            return val;
        }
    }
    return def;
}

/* Return the value for key as a bool, coercing strings ("true"/"1") and non-zero numbers. Returns def if absent. */
bool JsonParser::getBool(const std::string& key, bool def) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return def;
    }
    if (auto* b = std::get_if<bool>(&it->second)) {
        return *b;
    }
    if (auto* str = std::get_if<std::string>(&it->second)) {
        return *str == "true" || *str == "1";
    }
    if (auto* num = std::get_if<double>(&it->second)) {
        return *num != 0.0;
    }
    return def;
}

/* Advance pos_ past any whitespace characters. */
void JsonParser::skipWhitespace() {
    while (pos_ < json_.length() && std::isspace(json_[pos_])) {
        pos_++;
    }
}

/* Parse a JSON object ('{' key:value pairs '}'), populating values_ with each entry. */
bool JsonParser::parseObject() {
    if (!expect('{')) {
        return false;
    }

    skipWhitespace();
    if (peek() == '}') {
        pos_++;
        return true; // Empty object
    }

    while (true) {
        if (!parseKeyValue()) {
            return false;
        }

        skipWhitespace();
        char ch = next();
        if (ch == '}') {
            break;
        }
        if (ch != ',') {
            setError("Expected ',' or '}' in object");
            return false;
        }
        skipWhitespace();
    }

    return true;
}

/* Parse a single "key": value pair and store it in values_. */
bool JsonParser::parseKeyValue() {
    skipWhitespace();

    std::string key = parseString();
    if (key.empty() && !error_.empty()) {
        return false;
    }

    skipWhitespace();
    if (!expect(':')) {
        return false;
    }

    skipWhitespace();
    JsonValue value = parseValue();
    if (!error_.empty()) {
        return false;
    }

    values_[key] = value;
    return true;
}

/* Parse a JSON quoted string with backslash escape handling. Returns "" and sets error_ on failure. */
std::string JsonParser::parseString() {
    if (!expect('"')) {
        return "";
    }

    std::string result;
    while (pos_ < json_.length()) {
        char ch = json_[pos_++];

        if (ch == '"') {
            return result;
        }

        if (ch == '\\') {
            if (pos_ >= json_.length()) {
                setError("Unterminated escape sequence");
                return "";
            }
            ch = json_[pos_++];
            switch (ch) {
                case '"':  result += '"'; break;
                case '\\': result += '\\'; break;
                case '/':  result += '/'; break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case 'u': {
                    if (pos_ + 4 > json_.length()) {
                        setError("Incomplete unicode escape");
                        return "";
                    }
                    std::string hex = json_.substr(pos_, 4);
                    pos_ += 4;
                    char *endptr;
                    unsigned long cp = strtoul(hex.c_str(), &endptr, 16);
                    if (endptr != hex.c_str() + 4) {
                        setError("Invalid unicode escape");
                        return "";
                    }
                    /* Handle surrogate pairs */
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (pos_ + 6 <= json_.length() && json_[pos_] == '\\' && json_[pos_+1] == 'u') {
                            pos_ += 2;
                            std::string hex2 = json_.substr(pos_, 4);
                            pos_ += 4;
                            unsigned long cp2 = strtoul(hex2.c_str(), &endptr, 16);
                            if (endptr != hex2.c_str() + 4 || cp2 < 0xDC00 || cp2 > 0xDFFF) {
                                setError("Invalid surrogate pair");
                                return "";
                            }
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (cp2 - 0xDC00);
                        } else {
                            setError("Missing low surrogate");
                            return "";
                        }
                    }
                    /* Encode as UTF-8 */
                    if (cp < 0x80) {
                        result += (char)cp;
                    } else if (cp < 0x800) {
                        result += (char)(0xC0 | (cp >> 6));
                        result += (char)(0x80 | (cp & 0x3F));
                    } else if (cp < 0x10000) {
                        result += (char)(0xE0 | (cp >> 12));
                        result += (char)(0x80 | ((cp >> 6) & 0x3F));
                        result += (char)(0x80 | (cp & 0x3F));
                    } else {
                        result += (char)(0xF0 | (cp >> 18));
                        result += (char)(0x80 | ((cp >> 12) & 0x3F));
                        result += (char)(0x80 | ((cp >> 6) & 0x3F));
                        result += (char)(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default:
                    setError("Invalid escape sequence");
                    return "";
            }
        } else {
            result += ch;
        }
    }

    setError("Unterminated string");
    return "";
}

/* Dispatch to the appropriate parse function based on the leading character of the value. */
JsonParser::JsonValue JsonParser::parseValue() {
    skipWhitespace();

    if (pos_ >= json_.length()) {
        setError("Unexpected end of input");
        return nullptr;
    }

    char ch = peek();

    if (ch == '"') {
        return parseString();
    }

    if (ch == 't' || ch == 'f') {
        return parseBool();
    }

    if (ch == '-' || std::isdigit(ch)) {
        return parseNumber();
    }

    setError("Unexpected character in value");
    return nullptr;
}

/* Parse a JSON number (optional leading '-', integer digits, optional decimal). Returns 0.0 on error. */
double JsonParser::parseNumber() {
    size_t start = pos_;

    if (peek() == '-') {
        pos_++;
    }

    if (pos_ >= json_.length() || !std::isdigit(json_[pos_])) {
        setError("Invalid number format");
        return 0.0;
    }

    while (pos_ < json_.length() && std::isdigit(json_[pos_])) {
        pos_++;
    }

    // Handle decimal point
    if (pos_ < json_.length() && json_[pos_] == '.') {
        pos_++;
        if (pos_ >= json_.length() || !std::isdigit(json_[pos_])) {
            setError("Invalid number format after decimal point");
            return 0.0;
        }
        while (pos_ < json_.length() && std::isdigit(json_[pos_])) {
            pos_++;
        }
    }

    std::string numStr = json_.substr(start, pos_ - start);
    char* end;
    double value = std::strtod(numStr.c_str(), &end);

    if (end == numStr.c_str()) {
        setError("Failed to parse number");
        return 0.0;
    }

    return value;
}

/* Parse a JSON boolean literal ("true" or "false"). Sets error_ and returns false if neither matches. */
bool JsonParser::parseBool() {
    if (pos_ + 4 <= json_.length() && json_.substr(pos_, 4) == "true") {
        pos_ += 4;
        return true;
    }

    if (pos_ + 5 <= json_.length() && json_.substr(pos_, 5) == "false") {
        pos_ += 5;
        return false;
    }

    setError("Invalid boolean value");
    return false;
}

/* Record an error message with the current position; only the first error is retained. */
void JsonParser::setError(const std::string& msg) {
    if (error_.empty()) {  // Only set first error
        error_ = msg + " at position " + std::to_string(pos_);
    }
}

/* Consume the next non-whitespace character, setting error_ and returning false if it doesn't match ch. */
bool JsonParser::expect(char ch) {
    skipWhitespace();
    if (pos_ >= json_.length() || json_[pos_] != ch) {
        setError(std::string("Expected '") + ch + "'");
        return false;
    }
    pos_++;
    return true;
}

/* Return the current character without advancing pos_, or '\0' at end of input. */
char JsonParser::peek() const {
    if (pos_ >= json_.length()) {
        return '\0';
    }
    return json_[pos_];
}

/* Return the current character and advance pos_, or '\0' at end of input. */
char JsonParser::next() {
    if (pos_ >= json_.length()) {
        return '\0';
    }
    return json_[pos_++];
}
