/*
 * B-TRON Specification Compatible Header: error.h
 * Standard TRON Error Codes.
 */

#ifndef _BTRON_ERROR_H_
#define _BTRON_ERROR_H_

#include <btron/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define E_OK       (0)   /* Normal completion */
#define E_SYS     (-5)   /* System error */
#define E_NOMEM   (-10)  /* Out of memory */
#define E_NOSPT   (-17)  /* Feature not supported */
#define E_RSVR    (-25)  /* Reserved error */
#define E_PAR     (-33)  /* Parameter error */
#define E_LIMIT   (-34)  /* Limit exceeded */
#define E_ID      (-35)  /* Invalid ID */
#define E_OBJ     (-41)  /* Invalid object state */
#define E_NOEXS   (-52)  /* Object does not exist */
#define E_BUSY    (-65)  /* Resource busy */
#define E_TMOUT   (-69)  /* Timeout */

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_ERROR_H_ */
