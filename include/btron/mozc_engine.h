/*
 * B-TRON Specification Compatible Header: mozc_engine.h
 * Mozc Statistical Kana-Kanji Conversion Engine (KKC) for T-Kernel.
 * Specification: btron-tip.tex Section 3.2 (Viterbi N-Gram Cost Lattice Search) & REQ-2.3
 */

#ifndef _BTRON_MOZC_ENGINE_H_
#define _BTRON_MOZC_ENGINE_H_

#include <btron/types.h>
#include <btron/error.h>
#include <btron/tip.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum vocabulary and dictionary constants */
#define MOZC_MAX_WORD_LEN     64
#define MOZC_MAX_CANDIDATES   16
#define MOZC_TRIE_CHILDREN    64

/* Part of Speech (POS) IDs for Bigram Transition Cost Calculation */
typedef enum {
    POS_BOS = 0,     /* Beginning of Sentence */
    POS_NOUN,        /* Meishi (Noun) */
    POS_PROPER_NOUN, /* Koyuu Meishi (Names, Locations, Surnames) */
    POS_VERB,        /* Doushi */
    POS_ADJECTIVE,   /* Keiyoushi */
    POS_PARTICLE,    /* Joshi (wa, ga, o, ni, de, no, e, to, desu) */
    POS_AUX_VERB,    /* Jodoushi (desu, masu, da) */
    POS_SUFFIX,      /* Settouji / Setsubiji */
    POS_SYMBOL,      /* Kigo */
    POS_EOS,         /* End of Sentence */
    POS_COUNT
} MOZC_POS;

/* Dictionary Entry in System / User Dictionary */
typedef struct MozcEntry {
    char reading[MOZC_MAX_WORD_LEN];
    char value[MOZC_MAX_WORD_LEN];
    char annotation[MOZC_MAX_WORD_LEN];
    MOZC_POS pos;
    int unigram_cost;
    struct MozcEntry *next;
} MozcEntry;

/* Trie Node for Fast Morphological Lookup */
typedef struct MozcTrieNode {
    char key;
    MozcEntry *entries;
    struct MozcTrieNode *children;
    struct MozcTrieNode *sibling;
} MozcTrieNode;

/* Lattice Node for Dynamic Programming Viterbi Search */
typedef struct MozcLatticeNode {
    int start_pos;
    int end_pos;
    const MozcEntry *entry;
    int best_cost;
    struct MozcLatticeNode *prev_best;
    struct MozcLatticeNode *next;
} MozcLatticeNode;

/* Engine Initialization & Query */
ER mozc_engine_init(void);
ER mozc_engine_cleanup(void);

/* Transliteration: Romaji -> Hiragana / Katakana */
int mozc_romaji_to_hiragana(const char *romaji, char *out_hiragana, int max_len);
int mozc_hiragana_to_katakana(const char *hiragana, char *out_katakana, int max_len);

/* Morphological Bunsetsu Segmentation & Viterbi Lattice Search */
ER mozc_lattice_search(const char *reading_utf8, TIP_CLAUSE *clauses, int *num_clauses, int max_clauses);

/* Retrieve ranking candidates for a specific bunsetsu clause */
int mozc_get_candidates(const char *bunsetsu_reading, TIP_CANDIDATE *candidates, int max_candidates);

/* User Dictionary Real Object operations */
ER mozc_register_user_word(const char *reading, const char *value, const char *annotation);
ER mozc_load_user_dictionary(const char *filepath);
ER mozc_save_user_dictionary(const char *filepath);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_MOZC_ENGINE_H_ */
