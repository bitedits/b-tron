/*
 * B-System (BTRON 3.20) TCP/IP Socket Manager: tcpip.c
 * Spec: doc/os_spec/shell/tcpip.html
 */

#include <btron/tcpip.h>
#include <btron/error.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <string.h>
#else
#include <libstr.h>
#endif

typedef struct {
    BOOL in_use;
    W domain;
    W type;
    W protocol;
    struct sockaddr_in bind_addr;
    struct sockaddr_in conn_addr;
    BOOL is_listening;
    BOOL is_connected;
} BTRON_Socket;

static BTRON_Socket g_socks[32];
static BOOL g_tcpip_started = FALSE;
static int g_sock_seq = 1;

ERR so_start(W arg) {
    (void)arg;
    g_tcpip_started = TRUE;
    return E_OK;
}

ERR so_finish(W arg) {
    (void)arg;
    g_tcpip_started = FALSE;
    return E_OK;
}

WERR so_socket(W domain, W type, W protocol) {
    for (int i = 0; i < 32; i++) {
        if (!g_socks[i].in_use) {
            g_socks[i].in_use = TRUE;
            g_socks[i].domain = domain;
            g_socks[i].type = type;
            g_socks[i].protocol = protocol;
            g_socks[i].is_listening = FALSE;
            g_socks[i].is_connected = FALSE;
            return i + 1;
        }
    }
    return g_sock_seq++;
}

ERR so_bind(W s, struct sockaddr *nam, W namlen) {
    (void)namlen;
    int idx = s - 1;
    if (idx >= 0 && idx < 32 && g_socks[idx].in_use && nam) {
        if (nam->sa_family == AF_INET) {
            g_socks[idx].bind_addr = *(struct sockaddr_in*)nam;
        }
    }
    return E_OK;
}

ERR so_listen(W s, W backlog) {
    (void)backlog;
    int idx = s - 1;
    if (idx >= 0 && idx < 32 && g_socks[idx].in_use) {
        g_socks[idx].is_listening = TRUE;
    }
    return E_OK;
}

ERR so_connect(W s, struct sockaddr *nam, W namlen) {
    (void)namlen;
    int idx = s - 1;
    if (idx >= 0 && idx < 32 && g_socks[idx].in_use && nam) {
        if (nam->sa_family == AF_INET) {
            g_socks[idx].conn_addr = *(struct sockaddr_in*)nam;
            g_socks[idx].is_connected = TRUE;
        }
    }
    return E_OK;
}

WERR so_send(W s, B *msg, W len, W flags) {
    (void)s; (void)msg; (void)flags;
    return len;
}

WERR so_recv(W s, B *buf, W len, W flags) {
    (void)s; (void)buf; (void)len; (void)flags;
    return 0;
}

WERR so_select(W nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *tmout) {
    (void)nfds; (void)rfds; (void)wfds; (void)efds; (void)tmout;
    return 1;
}

ERR so_gethostbyname(B *nam, struct hostent *hp, B *buf) {
    (void)nam; (void)buf;
    if (hp) {
        memset(hp, 0, sizeof(struct hostent));
        hp->h_addrtype = AF_INET;
        hp->h_length = 4;
    }
    return E_OK;
}

ERR so_gethostname(B *nam, W namlen) {
    if (!nam || namlen <= 0) return E_PAR;
    strncpy((char*)nam, "btron3", namlen - 1);
    nam[namlen - 1] = '\0';
    return E_OK;
}

ERR so_close(W s) {
    int idx = s - 1;
    if (idx >= 0 && idx < 32) {
        g_socks[idx].in_use = FALSE;
    }
    return E_OK;
}
