#pragma once
/**
 * matchChoseong.hpp — Korean Choseong (Initial Consonant) Search Library
 *
 * Supports searching by initial consonants (choseong) of Korean syllables.
 * Automatically detects one of three search modes:
 *
 *   1. Pure choseong   "ㅎㄱ"  → matches syllables whose choseong are ㅎ, ㄱ
 *   2. Mixed          "학ㄱ"  → exact match for syllables, choseong match for jamo
 *   3. Normal         "학교"  → exact match (standard string search)
 *
 * Requirements : C++17 or later, header-only, no dependencies
 *
 * Quick start:
 *   matchChoseong::Index idx;
 *   idx.add({"한글", "학교", "하늘"});
 *   auto results = idx.search("ㅎㄱ"); // → {"한글", "학교"}
 *
 * MIT License — Copyright (c) 2026 - NOOYOUNG
 */

#include <string>
#include <string_view>
#include <vector>

namespace matchChoseong {

// =============================================================================
// UTF-8 Decoder
//
// Compliant with RFC 3629 / Unicode standard.
// Validates all of the following:
//   - Overlong encoding prevention
//   - Surrogate range block (U+D800~U+DFFF)
//   - Out-of-range codepoints (above U+10FFFF)
//   - Truncated sequence / buffer overread prevention
//   - Invalid bytes replaced with U+FFFD (replacement character)
// =============================================================================

/// Replacement character for invalid or undecodable bytes (U+FFFD)
constexpr char32_t REPLACEMENT_CHAR = 0xFFFD;

/// Returns true if the byte is a UTF-8 continuation byte (10xxxxxx)
inline bool is_continuation(unsigned char c) {
    return (c & 0xC0) == 0x80;
}

/**
 * Decodes a UTF-8 string into a vector of UTF-32 codepoints.
 *
 * @param s  UTF-8 encoded string (string_view)
 * @return   Vector of codepoints. Invalid bytes are replaced with U+FFFD.
 */
inline std::vector<char32_t> decode_utf8(std::string_view s) {
    std::vector<char32_t> result;
    result.reserve(s.size()); // worst case: all ASCII

    size_t i = 0;
    const size_t len = s.size();

    while (i < len) {
        unsigned char c = static_cast<unsigned char>(s[i]);

        // 1-byte sequence (ASCII: U+0000 ~ U+007F)
        if (c <= 0x7F) {
            result.push_back(c);
            i++;
        }

        // 2-byte sequence (U+0080 ~ U+07FF)
        else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= len || !is_continuation(s[i + 1])) {
                result.push_back(REPLACEMENT_CHAR);
                i++;
                continue;
            }
            char32_t cp =
                ((c & 0x1F) << 6) |
                (static_cast<unsigned char>(s[i + 1]) & 0x3F);

            // Overlong check: 2-byte sequences must encode U+0080 or above
            result.push_back(cp >= 0x80 ? cp : REPLACEMENT_CHAR);
            i += 2;
        }

        // 3-byte sequence (U+0800 ~ U+FFFF)
        // Korean syllables (U+AC00~U+D7A3) and jamo (U+3131~U+314E) are in this range
        else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= len ||
                !is_continuation(s[i + 1]) ||
                !is_continuation(s[i + 2])) {
                result.push_back(REPLACEMENT_CHAR);
                i++;
                continue;
            }
            char32_t cp =
                ((c & 0x0F) << 12) |
                ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 6) |
                (static_cast<unsigned char>(s[i + 2]) & 0x3F);

            // Overlong check (must be U+0800 or above)
            // Surrogate range block (U+D800~U+DFFF are reserved for UTF-16)
            if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) {
                result.push_back(REPLACEMENT_CHAR);
            } else {
                result.push_back(cp);
            }
            i += 3;
        }

        // 4-byte sequence (U+10000 ~ U+10FFFF)
        else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= len ||
                !is_continuation(s[i + 1]) ||
                !is_continuation(s[i + 2]) ||
                !is_continuation(s[i + 3])) {
                result.push_back(REPLACEMENT_CHAR);
                i++;
                continue;
            }
            char32_t cp =
                ((c & 0x07) << 18) |
                ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 12) |
                ((static_cast<unsigned char>(s[i + 2]) & 0x3F) << 6) |
                (static_cast<unsigned char>(s[i + 3]) & 0x3F);

            // Valid Unicode range: U+10000 ~ U+10FFFF
            if (cp < 0x10000 || cp > 0x10FFFF) {
                result.push_back(REPLACEMENT_CHAR);
            } else {
                result.push_back(cp);
            }
            i += 4;
        }

        // Invalid leading byte
        else {
            result.push_back(REPLACEMENT_CHAR);
            i++;
        }
    }

    return result;
}

// =============================================================================
// Hangul Unicode Constants
// =============================================================================

constexpr char32_t HANGUL_BASE   = 0xAC00; ///< First Hangul syllable block (가)
constexpr char32_t HANGUL_END    = 0xD7A3; ///< Last Hangul syllable block  (힣)
constexpr int      NUM_JUNGSEONG = 21;      ///< Number of vowels (jungseong)
constexpr int      NUM_JONGSEONG = 28;      ///< Number of final consonants (jongseong), including null
constexpr int      SYLLABLE_BLOCK = NUM_JUNGSEONG * NUM_JONGSEONG; ///< 21 × 28 = 588

/// Choseong (initial consonant) jamo table, 19 entries in Unicode order
constexpr char32_t CHOSEONG_TABLE[19] = {
    0x3131, // ㄱ
    0x3132, // ㄲ
    0x3134, // ㄴ
    0x3137, // ㄷ
    0x3138, // ㄸ
    0x3139, // ㄹ
    0x3141, // ㅁ
    0x3142, // ㅂ
    0x3143, // ㅃ
    0x3145, // ㅅ
    0x3146, // ㅆ
    0x3147, // ㅇ
    0x3148, // ㅈ
    0x3149, // ㅉ
    0x314A, // ㅊ
    0x314B, // ㅋ
    0x314C, // ㅌ
    0x314D, // ㅍ
    0x314E, // ㅎ
};

// =============================================================================
// Internal Utilities (detail namespace)
// Not intended for direct use by library consumers.
// =============================================================================

namespace detail {

/// Returns true if cp is a Hangul syllable block (가 ~ 힣)
inline bool is_hangul_syllable(char32_t cp) {
    return cp >= HANGUL_BASE && cp <= HANGUL_END;
}

/**
 * Returns true if cp is a standalone choseong jamo (ㄱ ~ ㅎ).
 * Uses a range check for O(1) performance instead of looping through the table.
 *
 * Note: U+3131~U+314E also includes compound consonants like ㄳ, ㄵ,
 *       but all 19 valid choseong are within this range, which is sufficient
 *       for search purposes.
 */
inline bool is_choseong_jamo(char32_t cp) {
    return cp >= 0x3131 && cp <= 0x314E;
}

/**
 * Extracts the choseong jamo from a Hangul syllable.
 *
 * Formula (by Unicode Hangul design):
 *   codepoint = HANGUL_BASE + (choseong_index × 588) + (jungseong × 28) + jongseong
 *   → choseong_index = (codepoint - HANGUL_BASE) / 588
 *
 * @param syllable  A Hangul syllable codepoint (must satisfy is_hangul_syllable())
 * @return          The corresponding choseong jamo codepoint
 */
inline char32_t extract_choseong(char32_t syllable) {
    int index = static_cast<int>(syllable - HANGUL_BASE) / SYLLABLE_BLOCK;
    return CHOSEONG_TABLE[index];
}

/**
 * Returns the "choseong representative" of a codepoint:
 *   - Hangul syllable → its choseong jamo
 *   - Anything else   → the codepoint itself (ASCII, digits, etc.)
 */
inline char32_t to_choseong(char32_t cp) {
    return is_hangul_syllable(cp) ? extract_choseong(cp) : cp;
}

/**
 * Returns true if query_char matches text_char under choseong search rules.
 *
 * Rules:
 *   - If query_char is a choseong jamo (ㄱ~ㅎ), compare against text_char's choseong
 *   - Otherwise, require an exact codepoint match
 */
inline bool chars_match(char32_t query_char, char32_t text_char) {
    if (is_choseong_jamo(query_char)) {
        return to_choseong(text_char) == query_char;
    }
    return query_char == text_char;
}

} // namespace detail

// =============================================================================
// Public API
// =============================================================================

/**
 * Result of a single search operation.
 * pos and len are valid only when matched == true.
 */
struct MatchResult {
    bool   matched = false; ///< Whether a match was found
    size_t pos     = 0;     ///< Match start position in codepoints (0-based)
    size_t len     = 0;     ///< Match length in codepoints
};

/**
 * Finds the first occurrence of query in text using choseong-aware matching.
 *
 * @param text   UTF-8 string to search in
 * @param query  UTF-8 search query (may contain standalone jamo like ㄱ~ㅎ)
 * @return       First match position and length, or matched=false if not found
 *
 * Examples:
 *   search("한글 학교", "ㅎㄱ") → {true, 0, 2}   matches "한글"
 *   search("한글 학교", "학ㄱ") → {true, 3, 2}   matches "학교"
 *   search("한글 학교", "ㅅ")   → {false, 0, 0}
 */
inline MatchResult search(std::string_view text, std::string_view query) {
    const auto t_cps = decode_utf8(text);
    const auto q_cps = decode_utf8(query);

    if (q_cps.empty() || t_cps.size() < q_cps.size()) {
        return {};
    }

    const size_t t_len = t_cps.size();
    const size_t q_len = q_cps.size();

    // Sliding window: compare q_len characters at each position in text
    for (size_t i = 0; i <= t_len - q_len; ++i) {
        bool matched = true;
        for (size_t j = 0; j < q_len; ++j) {
            if (!detail::chars_match(q_cps[j], t_cps[i + j])) {
                matched = false;
                break;
            }
        }
        if (matched) return { true, i, q_len };
    }

    return {};
}

/**
 * Returns true if query appears anywhere in text.
 * Convenience wrapper around search() when position is not needed.
 */
inline bool contains(std::string_view text, std::string_view query) {
    return search(text, query).matched;
}

/**
 * Finds all occurrences of query in text.
 * Useful for highlighting every matched position.
 *
 * @param text   UTF-8 string to search in
 * @param query  UTF-8 search query (may contain standalone jamo)
 * @return       All match positions and lengths
 */
inline std::vector<MatchResult> search_all(std::string_view text,
                                           std::string_view query) {
    const auto t_cps = decode_utf8(text);
    const auto q_cps = decode_utf8(query);

    std::vector<MatchResult> results;

    if (q_cps.empty() || t_cps.size() < q_cps.size()) {
        return results;
    }

    const size_t t_len = t_cps.size();
    const size_t q_len = q_cps.size();

    for (size_t i = 0; i <= t_len - q_len; ++i) {
        bool matched = true;
        for (size_t j = 0; j < q_len; ++j) {
            if (!detail::chars_match(q_cps[j], t_cps[i + j])) {
                matched = false;
                break;
            }
        }
        if (matched) results.push_back({ true, i, q_len });
    }

    return results;
}

// =============================================================================
// Index — word list search
// =============================================================================

/**
 * Manages a list of words and provides choseong-aware search over them.
 *
 * Example:
 *   matchChoseong::Index idx;
 *   idx.add({"한글", "학교", "하늘", "대한민국"});
 *
 *   idx.search("ㅎㄱ");     // → {"한글", "학교"}
 *   idx.search("학ㄱ");     // → {"학교"}
 *   idx.search("ㄷㅎㅁㄱ"); // → {"대한민국"}
 */
class Index {
public:
    Index() = default;

    // ── Insertion ────────────────────────────────────────────────────────────

    /// Adds a single word to the index.
    void add(std::string word) {
        if (!word.empty()) {
            words_.push_back(std::move(word));
        }
    }

    /// Adds multiple words at once via initializer_list.
    void add(std::initializer_list<std::string> words) {
        for (const auto& w : words) add(w);
    }

    /// Adds words from an iterator range (supports any STL container).
    template <typename InputIt>
    void add(InputIt first, InputIt last) {
        for (; first != last; ++first) add(*first);
    }

    // ── Search ───────────────────────────────────────────────────────────────

    /**
     * Returns all words that contain query as a choseong-aware substring.
     *
     * @param query  Search query (may contain standalone jamo)
     * @return       Matched words in insertion order
     */
    std::vector<std::string> search(std::string_view query) const {
        std::vector<std::string> results;
        for (const auto& word : words_) {
            if (contains(word, query)) {
                results.push_back(word);
            }
        }
        return results;
    }

    /**
     * Returns true if at least one word in the index matches query.
     */
    bool has_match(std::string_view query) const {
        for (const auto& word : words_) {
            if (contains(word, query)) return true;
        }
        return false;
    }

    // ── Management ───────────────────────────────────────────────────────────

    void   clear() { words_.clear(); }           ///< Removes all words from the index
    size_t size()  const { return words_.size(); } ///< Returns the number of indexed words
    bool   empty() const { return words_.empty(); } ///< Returns true if the index is empty

private:
    std::vector<std::string> words_;
};

} // namespace matchChoseong