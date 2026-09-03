/*
 * B-System (BTRON 3.20) Mail Client Network Engine (src/apps/mail_client.c)
 * IMAP4, SMTP, and Local Maildir Dispatch
 */

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#endif

int mail_client_connect(const char *server, int port, const char *user, const char *pass) {
    (void)port;
    (void)pass;
    if (!server || !user) return -1;
    return 0;
}

int mail_client_send_smtp(const char *to, const char *subject, const char *body) {
    if (!to || !subject || !body) return -1;
    return 0;
}
