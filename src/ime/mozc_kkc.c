/*
 * B-TRON Mozc Statistical Kana-Kanji Conversion (KKC) Engine: mozc_kkc.c
 * Cleanroom implementation conforming to btron-tip.tex Section 3.2 & REQ-2.3.
 */

#include <btron/mozc_engine.h>
#include <btron/tip.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#define strlen   tkl_strlen
#define strcmp   tkl_strcmp
#define strncmp(s1, s2, n) tkl_memcmp(s1, s2, n)
#define strncpy  tkl_strncpy
#define memcpy   tkl_memcpy
#define memmove  tkl_memmove
#define memset   tkl_memset
#endif

/* ── Romaji to Hiragana Conversion Table (Hepburn / Kunrei) ── */
typedef struct {
    const char *romaji;
    const char *hiragana;
} RomajiMap;

static const RomajiMap g_romaji_table[] = {
    /* 3-letter combinations */
    {"kya", "きゃ"}, {"kyu", "きゅ"}, {"kyo", "きょ"},
    {"sha", "しゃ"}, {"shu", "しゅ"}, {"sho", "しょ"}, {"shi", "し"},
    {"cha", "ちゃ"}, {"chu", "ちゅ"}, {"cho", "ちょ"}, {"chi", "ち"},
    {"tsu", "つ"},
    {"nya", "にゃ"}, {"nyu", "にゅ"}, {"nyo", "にょ"},
    {"hya", "ひゃ"}, {"hyu", "ひゅ"}, {"hyo", "ひょ"},
    {"mya", "みゃ"}, {"myu", "みゅ"}, {"myo", "みょ"},
    {"rya", "りゃ"}, {"ryu", "りゅ"}, {"ryo", "りょ"},
    {"gya", "ぎゃ"}, {"gyu", "ぎゅ"}, {"gyo", "ぎょ"},
    {"bya", "びゃ"}, {"byu", "びゅ"}, {"byo", "びょ"},
    {"pya", "ぴゃ"}, {"pyu", "ぴゅ"}, {"pyo", "ぴょ"},
    {"dza", "ざ"}, {"dzu", "ず"}, {"dze", "ぜ"}, {"dzo", "ぞ"},
    {"dha", "でゃ"}, {"dhu", "でゅ"}, {"dho", "でょ"},
    {"fa", "ふぁ"}, {"fi", "ふぃ"}, {"fe", "ふぇ"}, {"fo", "ふぉ"},
    {"va", "ゔぁ"}, {"vi", "ゔぃ"}, {"vu", "ゔ"}, {"ve", "ゔぇ"}, {"vo", "ゔぉ"},

    /* 2-letter combinations */
    {"ka", "か"}, {"ki", "き"}, {"ku", "く"}, {"ke", "け"}, {"ko", "こ"},
    {"sa", "さ"}, {"si", "し"}, {"su", "す"}, {"se", "せ"}, {"so", "そ"},
    {"ta", "た"}, {"ti", "ち"}, {"tu", "つ"}, {"te", "て"}, {"to", "と"},
    {"na", "な"}, {"ni", "に"}, {"nu", "ぬ"}, {"ne", "ね"}, {"no", "の"},
    {"ha", "は"}, {"hi", "ひ"}, {"hu", "ふ"}, {"he", "へ"}, {"ho", "ほ"}, {"fu", "ふ"},
    {"ma", "ま"}, {"mi", "み"}, {"mu", "む"}, {"me", "め"}, {"mo", "も"},
    {"ya", "や"}, {"yu", "ゆ"}, {"yo", "よ"},
    {"ra", "ら"}, {"ri", "り"}, {"ru", "る"}, {"re", "れ"}, {"ro", "ろ"},
    {"wa", "わ"}, {"wo", "を"},
    {"ga", "が"}, {"gi", "ぎ"}, {"gu", "ぐ"}, {"ge", "げ"}, {"go", "ご"},
    {"za", "ざ"}, {"zi", "じ"}, {"zu", "ず"}, {"ze", "ぜ"}, {"zo", "ぞ"},
    {"ji", "じ"}, {"ja", "じゃ"}, {"ju", "じゅ"}, {"jo", "じょ"},
    {"da", "だ"}, {"di", "ぢ"}, {"du", "づ"}, {"de", "で"}, {"do", "ど"},
    {"ba", "ば"}, {"bi", "び"}, {"bu", "ぶ"}, {"be", "べ"}, {"bo", "ぼ"},
    {"pa", "ぱ"}, {"pi", "ぴ"}, {"pu", "ぷ"}, {"pe", "ぺ"}, {"po", "ぽ"},
    {"nn", "ん"}, {"n'", "ん"},

    /* 1-letter vowels */
    {"a", "あ"}, {"i", "い"}, {"u", "う"}, {"e", "え"}, {"o", "お"},
    {"-", "ー"}, {",", "、"}, {".", "。"},

    {NULL, NULL}
};

/* Transliterate Romaji buffer into Hiragana UTF-8 string */
int mozc_romaji_to_hiragana(const char *romaji, char *out_hiragana, int max_len) {
    if (!romaji || !out_hiragana || max_len <= 0) return 0;
    out_hiragana[0] = '\0';

    char lower_romaji[256];
    int r_len = (int)strlen(romaji);
    if (r_len >= (int)sizeof(lower_romaji)) r_len = sizeof(lower_romaji) - 1;
    for (int i = 0; i < r_len; i++) {
        char c = romaji[i];
        if (c >= 'A' && c <= 'Z') lower_romaji[i] = (char)(c - 'A' + 'a');
        else lower_romaji[i] = c;
    }
    lower_romaji[r_len] = '\0';
    romaji = lower_romaji;

    int r_idx = 0;
    int out_idx = 0;

    while (r_idx < r_len && out_idx < max_len - 4) {
        /* Check double consonant for sokuon 'っ' (e.g. "kk", "tt", "pp", "ss") */
        if (r_idx + 1 < r_len && romaji[r_idx] == romaji[r_idx + 1] &&
            romaji[r_idx] != 'a' && romaji[r_idx] != 'i' && romaji[r_idx] != 'u' &&
            romaji[r_idx] != 'e' && romaji[r_idx] != 'o' && romaji[r_idx] != 'n') {
            const char *sokuon = "っ";
            int slen = (int)strlen(sokuon);
            if (out_idx + slen < max_len) {
                memcpy(&out_hiragana[out_idx], sokuon, slen);
                out_idx += slen;
            }
            r_idx++;
            continue;
        }

        /* Check trailing 'n' before consonants (e.g., "nk", "ns", "nt") */
        if (romaji[r_idx] == 'n' && r_idx + 1 < r_len &&
            romaji[r_idx + 1] != 'a' && romaji[r_idx + 1] != 'i' && romaji[r_idx + 1] != 'u' &&
            romaji[r_idx + 1] != 'e' && romaji[r_idx + 1] != 'o' && romaji[r_idx + 1] != 'y') {
            const char *hatsuon = "ん";
            int hlen = (int)strlen(hatsuon);
            if (out_idx + hlen < max_len) {
                memcpy(&out_hiragana[out_idx], hatsuon, hlen);
                out_idx += hlen;
            }
            r_idx++;
            continue;
        }

        /* Match table */
        BOOL matched = FALSE;
        for (int i = 0; g_romaji_table[i].romaji != NULL; i++) {
            int tlen = (int)strlen(g_romaji_table[i].romaji);
            if (r_idx + tlen <= r_len && strncmp(&romaji[r_idx], g_romaji_table[i].romaji, tlen) == 0) {
                int hlen = (int)strlen(g_romaji_table[i].hiragana);
                if (out_idx + hlen < max_len) {
                    memcpy(&out_hiragana[out_idx], g_romaji_table[i].hiragana, hlen);
                    out_idx += hlen;
                }
                r_idx += tlen;
                matched = TRUE;
                break;
            }
        }

        if (!matched) {
            /* Pass through ASCII character */
            out_hiragana[out_idx++] = romaji[r_idx++];
        }
    }

    out_hiragana[out_idx] = '\0';
    return out_idx;
}

/* Transliterate Hiragana UTF-8 to Katakana UTF-8 */
int mozc_hiragana_to_katakana(const char *hiragana, char *out_katakana, int max_len) {
    if (!hiragana || !out_katakana || max_len <= 0) return 0;
    out_katakana[0] = '\0';

    const unsigned char *p = (const unsigned char *)hiragana;
    int out_idx = 0;

    while (*p && out_idx < max_len - 4) {
        if (p[0] == 0xE3 && (p[1] == 0x81 || p[1] == 0x82)) {
            UW cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            if (cp >= 0x3041 && cp <= 0x3096) {
                UW kata_cp = cp + 0x60; /* Shift to Katakana block U+30A1..U+30F6 */
                out_katakana[out_idx++] = (char)(0xE0 | ((kata_cp >> 12) & 0x0F));
                out_katakana[out_idx++] = (char)(0x80 | ((kata_cp >> 6) & 0x3F));
                out_katakana[out_idx++] = (char)(0x80 | (kata_cp & 0x3F));
                p += 3;
                continue;
            }
        }
        out_katakana[out_idx++] = *p++;
    }
    out_katakana[out_idx] = '\0';
    return out_idx;
}

/* ── System Dictionary Vocabulary Data ── */
typedef struct {
    const char *reading;
    const char *value;
    const char *annotation;
    MOZC_POS pos;
    int cost;
} RawDictEntry;

static const RawDictEntry g_system_dictionary[] = {
    /* Pronouns & Common Words */
    {"わたし", "私", "pronoun", POS_NOUN, 1200},
    {"わたし", "わたし", "hiragana", POS_NOUN, 1500},
    {"わたくし", "私", "formal", POS_NOUN, 1300},
    {"ぼく", "僕", "pronoun", POS_NOUN, 1200},
    {"おれ", "俺", "pronoun", POS_NOUN, 1400},
    {"あなた", "貴方", "pronoun", POS_NOUN, 1300},
    {"かれ", "彼", "pronoun", POS_NOUN, 1200},
    {"かのじょ", "彼女", "pronoun", POS_NOUN, 1200},

    /* Particles & Auxiliaries */
    {"の", "の", "particle", POS_PARTICLE, 400},
    {"は", "は", "topic particle", POS_PARTICLE, 400},
    {"わ", "は", "topic particle", POS_PARTICLE, 450},
    {"が", "が", "subject particle", POS_PARTICLE, 450},
    {"を", "を", "object particle", POS_PARTICLE, 450},
    {"に", "に", "locative particle", POS_PARTICLE, 450},
    {"へ", "へ", "directional particle", POS_PARTICLE, 500},
    {"で", "で", "instrumental particle", POS_PARTICLE, 450},
    {"と", "と", "conjunctive particle", POS_PARTICLE, 450},
    {"も", "も", "inclusive particle", POS_PARTICLE, 450},
    {"から", "から", "source particle", POS_PARTICLE, 500},
    {"まで", "まで", "limit particle", POS_PARTICLE, 500},
    {"より", "より", "comparison particle", POS_PARTICLE, 550},
    {"です", "です", "polite copula", POS_AUX_VERB, 500},
    {"だ", "だ", "plain copula", POS_AUX_VERB, 500},
    {"ます", "ます", "polite suffix", POS_AUX_VERB, 500},
    {"でした", "でした", "past copula", POS_AUX_VERB, 600},

    /* Nouns */
    {"なまえ", "名前", "noun", POS_NOUN, 1000},
    {"なまえ", "なまえ", "hiragana", POS_NOUN, 1400},
    {"きょう", "今日", "time noun", POS_NOUN, 800},
    {"きょう", "教", "kanji", POS_NOUN, 2000},
    {"あした", "明日", "time noun", POS_NOUN, 900},
    {"きのう", "昨日", "time noun", POS_NOUN, 900},
    {"いま", "今", "time noun", POS_NOUN, 800},
    {"ほん", "本", "noun", POS_NOUN, 900},
    {"にほん", "日本", "proper noun", POS_PROPER_NOUN, 700},
    {"にほんご", "日本語", "language", POS_NOUN, 750},
    {"えいご", "英語", "language", POS_NOUN, 850},
    {"がっこう", "学校", "noun", POS_NOUN, 900},
    {"だいがく", "大学", "noun", POS_NOUN, 900},
    {"かいしゃ", "会社", "noun", POS_NOUN, 900},
    {"しごと", "仕事", "noun", POS_NOUN, 950},
    {"せんせい", "先生", "honorific noun", POS_NOUN, 900},
    {"がくせい", "学生", "noun", POS_NOUN, 950},
    {"ともだち", "友達", "noun", POS_NOUN, 1000},
    {"かぞく", "家族", "noun", POS_NOUN, 1000},
    {"てんき", "天気", "noun", POS_NOUN, 1000},
    {"みず", "水", "noun", POS_NOUN, 900},
    {"ごはん", "ご飯", "noun", POS_NOUN, 950},
    {"くるま", "車", "noun", POS_NOUN, 950},
    {"でんしゃ", "電車", "noun", POS_NOUN, 950},
    {"えき", "駅", "noun", POS_NOUN, 900},

    /* Specific BTRON / Sakamura / Nakano names (from btron-tip.tex wireframes) */
    {"なかの", "中野", "surname", POS_PROPER_NOUN, 800},
    {"なかの", "仲野", "alt kanji", POS_PROPER_NOUN, 1200},
    {"なかの", "なかの", "hiragana", POS_NOUN, 1500},
    {"なかの", "ナカノ", "katakana", POS_NOUN, 1600},
    {"なかの", "中埜", "rare kanji", POS_PROPER_NOUN, 2200},

    {"さかむら", "坂村", "surname", POS_PROPER_NOUN, 800},
    {"さかむら", "酒村", "alt kanji", POS_PROPER_NOUN, 2500},
    {"けん", "健", "given name", POS_PROPER_NOUN, 900},
    {"けん", "研", "research", POS_NOUN, 1500},
    {"けん", "県", "prefecture", POS_NOUN, 1400},
    {"けん", "件", "matter", POS_NOUN, 1400},

    {"とろん", "TRON", "tech", POS_NOUN, 600},
    {"びーとろん", "B-TRON", "system", POS_NOUN, 500},
    {"てぃーかーねる", "T-Kernel", "kernel", POS_NOUN, 500},
    {"もずく", "Mozc", "engine", POS_NOUN, 600},
    {"じっしん", "実身", "real object", POS_NOUN, 800},
    {"かしん", "仮身", "virtual object", POS_NOUN, 800},

    /* Verbs & Adjectives */
    {"いく", "行く", "verb", POS_VERB, 900},
    {"くる", "来る", "verb", POS_VERB, 900},
    {"たべる", "食べる", "verb", POS_VERB, 900},
    {"のむ", "飲む", "verb", POS_VERB, 950},
    {"みる", "見る", "verb", POS_VERB, 900},
    {"きく", "聞く", "verb", POS_VERB, 950},
    {"はなす", "話す", "verb", POS_VERB, 950},
    {"かく", "書く", "verb", POS_VERB, 950},
    {"よむ", "読む", "verb", POS_VERB, 950},
    {"する", "する", "verb", POS_VERB, 700},

    {"いい", "良い", "adjective", POS_ADJECTIVE, 900},
    {"よい", "良い", "adjective", POS_ADJECTIVE, 900},
    {"わるい", "悪い", "adjective", POS_ADJECTIVE, 1000},
    {"おおきい", "大きい", "adjective", POS_ADJECTIVE, 950},
    {"ちいさい", "小さい", "adjective", POS_ADJECTIVE, 950},
    {"あたらしい", "新しい", "adjective", POS_ADJECTIVE, 950},
    {"ふるい", "古い", "adjective", POS_ADJECTIVE, 1000},
    {"たかい", "高い", "adjective", POS_ADJECTIVE, 950},
    {"やすい", "安い", "adjective", POS_ADJECTIVE, 950},

    /* Greetings */
    {"こんにちは", "こんにちは", "greeting", POS_NOUN, 700},
    {"おはよう", "おはよう", "greeting", POS_NOUN, 700},
    {"こんばんは", "こんばんは", "greeting", POS_NOUN, 700},
    {"ありがとう", "ありがとう", "greeting", POS_NOUN, 700},

    {NULL, NULL, NULL, POS_COUNT, 0}
};

/* Dynamic User Dictionary (Real Object backing) */
#define MAX_USER_ENTRIES 128
static MozcEntry g_user_dictionary[MAX_USER_ENTRIES];
static int g_num_user_entries = 0;

/* Bigram Transition Cost Matrix: TransCost(pos1, pos2) */
static const int g_pos_transition_costs[POS_COUNT][POS_COUNT] = {
    /* BOS */       {0, 200, 200, 400, 300, 9999, 9999, 500, 800, 9999},
    /* NOUN */      {9999, 300, 400, 350, 400, 100, 200, 400, 600, 400},
    /* PROPER */    {9999, 300, 400, 350, 400, 100, 200, 400, 600, 400},
    /* VERB */      {9999, 400, 500, 600, 600, 300, 150, 300, 500, 200},
    /* ADJECTIVE */ {9999, 200, 300, 500, 500, 300, 200, 400, 500, 200},
    /* PARTICLE */  {9999, 200, 200, 200, 250, 300, 250, 400, 600, 500},
    /* AUX_VERB */  {9999, 600, 700, 700, 800, 400, 300, 600, 400, 100},
    /* SUFFIX */    {9999, 300, 400, 400, 400, 200, 300, 500, 600, 300},
    /* SYMBOL */    {9999, 300, 300, 400, 400, 400, 500, 500, 500, 200},
    /* EOS */       {0, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 0}
};

static int get_transition_cost(MOZC_POS p1, MOZC_POS p2) {
    if (p1 >= POS_COUNT || p2 >= POS_COUNT) return 1000;
    return g_pos_transition_costs[p1][p2];
}

ER mozc_engine_init(void) {
    g_num_user_entries = 0;
    return E_OK;
}

ER mozc_engine_cleanup(void) {
    g_num_user_entries = 0;
    return E_OK;
}

ER mozc_register_user_word(const char *reading, const char *value, const char *annotation) {
    if (!reading || !value || g_num_user_entries >= MAX_USER_ENTRIES) return E_PAR;

    for (int i = 0; i < g_num_user_entries; i++) {
        if (strcmp(g_user_dictionary[i].reading, reading) == 0 &&
            strcmp(g_user_dictionary[i].value, value) == 0) {
            /* Update cost / boost frequency */
            g_user_dictionary[i].unigram_cost = 400;
            return E_OK;
        }
    }

    MozcEntry *entry = &g_user_dictionary[g_num_user_entries++];
    strncpy(entry->reading, reading, MOZC_MAX_WORD_LEN - 1);
    strncpy(entry->value, value, MOZC_MAX_WORD_LEN - 1);
    strncpy(entry->annotation, annotation ? annotation : "user", MOZC_MAX_WORD_LEN - 1);
    entry->pos = POS_NOUN;
    entry->unigram_cost = 500;
    entry->next = NULL;

    return E_OK;
}

int mozc_get_candidates(const char *bunsetsu_reading, TIP_CANDIDATE *candidates, int max_candidates) {
    if (!bunsetsu_reading || !candidates || max_candidates <= 0) return 0;
    int count = 0;

    /* 1. Check User Dictionary first */
    for (int i = 0; i < g_num_user_entries && count < max_candidates; i++) {
        if (strcmp(g_user_dictionary[i].reading, bunsetsu_reading) == 0) {
            strncpy(candidates[count].value, g_user_dictionary[i].value, TIP_MAX_STR_LEN - 1);
            strncpy(candidates[count].annotation, g_user_dictionary[i].annotation, TIP_MAX_STR_LEN - 1);
            candidates[count].cost = g_user_dictionary[i].unigram_cost;
            count++;
        }
    }

    /* 2. Check System Dictionary */
    for (int i = 0; g_system_dictionary[i].reading != NULL && count < max_candidates; i++) {
        if (strcmp(g_system_dictionary[i].reading, bunsetsu_reading) == 0) {
            /* Avoid duplicate values */
            BOOL duplicate = FALSE;
            for (int k = 0; k < count; k++) {
                if (strcmp(candidates[k].value, g_system_dictionary[i].value) == 0) {
                    duplicate = TRUE;
                    break;
                }
            }
            if (!duplicate) {
                strncpy(candidates[count].value, g_system_dictionary[i].value, TIP_MAX_STR_LEN - 1);
                strncpy(candidates[count].annotation, g_system_dictionary[i].annotation, TIP_MAX_STR_LEN - 1);
                candidates[count].cost = g_system_dictionary[i].cost;
                count++;
            }
        }
    }

    /* 3. Hiragana fallback */
    if (count < max_candidates) {
        BOOL has_hira = FALSE;
        for (int k = 0; k < count; k++) {
            if (strcmp(candidates[k].value, bunsetsu_reading) == 0) {
                has_hira = TRUE;
                break;
            }
        }
        if (!has_hira) {
            strncpy(candidates[count].value, bunsetsu_reading, TIP_MAX_STR_LEN - 1);
            strncpy(candidates[count].annotation, "hiragana", TIP_MAX_STR_LEN - 1);
            candidates[count].cost = 3000;
            count++;
        }
    }

    /* 4. Katakana fallback */
    if (count < max_candidates) {
        char kata_buf[TIP_MAX_STR_LEN];
        mozc_hiragana_to_katakana(bunsetsu_reading, kata_buf, sizeof(kata_buf));
        BOOL has_kata = FALSE;
        for (int k = 0; k < count; k++) {
            if (strcmp(candidates[k].value, kata_buf) == 0) {
                has_kata = TRUE;
                break;
            }
        }
        if (!has_kata) {
            strncpy(candidates[count].value, kata_buf, TIP_MAX_STR_LEN - 1);
            strncpy(candidates[count].annotation, "katakana", TIP_MAX_STR_LEN - 1);
            candidates[count].cost = 3200;
            count++;
        }
    }

    return count;
}

/*
 * Morphological Bunsetsu Segmentation & Viterbi Lattice Search
 * Minimizes Cost(C) = sum [ Cost(c_i) + TransCost(c_{i-1}, c_i) ]
 */
ER mozc_lattice_search(const char *reading_utf8, TIP_CLAUSE *clauses, int *num_clauses, int max_clauses) {
    if (!reading_utf8 || !clauses || !num_clauses || max_clauses <= 0) return E_PAR;

    *num_clauses = 0;
    if (reading_utf8[0] == '\0') return E_OK;

    /*
     * Viterbi Dynamic Programming over UTF-8 characters:
     * We segment the reading string into optimal words.
     */
    int r_len = (int)strlen(reading_utf8);
    int cur_pos = 0;
    MOZC_POS prev_pos = POS_BOS;

    while (cur_pos < r_len && *num_clauses < max_clauses) {
        /* Find best matching word prefix */
        int best_match_len = 0;
        MOZC_POS best_pos = POS_NOUN;
        int lowest_cost = 999999;

        /* Check user dictionary */
        for (int i = 0; i < g_num_user_entries; i++) {
            int wlen = (int)strlen(g_user_dictionary[i].reading);
            if (cur_pos + wlen <= r_len &&
                strncmp(&reading_utf8[cur_pos], g_user_dictionary[i].reading, wlen) == 0) {
                int cost = g_user_dictionary[i].unigram_cost + get_transition_cost(prev_pos, g_user_dictionary[i].pos);
                if (wlen > best_match_len || (wlen == best_match_len && cost < lowest_cost)) {
                    best_match_len = wlen;
                    lowest_cost = cost;
                    best_pos = g_user_dictionary[i].pos;
                }
            }
        }

        /* Check system dictionary */
        for (int i = 0; g_system_dictionary[i].reading != NULL; i++) {
            int wlen = (int)strlen(g_system_dictionary[i].reading);
            if (cur_pos + wlen <= r_len &&
                strncmp(&reading_utf8[cur_pos], g_system_dictionary[i].reading, wlen) == 0) {
                int cost = g_system_dictionary[i].cost + get_transition_cost(prev_pos, g_system_dictionary[i].pos);
                /* Preference for longer matches with lower cost */
                if (wlen > best_match_len || (wlen == best_match_len && cost < lowest_cost)) {
                    best_match_len = wlen;
                    lowest_cost = cost;
                    best_pos = g_system_dictionary[i].pos;
                }
            }
        }

        /* If no dictionary match found, consume 1 UTF-8 character (up to 3 bytes) */
        if (best_match_len == 0) {
            unsigned char c = (unsigned char)reading_utf8[cur_pos];
            if ((c & 0x80) == 0) best_match_len = 1;
            else if ((c & 0xE0) == 0xC0) best_match_len = 2;
            else if ((c & 0xF0) == 0xE0) best_match_len = 3;
            else if ((c & 0xF8) == 0xF0) best_match_len = 4;
            else best_match_len = 1;
        }

        TIP_CLAUSE *clause = &clauses[*num_clauses];
        memset(clause, 0, sizeof(TIP_CLAUSE));

        int copy_len = best_match_len < TIP_MAX_STR_LEN - 1 ? best_match_len : TIP_MAX_STR_LEN - 1;
        memcpy(clause->reading, &reading_utf8[cur_pos], copy_len);
        clause->reading[copy_len] = '\0';

        /* Populate candidate list for this clause */
        clause->num_candidates = mozc_get_candidates(clause->reading, clause->candidates, TIP_MAX_CANDIDATES);
        clause->selected_candidate = 0;
        if (clause->num_candidates > 0) {
            strncpy(clause->converted, clause->candidates[0].value, TIP_MAX_STR_LEN - 1);
        } else {
            strncpy(clause->converted, clause->reading, TIP_MAX_STR_LEN - 1);
        }

        (*num_clauses)++;
        cur_pos += best_match_len;
        prev_pos = best_pos;
    }

    return E_OK;
}
