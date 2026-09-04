/*
 * B-System (BTRON 3.20) Embedded Icons Bundle Header: icons_bundle.h
 * Auto-generated embedded asset lookup table.
 */

#ifndef _BTRON_ICONS_BUNDLE_H_
#define _BTRON_ICONS_BUNDLE_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern C {
#endif

/* Lookup an embedded GIF icon by name (e.g. "cabinet", "appearance", "terminal") */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak))
#endif
const uint8_t* icons_bundle_get(const char *name, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_ICONS_BUNDLE_H_ */
