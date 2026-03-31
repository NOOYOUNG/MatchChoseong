#include <iostream>
#include "../include/matchChoseong.hpp"

int main() {

    // =========================================================================
    // 1. Pure chosung search
    // =========================================================================
    std::cout << "=== Pure Chosung Search ===\n";

    matchChoseong::Index idx;
    idx.add({
        "한글", "학교", "하늘", "한국",
        "대한민국", "대학교", "대화",
        "사랑", "사람", "사과",
        "서울", "선생님",
        "컴퓨터", "키보드"
    });

    // "ㅎㄱ" → words whose choseong sequence starts with ㅎ, ㄱ
    for (const auto& w : idx.search("ㅎㄱ"))
        std::cout << "  " << w << "\n";
    // 한글, 학교

    // =========================================================================
    // 2. Mixed search (syllable + jamo)
    // =========================================================================
    std::cout << "\n=== Mixed Search ===\n";

    // "학ㄱ" → "학" must match exactly, next char's choseong must be ㄱ
    for (const auto& w : idx.search("학ㄱ"))
        std::cout << "  " << w << "\n";
    // 학교

    // =========================================================================
    // 3. Normal search (exact match)
    // =========================================================================
    std::cout << "\n=== Normal Search ===\n";

    for (const auto& w : idx.search("대학"))
        std::cout << "  " << w << "\n";
    // 대학교

    // =========================================================================
    // 4. Long chosung sequence
    // =========================================================================
    std::cout << "\n=== Long Chosung Sequence ===\n";

    for (const auto& w : idx.search("ㄷㅎㅁㄱ"))
        std::cout << "  " << w << "\n";
    // 대한민국

    // =========================================================================
    // 5. search() — returns position and length of first match
    // =========================================================================
    std::cout << "\n=== search() — First Match Position ===\n";

    // Useful when you need to know WHERE in the string the match occurred
    auto r = matchChoseong::search("한글 학교 하늘", "ㅎㄱ");
    if (r.matched) {
        std::cout << "  matched at pos=" << r.pos
                  << " len=" << r.len << "\n";
    }
    // matched at pos=0 len=2  ("한글")

    // =========================================================================
    // 6. search_all() — returns all match positions
    // =========================================================================
    std::cout << "\n=== search_all() — All Match Positions ===\n";

    // Useful for highlighting every occurrence in a string
    auto all = matchChoseong::search_all("한글 학교 하늘 한국", "ㅎ");
    for (const auto& m : all)
        std::cout << "  pos=" << m.pos << "\n";
    // pos=0 (한), pos=3 (학), pos=6 (하), pos=9 (한)

    // =========================================================================
    // 7. contains() — simple boolean check
    // =========================================================================
    std::cout << "\n=== contains() ===\n";

    std::cout << "  ㅎ in 안녕하세요 : " << matchChoseong::contains("안녕하세요", "ㅎ") << "\n"; // 1
    std::cout << "  ㄱ in 안녕하세요 : " << matchChoseong::contains("안녕하세요", "ㄱ") << "\n"; // 0

    // =========================================================================
    // 8. has_match() — check if any word in the index matches
    // =========================================================================
    std::cout << "\n=== has_match() ===\n";

    std::cout << "  any ㅅㄹ : " << idx.has_match("ㅅㄹ") << "\n"; // 1 (사랑, 사람)
    std::cout << "  any ㅂㄱ : " << idx.has_match("ㅂㄱ") << "\n"; // 0

    // =========================================================================
    // 9. Index management
    // =========================================================================
    std::cout << "\n=== Index Management ===\n";

    std::cout << "  size  : " << idx.size()  << "\n"; // 14
    std::cout << "  empty : " << idx.empty() << "\n"; // 0

    idx.clear();
    std::cout << "  after clear — empty : " << idx.empty() << "\n"; // 1

    // =========================================================================
    // 10. Adding words from a vector (iterator range)
    // =========================================================================
    std::cout << "\n=== Add from Vector ===\n";

    std::vector<std::string> words = { "바나나", "바다", "바람" };
    idx.add(words.begin(), words.end());

    for (const auto& w : idx.search("ㅂㄷ"))
        std::cout << "  " << w << "\n";
    // 바다

    return 0;
}