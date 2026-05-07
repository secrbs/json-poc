#include "json.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace json {

// ============================================================
// Value: 타입 접근자
// ============================================================

bool Value::as_bool() const {
    if (type_ != Type::Bool) throw TypeError("value is not a bool");
    return bool_;
}

double Value::as_number() const {
    if (type_ != Type::Number) throw TypeError("value is not a number");
    return number_;
}

int Value::as_int() const {
    return static_cast<int>(as_number());
}

const std::string& Value::as_string() const {
    if (type_ != Type::String) throw TypeError("value is not a string");
    return string_;
}

const Value::Array& Value::as_array() const {
    if (type_ != Type::Array) throw TypeError("value is not an array");
    return array_;
}

const Value::Object& Value::as_object() const {
    if (type_ != Type::Object) throw TypeError("value is not an object");
    return object_;
}

// ============================================================
// Value: 배열 연산
// ============================================================

void Value::push_back(Value v) {
    if (type_ == Type::Null) { type_ = Type::Array; }
    if (type_ != Type::Array) throw TypeError("value is not an array");
    array_.push_back(std::move(v));
}

Value& Value::operator[](size_t idx) {
    if (type_ != Type::Array) throw TypeError("value is not an array");
    if (idx >= array_.size()) throw std::out_of_range("array index out of range");
    return array_[idx];
}

const Value& Value::operator[](size_t idx) const {
    if (type_ != Type::Array) throw TypeError("value is not an array");
    if (idx >= array_.size()) throw std::out_of_range("array index out of range");
    return array_[idx];
}

Value::Array::iterator Value::begin() {
    if (type_ != Type::Array) throw TypeError("value is not an array");
    return array_.begin();
}

Value::Array::iterator Value::end() {
    if (type_ != Type::Array) throw TypeError("value is not an array");
    return array_.end();
}

Value::Array::const_iterator Value::begin() const {
    if (type_ != Type::Array) throw TypeError("value is not an array");
    return array_.begin();
}

Value::Array::const_iterator Value::end() const {
    if (type_ != Type::Array) throw TypeError("value is not an array");
    return array_.end();
}

// ============================================================
// Value: 오브젝트 연산
// ============================================================

Value& Value::operator[](const std::string& key) {
    if (type_ == Type::Null) { type_ = Type::Object; }
    if (type_ != Type::Object) throw TypeError("value is not an object");
    return object_[key];
}

const Value& Value::operator[](const std::string& key) const {
    if (type_ != Type::Object) throw TypeError("value is not an object");
    auto it = object_.find(key);
    if (it == object_.end()) throw std::out_of_range("key not found: " + key);
    return it->second;
}

bool Value::has_key(const std::string& key) const {
    if (type_ != Type::Object) return false;
    return object_.find(key) != object_.end();
}

std::vector<std::string> Value::keys() const {
    if (type_ != Type::Object) throw TypeError("value is not an object");
    std::vector<std::string> result;
    result.reserve(object_.size());
    for (const auto& [k, v] : object_)
        result.push_back(k);
    return result;
}

size_t Value::size() const {
    if (type_ == Type::Array)  return array_.size();
    if (type_ == Type::Object) return object_.size();
    if (type_ == Type::String) return string_.size();
    throw TypeError("size() not supported for this type");
}

// ============================================================
// Value: 직렬화 (dump)
// ============================================================

void Value::escape_string(std::string& out, const std::string& s) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    out += '"';
}

void Value::dump_impl(std::string& out, int indent, int depth) const {
    auto newline_indent = [&](int d) {
        if (indent >= 0) {
            out += '\n';
            out.append(static_cast<size_t>(indent * d), ' ');
        }
    };

    switch (type_) {
        case Type::Null:
            out += "null";
            break;

        case Type::Bool:
            out += bool_ ? "true" : "false";
            break;

        case Type::Number: {
            if (std::isnan(number_) || std::isinf(number_)) {
                out += "null"; // JSON 스펙: NaN/Infinity 불허
            } else if (number_ == static_cast<long long>(number_) &&
                       std::abs(number_) < 1e15) {
                out += std::to_string(static_cast<long long>(number_));
            } else {
                char buf[32];
                snprintf(buf, sizeof(buf), "%.15g", number_);
                out += buf;
            }
            break;
        }

        case Type::String:
            escape_string(out, string_);
            break;

        case Type::Array: {
            out += '[';
            for (size_t i = 0; i < array_.size(); ++i) {
                if (i == 0) newline_indent(depth + 1);
                else        { out += ','; newline_indent(depth + 1); }
                array_[i].dump_impl(out, indent, depth + 1);
            }
            if (!array_.empty()) newline_indent(depth);
            out += ']';
            break;
        }

        case Type::Object: {
            out += '{';
            bool first = true;
            for (const auto& [k, v] : object_) {
                if (first) { newline_indent(depth + 1); first = false; }
                else       { out += ','; newline_indent(depth + 1); }
                escape_string(out, k);
                out += ':';
                if (indent >= 0) out += ' ';
                v.dump_impl(out, indent, depth + 1);
            }
            if (!object_.empty()) newline_indent(depth);
            out += '}';
            break;
        }
    }
}

std::string Value::dump(int indent) const {
    std::string out;
    dump_impl(out, indent, 0);
    return out;
}

// ============================================================
// 파서 (내부 구현)
// ============================================================

namespace {

class Parser {
public:
    explicit Parser(const std::string& s) : s_(s), pos_(0) {}

    Value parse() {
        skip_ws();
        Value v = parse_value();
        skip_ws();
        if (pos_ != s_.size())
            throw ParseError("unexpected trailing characters", pos_);
        return v;
    }

private:
    const std::string& s_;
    size_t             pos_;

    void skip_ws() {
        while (pos_ < s_.size() && (s_[pos_] == ' '  || s_[pos_] == '\t' ||
                                     s_[pos_] == '\n' || s_[pos_] == '\r'))
            ++pos_;
    }

    char peek() const {
        if (pos_ >= s_.size())
            throw ParseError("unexpected end of input", pos_);
        return s_[pos_];
    }

    char consume() {
        if (pos_ >= s_.size())
            throw ParseError("unexpected end of input", pos_);
        return s_[pos_++];
    }

    void expect(char c) {
        char got = consume();
        if (got != c)
            throw ParseError(std::string("expected '") + c + "', got '" + got + "'", pos_ - 1);
    }

    // ---- 리터럴 ----

    Value parse_null() {
        if (s_.substr(pos_, 4) != "null")
            throw ParseError("invalid literal (expected 'null')", pos_);
        pos_ += 4;
        return Value{};
    }

    Value parse_bool() {
        if (s_.size() >= pos_ + 4 && s_.substr(pos_, 4) == "true")  { pos_ += 4; return Value{true};  }
        if (s_.size() >= pos_ + 5 && s_.substr(pos_, 5) == "false") { pos_ += 5; return Value{false}; }
        throw ParseError("invalid literal (expected 'true' or 'false')", pos_);
    }

    Value parse_number() {
        size_t start = pos_;
        if (s_[pos_] == '-') ++pos_;
        if (pos_ >= s_.size() || s_[pos_] < '0' || s_[pos_] > '9')
            throw ParseError("invalid number", pos_);
        while (pos_ < s_.size() && s_[pos_] >= '0' && s_[pos_] <= '9') ++pos_;
        if (pos_ < s_.size() && s_[pos_] == '.') {
            ++pos_;
            while (pos_ < s_.size() && s_[pos_] >= '0' && s_[pos_] <= '9') ++pos_;
        }
        if (pos_ < s_.size() && (s_[pos_] == 'e' || s_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < s_.size() && (s_[pos_] == '+' || s_[pos_] == '-')) ++pos_;
            while (pos_ < s_.size() && s_[pos_] >= '0' && s_[pos_] <= '9') ++pos_;
        }
        return Value{std::stod(s_.substr(start, pos_ - start))};
    }

    // ---- 문자열 ----

    std::string parse_string_raw() {
        expect('"');
        std::string result;
        while (true) {
            if (pos_ >= s_.size())
                throw ParseError("unterminated string", pos_);
            char c = s_[pos_++];
            if (c == '"') break;
            if (c != '\\') { result += c; continue; }

            char esc = consume();
            switch (esc) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case 'u': {
                    if (pos_ + 4 > s_.size())
                        throw ParseError("invalid unicode escape", pos_);
                    unsigned int cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        char h = s_[pos_++];
                        cp <<= 4;
                        if (h >= '0' && h <= '9') cp |= (h - '0');
                        else if (h >= 'a' && h <= 'f') cp |= (h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') cp |= (h - 'A' + 10);
                        else throw ParseError("invalid hex digit in \\u escape", pos_ - 1);
                    }
                    // UTF-8 인코딩
                    if (cp < 0x80) {
                        result += static_cast<char>(cp);
                    } else if (cp < 0x800) {
                        result += static_cast<char>(0xC0 | (cp >> 6));
                        result += static_cast<char>(0x80 | (cp & 0x3F));
                    } else {
                        result += static_cast<char>(0xE0 | (cp >> 12));
                        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default:
                    throw ParseError(std::string("invalid escape '\\") + esc + "'", pos_ - 1);
            }
        }
        return result;
    }

    // ---- 배열 ----

    Value parse_array() {
        expect('[');
        Value::Array arr;
        skip_ws();
        if (peek() == ']') { ++pos_; return Value{std::move(arr)}; }
        while (true) {
            arr.push_back(parse_value());
            skip_ws();
            char c = peek();
            if (c == ']') { ++pos_; break; }
            if (c != ',') throw ParseError("expected ',' or ']' in array", pos_);
            ++pos_;
        }
        return Value{std::move(arr)};
    }

    // ---- 오브젝트 ----

    Value parse_object() {
        expect('{');
        Value::Object obj;
        skip_ws();
        if (peek() == '}') { ++pos_; return Value{std::move(obj)}; }
        while (true) {
            skip_ws();
            if (peek() != '"')
                throw ParseError("expected string key in object", pos_);
            std::string key = parse_string_raw();
            skip_ws();
            expect(':');
            obj[std::move(key)] = parse_value();
            skip_ws();
            char c = peek();
            if (c == '}') { ++pos_; break; }
            if (c != ',') throw ParseError("expected ',' or '}' in object", pos_);
            ++pos_;
        }
        return Value{std::move(obj)};
    }

    // ---- 진입점 ----

    Value parse_value() {
        skip_ws();
        char c = peek();
        if (c == '"') return Value{parse_string_raw()};
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == 'n') return parse_null();
        if (c == '-' || (c >= '0' && c <= '9')) return parse_number();
        throw ParseError(std::string("unexpected character '") + c + "'", pos_);
    }
};

} // anonymous namespace

// ============================================================
// Value: 파싱 정적 메서드
// ============================================================

Value Value::parse(const std::string& s) {
    return Parser(s).parse();
}

Value Value::parse(std::istream& is) {
    std::string content((std::istreambuf_iterator<char>(is)),
                         std::istreambuf_iterator<char>());
    return Parser(content).parse();
}

// ============================================================
// 파일 I/O
// ============================================================

Value load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open())
        throw std::runtime_error("cannot open file for reading: " + path);
    return Value::parse(f);
}

void save(const std::string& path, const Value& v, int indent) {
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open())
        throw std::runtime_error("cannot open file for writing: " + path);
    f << v.dump(indent);
    if (!f)
        throw std::runtime_error("write error: " + path);
}

} // namespace json
