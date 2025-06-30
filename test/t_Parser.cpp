#include <cassert>
#include <iostream>
#include <sstream>
#define private   public
#define protected public
#include "Parser.hpp"

// 方便重複驗證字串 → int 轉換
void test_toInt() {
    Parser p;
    assert(p.toInt("0") == 0);
    assert(p.toInt("1") == 1);
    assert(p.toInt("-") == -1);
    bool threw = false;
    try { p.toInt("x"); }          // 非法輸入應該丟例外:contentReference[oaicite:0]{index=0}
    catch (const std::runtime_error&) { threw = true; }
    assert(threw);
}

// 驗證 R0W1CI0CO1 拆解成 4 個 SingleOp
void test_explodeOpToken() {
    Parser p;
    auto v = p.explodeOpToken("R0W1Ci0Co1");      // ⬅︎ 大小寫交錯也要吃得下:contentReference[oaicite:1]{index=1}
    assert(v.size() == 4);
    assert(v[0].type_ == OpType::R && v[0].value_ == 0);
    assert(v[2].type_ == OpType::CI && v[2].value_ == 0);
}

// 讀 fault.json：檢查第一條 SAF
void test_parseFaults() {
    Parser p;
    auto list = p.parseFaults("fault.json");
    assert(!list.empty());
    const FaultConfig& f = list.front();
    assert(f.id_.faultName_ == "Stuck-at Fault (SAF)");
    assert(!f.is_twoCell_);
    assert(f.VI_ == 0);
    assert(f.trigger_.size() == 1);           // W1
    assert(f.faultValue_ == 0);
    assert(f.finalReadValue_ == -1);
}

// 為 parseMarchTest 建一個 cin mock
std::vector<MarchElement> parse_march_with_choice(std::size_t idx) {
    Parser p;
    std::istringstream fakeIn(std::to_string(idx) + "\n");
    std::streambuf* orig = std::cin.rdbuf(fakeIn.rdbuf()); // ↱ 取代 cin
    auto elems = p.parseMarchTest("marchTest.json");       // 內部會讀 cin:contentReference[oaicite:2]{index=2}
    std::cin.rdbuf(orig);                                  // ↳ 還原
    return elems;
}

void test_parseMarchTest_ok() {
    auto elems = parse_march_with_choice(1);   // 選 「MATS++」 (第 1 筆)
    assert(elems.size() == 3);                 // 3 個 MarchElement
    assert(elems[0].addrOrder_ == Direction::BOTH);
    assert(elems[0].ops_.size() == 1);         // (w0)
}

void test_parseMarchTest_invalid_index() {
    bool threw = false;
    Parser p;
    std::istringstream wrong("999\n");
    std::streambuf* orig = std::cin.rdbuf(wrong.rdbuf());
    try { p.parseMarchTest("marchTest.json"); }            // 應該丟 "Invalid selection":contentReference[oaicite:3]{index=3}
    catch (const std::runtime_error&) { threw = true; }
    std::cin.rdbuf(orig);
    assert(threw);
}

int main() {
    test_toInt();
    test_explodeOpToken();
    test_parseFaults();
    test_parseMarchTest_ok();
    test_parseMarchTest_invalid_index();
    std::cout << "✅  All cassert tests passed!\n";
}
