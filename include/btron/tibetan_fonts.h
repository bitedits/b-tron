#ifndef _BTRON_TIBETAN_FONTS_H_
#define _BTRON_TIBETAN_FONTS_H_

#include <btron/types.h>

#ifdef __cplusplus
extern "C" {
#endif

const UB* get_tibetan_glyph_bitmap(UW codepoint);
const UB* get_tibetan_glyph_bitmap_ex(UW codepoint, H *out_width, H *out_height);
int render_tibetan_stack_bitmap(UW root_cp, UW sub_cp, UW vowel_cp, UB out_buf[32]);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_TIBETAN_FONTS_H_ */
