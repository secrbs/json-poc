#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iosfwd>

namespace json {

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg, size_t pos)
        : std::runtime_error(msg + " (pos " + std::to_string(pos) + ")"), pos_(pos) {}
    size_t pos() const { return pos_; }
private:
    size_t pos_;
};

class TypeError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Value {
public:
    using Array  = std::vector<Value>;
    using Object = std::map<std::string, Value>;

    enum class Type { Null, Bool, Number, String, Array, Object };

    // --- 생성자 ---
    Value()                      : type_(Type::Null) {}
    Value(std::nullptr_t)        : type_(Type::Null) {}
    Value(bool v)                : type_(Type::Bool),   bool_(v) {}
    Value(int v)                 : type_(Type::Number), number_(static_cast<double>(v)) {}
    Value(long long v)           : type_(Type::Number), number_(static_cast<double>(v)) {}
    Value(double v)              : type_(Type::Number), number_(v) {}
    Value(const char* v)         : type_(Type::String), string_(v) {}
    Value(std::string v)         : type_(Type::String), string_(std::move(v)) {}
    Value(Array v)               : type_(Type::Array),  array_(std::move(v)) {}
    Value(Object v)              : type_(Type::Object), object_(std::move(v)) {}

    // --- 타입 확인 ---
    Type type()      const { return type_; }
    bool is_null()   const { return type_ == Type::Null; }
    bool is_bool()   const { return type_ == Type::Bool; }
    bool is_number() const { return type_ == Type::Number; }
    bool is_string() const { return type_ == Type::String; }
    bool is_array()  const { return type_ == Type::Array; }
    bool is_object() const { return type_ == Type::Object; }

    // --- 값 접근 (타입 불일치 시 TypeError 발생) ---
    bool               as_bool()   const;
    double             as_number() const;
    int                as_int()    const;
    const std::string& as_string() const;
    const Array&       as_array()  const;
    const Object&      as_object() const;

    // --- 배열 연산 ---
    void          push_back(Value v);
    Value&        operator[](size_t idx);
    const Value&  operator[](size_t idx) const;
    Array::iterator       begin();
    Array::iterator       end();
    Array::const_iterator begin() const;
    Array::const_iterator end()   const;

    // --- 오브젝트 연산 ---
    Value&       operator[](const std::string& key);
    const Value& operator[](const std::string& key) const;
    bool         has_key(const std::string& key) const;
    std::vector<std::string> keys() const;

    size_t size() const;

    // --- 직렬화 ---
    // indent < 0 이면 compact, >= 0 이면 pretty-print (spaces per level)
    std::string dump(int indent = -1) const;

    // --- 파싱 ---
    static Value parse(const std::string& s);
    static Value parse(std::istream& is);

private:
    Type        type_;
    bool        bool_   = false;
    double      number_ = 0.0;
    std::string string_;
    Array       array_;
    Object      object_;

    void dump_impl(std::string& out, int indent, int depth) const;
    static void escape_string(std::string& out, const std::string& s);
};

// --- 파일 I/O ---
Value load(const std::string& path);
void  save(const std::string& path, const Value& v, int indent = 2);

} // namespace json
