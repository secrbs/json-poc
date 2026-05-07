#include "json.h"
#include <iostream>

int main() {
    // --------------------------------------------------------
    // 1. 오브젝트 직접 구성
    // --------------------------------------------------------
    json::Value config;
    config["app"]     = "json_poc";
    config["version"] = 2;
    config["debug"]   = true;
    config["pi"]      = 3.14159265358979;

    // 중첩 오브젝트
    json::Value server;
    server["host"] = "localhost";
    server["port"] = 8080;
    config["server"] = server;

    // 배열
    json::Value tags = json::Value::Array{"c++", "json", "parser", "file-io"};
    config["tags"] = tags;

    std::cout << "=== 구성한 JSON ===\n" << config.dump(2) << "\n\n";

    // --------------------------------------------------------
    // 2. 파일 저장
    // --------------------------------------------------------
    json::save("output.json", config, 2);
    std::cout << "output.json 저장 완료\n\n";

    // --------------------------------------------------------
    // 3. 문자열에서 파싱
    // --------------------------------------------------------
    std::string raw = R"({
        "name": "Alice",
        "scores": [95, 87, 100],
        "active": true,
        "ratio": 0.75,
        "note": null
    })";

    json::Value parsed = json::Value::parse(raw);
    std::cout << "=== 파싱 결과 ===\n" << parsed.dump(2) << "\n\n";

    // 개별 값 접근
    std::cout << "name   : " << parsed["name"].as_string() << "\n";
    std::cout << "active : " << (parsed["active"].as_bool() ? "true" : "false") << "\n";
    std::cout << "ratio  : " << parsed["ratio"].as_number() << "\n";
    std::cout << "score0 : " << parsed["scores"][0].as_int() << "\n";
    std::cout << "note   : " << (parsed["note"].is_null() ? "null" : "not null") << "\n\n";

    // --------------------------------------------------------
    // 4. 파일에서 로드
    // --------------------------------------------------------
    json::Value loaded = json::load("output.json");
    std::cout << "=== 파일 로드 결과 ===\n";
    std::cout << "app     : " << loaded["app"].as_string() << "\n";
    std::cout << "version : " << loaded["version"].as_int() << "\n";
    std::cout << "host    : " << loaded["server"]["host"].as_string() << "\n";
    std::cout << "port    : " << loaded["server"]["port"].as_int() << "\n";
    std::cout << "tags    : ";
    for (const auto& t : loaded["tags"])
        std::cout << t.as_string() << " ";
    std::cout << "\n\n";

    // --------------------------------------------------------
    // 5. compact 직렬화
    // --------------------------------------------------------
    std::cout << "=== compact 출력 ===\n" << loaded.dump() << "\n";

    return 0;
}
