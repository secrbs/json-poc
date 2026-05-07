# json library

외부 의존성 없이 C++17 표준 라이브러리만으로 구현한 JSON 파싱·직렬화·파일 I/O 라이브러리입니다.

## 파일 구성

```
json_poc/
├── json.h      // 공개 API 헤더
├── json.cpp    // 파서 / 직렬화 / 파일 I/O 구현
└── main.cpp    // 사용 예제
```

## 빌드 요구사항

| 항목 | 버전 |
|------|------|
| MSVC | v143 (Visual Studio 2022) |
| C++ 표준 | C++17 |
| 컴파일러 플래그 | `/utf-8` (소스 파일 UTF-8 인식) |

Visual Studio에서 `json_poc.sln`을 열거나 MSBuild로 빌드합니다.

```
MSBuild json_poc.sln /p:Configuration=Debug /p:Platform=x64
```

---

## API 레퍼런스

### 헤더 포함

```cpp
#include "json.h"
```

모든 심볼은 `json` 네임스페이스 안에 있습니다.

---

### json::Value — 값 타입

JSON의 6가지 타입을 하나의 클래스로 표현합니다.

| JSON 타입 | C++ 생성 방법 | `Type` 열거값 |
|-----------|--------------|--------------|
| null      | `Value{}`    | `Type::Null`   |
| boolean   | `Value{true}` | `Type::Bool`  |
| number    | `Value{42}`, `Value{3.14}` | `Type::Number` |
| string    | `Value{"hello"}` | `Type::String` |
| array     | `Value{Value::Array{...}}` | `Type::Array` |
| object    | `Value{}` 후 `[]` 대입 | `Type::Object` |

---

### 값 생성

```cpp
// 기본 타입
json::Value null_val;                  // null
json::Value bool_val  = true;
json::Value int_val   = 42;
json::Value float_val = 3.14159;
json::Value str_val   = "hello";

// 배열 — 초기화 리스트
json::Value arr = json::Value::Array{1, 2, 3};
json::Value mixed = json::Value::Array{"a", 1, true, nullptr};

// 오브젝트 — 빈 Value에 키 대입 (map처럼 동작)
json::Value obj;
obj["name"]    = "Alice";
obj["age"]     = 30;
obj["active"]  = true;
```

---

### 타입 확인

```cpp
json::Value v = json::Value::parse(R"({"x": 1})");

v.is_null()   // false
v.is_object() // true
v.is_array()  // false
v.is_number() // false

v.type() == json::Value::Type::Object  // true
```

---

### 값 읽기

타입이 일치하지 않으면 `json::TypeError` 예외가 발생합니다.

```cpp
json::Value v = json::Value::parse(R"({
    "name":   "Bob",
    "score":  95,
    "passed": true,
    "ratio":  0.95,
    "note":   null
})");

std::string name  = v["name"].as_string();   // "Bob"
int         score = v["score"].as_int();      // 95
bool        pass  = v["passed"].as_bool();    // true
double      ratio = v["ratio"].as_number();   // 0.95
bool        isnul = v["note"].is_null();      // true
```

---

### 배열 조작

```cpp
json::Value arr = json::Value::Array{10, 20, 30};

// 인덱스 접근
int first = arr[0].as_int();   // 10

// 원소 추가
arr.push_back(40);
arr.push_back("extra");

// 크기
size_t n = arr.size();   // 5

// range-for
for (const auto& item : arr) {
    std::cout << item.dump() << "\n";
}
```

---

### 오브젝트 조작

```cpp
json::Value obj;
obj["host"] = "localhost";
obj["port"] = 8080;

// 키 존재 확인
if (obj.has_key("port")) {
    int port = obj["port"].as_int();  // 8080
}

// 모든 키 열거
for (const std::string& k : obj.keys()) {
    std::cout << k << ": " << obj[k].dump() << "\n";
}

// 키 개수
size_t n = obj.size();
```

---

### 중첩 구조

```cpp
json::Value config;
config["app"]     = "myapp";
config["version"] = 1;

json::Value db;
db["host"] = "db.local";
db["port"] = 5432;
config["database"] = db;

config["tags"] = json::Value::Array{"backend", "service"};

// 중첩 접근
std::string host = config["database"]["host"].as_string();  // "db.local"
std::string tag0 = config["tags"][0].as_string();           // "backend"
```

---

### 파싱

```cpp
// 문자열에서 파싱
std::string raw = R"({"key": "value", "nums": [1, 2, 3]})";
json::Value v = json::Value::parse(raw);

// istream에서 파싱
std::istringstream ss(raw);
json::Value v2 = json::Value::parse(ss);

// 파싱 오류 처리
try {
    json::Value bad = json::Value::parse("{invalid}");
} catch (const json::ParseError& e) {
    std::cerr << "파싱 실패: " << e.what() << "\n";
    // e.pos() 로 오류 위치(바이트 오프셋) 확인 가능
}
```

**지원 문법**

| 항목 | 예시 |
|------|------|
| 정수 / 실수 | `42`, `-7`, `3.14`, `1e10`, `-2.5E-3` |
| 문자열 이스케이프 | `\"`, `\\`, `\/`, `\b`, `\f`, `\n`, `\r`, `\t`, `\uXXXX` |
| 중첩 배열·오브젝트 | 깊이 제한 없음 |
| 공백 | 토큰 사이 임의 공백 허용 |

---

### 직렬화 (dump)

```cpp
json::Value v;
v["x"] = 1;
v["y"] = json::Value::Array{2, 3};

// compact (기본값: indent = -1)
std::string compact = v.dump();
// {"x":1,"y":[2,3]}

// pretty-print (들여쓰기 2칸)
std::string pretty = v.dump(2);
// {
//   "x": 1,
//   "y": [
//     2,
//     3
//   ]
// }

// 들여쓰기 4칸
std::string pretty4 = v.dump(4);
```

**직렬화 규칙**

- `NaN` / `Infinity` → `null` (JSON 스펙 준수)
- 정수로 표현 가능한 실수 → 소수점 없이 출력 (`2.0` → `2`)
- 오브젝트 키는 알파벳 순 정렬 (`std::map` 기반)

---

### 파일 I/O

```cpp
#include "json.h"

// 저장 (기본 들여쓰기 2칸)
json::Value cfg;
cfg["debug"] = false;
cfg["level"] = 3;

json::save("config.json", cfg);         // indent = 2 (기본값)
json::save("config.min.json", cfg, -1); // compact

// 로드
try {
    json::Value loaded = json::load("config.json");
    int level = loaded["level"].as_int();  // 3
} catch (const std::runtime_error& e) {
    std::cerr << "파일 오류: " << e.what() << "\n";
}
```

---

### 예외 처리 요약

| 예외 클래스 | 발생 조건 |
|-------------|-----------|
| `json::ParseError` | JSON 문법 오류. `e.pos()`로 바이트 오프셋 확인 |
| `json::TypeError`  | `as_string()` 등 타입 불일치 접근 |
| `std::out_of_range` | 존재하지 않는 배열 인덱스 또는 오브젝트 키 (const 접근 시) |
| `std::runtime_error` | 파일 열기/쓰기 실패 |

```cpp
try {
    json::Value v = json::load("data.json");
    std::cout << v["title"].as_string() << "\n";
}
catch (const json::ParseError& e)    { /* JSON 문법 오류 */ }
catch (const json::TypeError& e)     { /* 타입 불일치 */    }
catch (const std::out_of_range& e)   { /* 키 없음 */        }
catch (const std::runtime_error& e)  { /* 파일 오류 */      }
```

---

## 전체 예제

```cpp
#include "json.h"
#include <iostream>

int main() {
    // 1. 오브젝트 구성
    json::Value user;
    user["id"]    = 1001;
    user["name"]  = "Alice";
    user["admin"] = false;
    user["score"] = 98.5;
    user["tags"]  = json::Value::Array{"c++", "backend"};

    json::Value addr;
    addr["city"]  = "Seoul";
    addr["zip"]   = "04524";
    user["addr"]  = addr;

    // 2. 파일 저장
    json::save("user.json", user, 2);

    // 3. 파일 로드 후 읽기
    json::Value loaded = json::load("user.json");

    std::cout << "name : " << loaded["name"].as_string()       << "\n";
    std::cout << "score: " << loaded["score"].as_number()      << "\n";
    std::cout << "city : " << loaded["addr"]["city"].as_string() << "\n";

    std::cout << "tags : ";
    for (const auto& t : loaded["tags"])
        std::cout << t.as_string() << " ";
    std::cout << "\n";

    // 4. compact 출력
    std::cout << "\n" << loaded.dump() << "\n";

    return 0;
}
```

**출력 예시**

```
name : Alice
score: 98.5
city : Seoul
tags : c++ backend 

{"addr":{"city":"Seoul","zip":"04524"},"admin":false,"id":1001,"name":"Alice","score":98.5,"tags":["c++","backend"]}
```
