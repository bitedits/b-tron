/*
 * B-System (BTRON 3.20) Tibetan-English Lexicon & Candidate Engine: tibetan_dict.c
 * Canonical Buddhist Digital Resource Center & Rangjung Yeshe Dictionary Subsystem
 */

#include <btron/tibetan_dict.h>
#include <btron/tip.h>
#include "wylie.h"
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
#define snprintf tkl_snprintf
#define memset   tkl_memset
#endif

static const TibetanDictEntry g_tibetan_lexicon[] = {
    /* Fundamental Buddhist & Philosophical Terminology */
    { "chos", "ཆོས", "Dharma / Phenomenon / Truth / Teaching", "Dharma" },
    { "chos nyid", "ཆོས་ཉིད", "Dharmatā / Ultimate Nature of Reality", "Dharmatā" },
    { "chos sku", "ཆོས་སྐུ", "Dharmakāya / Truth Body of Buddha", "Dharmakāya" },
    { "longs sku", "ལོངས་སྐུ", "Saṃbhogakāya / Enjoyment Body", "Saṃbhogakāya" },
    { "sprul sku", "སྤྲུལ་སྐུ", "Nirmāṇakāya / Emanation Body / Tulku", "Nirmāṇakāya" },
    { "sku gsum", "སྐུ་གསུམ", "Trikāya / Three Bodies of Buddha", "Trikāya" },

    { "snying thig", "སྙིང་ཐིག", "Heart Essence / Innermost Vitality (Dzogchen)", "Cittatilaka" },
    { "rdzogs chen", "རྫོགས་ཆེན", "Great Perfection / Atiyoga", "Mahāsaṃdhi" },
    { "phyag rgya chen po", "ཕྱག་རྒྱ་ཆེན་པོ", "Great Seal / Mahāmudrā", "Mahāmudrā" },
    { "dbu ma", "དབུ་མ", "Middle Way Philosophy / Madhyamaka", "Madhyamaka" },
    { "tshad ma", "ཚད་མ", "Valid Cognition / Epistemology & Logic", "Pramāṇa" },
    { "phar phyin", "ཕར་ཕྱིན", "Perfection / Transcendent Virtue", "Pāramitā" },

    { "byang chub", "བྱང་ཆུབ", "Enlightenment / Awakening / Bodhi", "Bodhi" },
    { "byang chub sems", "བྱང་ཆུབ་སེམས", "Awakened Mind / Bodhicitta", "Bodhicitta" },
    { "byang chub sems dpa'", "བྱང་ཆུབ་སེམས་དཔའ", "Bodhisattva / Heroic Being", "Bodhisattva" },
    { "sems", "སེམས", "Mind / Mental Consciousness / Citta", "Citta" },
    { "sems nyid", "སེམས་ཉིད", "Nature of Mind / Pure Awareness", "Cittatā" },
    { "rig pa", "རིག་པ", "Intrinsic Pure Awareness / Vidya", "Vidyā" },
    { "ma rig pa", "མ་རིག་པ", "Fundamental Ignorance / Delusion", "Avidyā" },
    { "ye shes", "ཡེ་ཤེས", "Primordial Pristine Wisdom", "Jñāna" },
    { "shes rab", "ཤེས་རབ", "Transcendent Wisdom / Discriminating Insight", "Prajñā" },
    { "thabs", "ཐབས", "Skillful Means / Method", "Upāya" },
    { "snying rje", "སྙིང་རྗེ", "Universal Compassion / Karuṇā", "Karuṇā" },
    { "byams pa", "བྱམས་པ", "Loving Kindness / Maitrī", "Maitrī" },

    { "stong pa nyid", "སྟོང་པ་ཉིད", "Emptiness / Voidness / Śūnyatā", "Śūnyatā" },
    { "bde ba", "བདེ་བ", "Bliss / Joy / Well-Being / Sukha", "Sukha" },
    { "sdug bsngal", "སྡུག་བསྔལ", "Suffering / Dissatisfaction / Duḥkha", "Duḥkha" },
    { "las", "ལས", "Action / Intentional Deeds / Karma", "Karma" },
    { "'khor ba", "འཁོར་བ", "Cyclic Existence / Saṃsāra", "Saṃsāra" },
    { "mya ngan las 'das pa", "མྱ་ངན་ལས་འདས་པ", "Nirvāṇa / Cessation of Suffering", "Nirvāṇa" },
    { "kun gzhi", "ཀུན་གཞི", "All-Ground Consciousness / Ālayavijñāna", "Ālayavijñāna" },

    { "bla ma", "བླ་མ", "Guru / Spiritual Teacher / Lama", "Guru" },
    { "sangs rgyas", "སངས་རྒྱས", "Buddha / Fully Awakened One", "Buddha" },
    { "dge 'dun", "དགེ་འདུན", "Saṅgha / Noble Spiritual Community", "Saṅgha" },
    { "dkon mchog gsum", "དཀོན་མཆོག་གསུམ", "Three Precious Jewels / Triratna", "Triratna" },
    { "yi dam", "ཡི་དམ", "Meditation Deity / Iṣṭadevatā", "Iṣṭadevatā" },
    { "mkha' 'gro ma", "མཁའ་འགྲོ་མ", "Sky Farer / Wisdom Dākinī", "Dākinī" },
    { "chos skyong", "ཆོས་སྐྱོང", "Dharmapāla / Dharma Protector", "Dharmapāla" },

    { "bka' 'gyur", "བཀའ་འགྱུར", "Kangyur / Word of the Buddha Canon", "Buddhavacana" },
    { "bstan 'gyur", "བསྟན་འགྱུར", "Tengyur / Translated Shastra Treatises", "Śāstra" },
    { "sutra", "མདོ", "Discourse / Sutra Scripture", "Sūtra" },
    { "mdo", "མདོ", "Sutra / Discourse of the Buddha", "Sūtra" },
    { "rgyud", "རྒྱུད", "Continuum / Tantra Scripture", "Tantra" },
    { "smon lam", "སྨོན་ལམ", "Aspiration Prayer / Praṇidhāna", "Praṇidhāna" },
    { "tshogs", "ཚོགས", "Assembly / Gathering of Merit (Gaṇacakra)", "Gaṇacakra" },
    { "dbang", "དབང", "Empowerment / Initiation / Abhiṣeka", "Abhiṣeka" },
    { "khrid", "ཁྲིད", "Practical Guidance / Meditation Manual", "Upadeśa" },
    { "gter ma", "གཏེར་མ", "Treasure Revelation / Terma", "Nidhi" },

    /* Mantras & Invocations */
    { "oM AH hU~M", "ཨོཾ་ཨཱཿ་ཧཱུྃ", "Mantra of Body, Speech and Mind", "Kāyavākcitta" },
    { "oM ma Ni pad+me hU~M", "ཨོཾ་མ་ཎི་པདྨེ་ཧཱུྃ", "Six-Syllable Avalokiteśvara Mantra", "Ṣaḍakṣara" },
    { "oM badz+ra sa twa hU~M", "ཨོཾ་བཛྲ་སཏྭ་ཧཱུྃ", "Vajrasattva Purification Mantra", "Vajrasattva" },
    { "badz+ra", "བཛྲ", "Indestructible Diamond Scepter / Vajra", "Vajra" },
    { "pad+ma", "པདྨ", "Lotus Flower / Purity / Padma", "Padma" },
    { "rat+na", "རཏྣ", "Precious Jewel / Ratna", "Ratna" },
    { "kar+ma", "ཀརྨ", "Action / Activity / Karma Family", "Karma" },

    /* Grammatical Particles & Common Words */
    { "kyi", "ཀྱི", "Genitive Particle (of / 's)", "Genitive" },
    { "gi", "གི", "Genitive Particle (of / 's)", "Genitive" },
    { "gyi", "གྱི", "Genitive Particle (of / 's)", "Genitive" },
    { "'i", "འི", "Genitive Particle (of / 's)", "Genitive" },
    { "la", "ལ", "Locative / Dative Particle (in / at / to)", "Locative" },
    { "nas", "ནས", "Ablative Particle (from / having done)", "Ablative" },
    { "las", "ལས", "Comparative / Source Particle (than / from)", "Source" },
    { "dang", "དང", "Conjunctive Particle (and / with)", "Conjunction" },
    { "kyang", "ཀྱང", "Concessive Particle (even / also / although)", "Concessive" },
    { "yang", "ཡང", "Concessive Particle (also / too)", "Concessive" },
    { "ste", "སྟེ", "Continuative Particle (and / semi-colon)", "Continuative" },
    { "te", "ཏེ", "Continuative Particle (and / which)", "Continuative" },
    { "de", "དེ", "Demonstrative Pronoun (that / the aforesaid)", "That" },
    { "'di", "འདི", "Demonstrative Pronoun (this / the present)", "This" },
    { "pa", "པ", "Nominalizing / Agent Particle", "Nominalizer" },
    { "ba", "བ", "Nominalizing / Affirmative Particle", "Nominalizer" },
    { "po", "པོ", "Masculine / Agentive Particle", "Agent" },
    { "mo", "མོ", "Feminine Particle", "Feminine" },

    { NULL, NULL, NULL, NULL }
};

int tibetan_lookup_candidates(const char *wylie_input,
                              const char *tibetan_reading,
                              TIP_CANDIDATE *out_candidates,
                              int max_candidates) {
    if (!out_candidates || max_candidates <= 0) return 0;

    int count = 0;

    /* 1. Exact Dictionary Matches */
    if (wylie_input && wylie_input[0] != '\0') {
        for (int i = 0; g_tibetan_lexicon[i].wylie != NULL && count < max_candidates; i++) {
            if (strcmp(wylie_input, g_tibetan_lexicon[i].wylie) == 0) {
                strncpy(out_candidates[count].value, g_tibetan_lexicon[i].tibetan, TIP_MAX_STR_LEN - 1);
                out_candidates[count].value[TIP_MAX_STR_LEN - 1] = '\0';
                
                strncpy(out_candidates[count].annotation, g_tibetan_lexicon[i].english, TIP_MAX_STR_LEN - 1);
                out_candidates[count].annotation[TIP_MAX_STR_LEN - 1] = '\0';
                out_candidates[count].cost = 10;
                count++;
                break;
            }
        }
    }

    /* 2. Direct Transliteration Candidate */
    if (tibetan_reading && tibetan_reading[0] != '\0' && count < max_candidates) {
        /* Check if not already identical to candidate 0 */
        if (count == 0 || strcmp(out_candidates[0].value, tibetan_reading) != 0) {
            strncpy(out_candidates[count].value, tibetan_reading, TIP_MAX_STR_LEN - 1);
            out_candidates[count].value[TIP_MAX_STR_LEN - 1] = '\0';
            strncpy(out_candidates[count].annotation, "Wylie Direct Transliteration", TIP_MAX_STR_LEN - 1);
            out_candidates[count].cost = 20;
            count++;
        }
    }

    /* 3. Prefix/Partial Dictionary Matches */
    if (wylie_input && wylie_input[0] != '\0') {
        size_t in_len = strlen(wylie_input);
        for (int i = 0; g_tibetan_lexicon[i].wylie != NULL && count < max_candidates; i++) {
            if (strncmp(g_tibetan_lexicon[i].wylie, wylie_input, in_len) == 0 &&
                strcmp(g_tibetan_lexicon[i].wylie, wylie_input) != 0) {
                
                /* Avoid duplicate values */
                int dup = 0;
                for (int k = 0; k < count; k++) {
                    if (strcmp(out_candidates[k].value, g_tibetan_lexicon[i].tibetan) == 0) {
                        dup = 1; break;
                    }
                }
                if (!dup) {
                    strncpy(out_candidates[count].value, g_tibetan_lexicon[i].tibetan, TIP_MAX_STR_LEN - 1);
                    out_candidates[count].value[TIP_MAX_STR_LEN - 1] = '\0';
                    strncpy(out_candidates[count].annotation, g_tibetan_lexicon[i].english, TIP_MAX_STR_LEN - 1);
                    out_candidates[count].annotation[TIP_MAX_STR_LEN - 1] = '\0';
                    out_candidates[count].cost = 30 + (int)(strlen(g_tibetan_lexicon[i].wylie) - in_len);
                    count++;
                }
            }
        }
    }

    /* 4. Variant with Terminal Tsheg (་) */
    if (tibetan_reading && tibetan_reading[0] != '\0' && count < max_candidates) {
        char with_tsheg[TIP_MAX_STR_LEN];
        snprintf(with_tsheg, sizeof(with_tsheg), "%s\xe0\xbc\x8b", tibetan_reading); /* 0x0F0B in UTF-8 */
        strncpy(out_candidates[count].value, with_tsheg, TIP_MAX_STR_LEN - 1);
        out_candidates[count].value[TIP_MAX_STR_LEN - 1] = '\0';
        strncpy(out_candidates[count].annotation, "With Syllable Tsheg (་)", TIP_MAX_STR_LEN - 1);
        out_candidates[count].cost = 40;
        count++;
    }

    /* 5. Variant with Terminal Shad (།) */
    if (tibetan_reading && tibetan_reading[0] != '\0' && count < max_candidates) {
        char with_shad[TIP_MAX_STR_LEN];
        snprintf(with_shad, sizeof(with_shad), "%s\xe0\xbc\x8d", tibetan_reading); /* 0x0F0D in UTF-8 */
        strncpy(out_candidates[count].value, with_shad, TIP_MAX_STR_LEN - 1);
        out_candidates[count].value[TIP_MAX_STR_LEN - 1] = '\0';
        strncpy(out_candidates[count].annotation, "With Verse Shad (།)", TIP_MAX_STR_LEN - 1);
        out_candidates[count].cost = 50;
        count++;
    }

    /* 6. Raw Wylie ASCII String Candidate */
    if (wylie_input && wylie_input[0] != '\0' && count < max_candidates) {
        strncpy(out_candidates[count].value, wylie_input, TIP_MAX_STR_LEN - 1);
        out_candidates[count].value[TIP_MAX_STR_LEN - 1] = '\0';
        strncpy(out_candidates[count].annotation, "Raw Wylie ASCII", TIP_MAX_STR_LEN - 1);
        out_candidates[count].cost = 60;
        count++;
    }

    return count;
}
