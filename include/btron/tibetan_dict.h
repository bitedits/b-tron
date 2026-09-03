/*
 * B-System (BTRON 3.20) Tibetan-English Lexicon & Candidate Engine: tibetan_dict.h
 * Buddhist Terminology, Philosophical Vocabulary & Extended Wylie Candidate Lookup
 */

#ifndef _BTRON_TIBETAN_DICT_H_
#define _BTRON_TIBETAN_DICT_H_

#include <btron/tip.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *wylie;
    const char *tibetan;
    const char *english;
    const char *sanskrit;
} TibetanDictEntry;

/**
 * Lookup Tibetan dictionary candidates matching input Wylie or Tibetan reading.
 * @param wylie_input     Input Wylie ASCII string (e.g. "chos", "snying thig")
 * @param tibetan_reading Direct Wylie-to-Tibetan Unicode string (e.g. "ཆོས")
 * @param out_candidates  Array of TIP_CANDIDATE to fill
 * @param max_candidates  Maximum number of candidates to return
 * @return Number of candidates generated.
 */
int tibetan_lookup_candidates(const char *wylie_input,
                              const char *tibetan_reading,
                              TIP_CANDIDATE *out_candidates,
                              int max_candidates);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_TIBETAN_DICT_H_ */
