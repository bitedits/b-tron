/*
 * BTRON 3.20 TCP/IP Network Manager Reference Stubs
 * src/kernel/tcpip.c
 *
 * Implements linkable stubs returning E_NOSPT (-17) for unimplemeted socket APIs.
 */

#include <btron/tcpip.h>
#include <btron/error.h>

ERR so_start(W arg)
{
    (void)arg;
    return E_NOSPT;
}

ERR so_finish(W arg)
{
    (void)arg;
    return E_NOSPT;
}

WERR so_socket(W domain, W type, W protocol)
{
    (void)domain; (void)type; (void)protocol;
    return E_NOSPT;
}

ERR so_bind(W s, struct sockaddr *nam, W namlen)
{
    (void)s; (void)nam; (void)namlen;
    return E_NOSPT;
}

ERR so_listen(W s, W backlog)
{
    (void)s; (void)backlog;
    return E_NOSPT;
}

WERR so_accept(W s, struct sockaddr *nam, W *namlen)
{
    (void)s; (void)nam; (void)namlen;
    return E_NOSPT;
}

ERR so_connect(W s, struct sockaddr *nam, W namlen)
{
    (void)s; (void)nam; (void)namlen;
    return E_NOSPT;
}

WERR so_send(W s, B *buf, W len, W flags)
{
    (void)s; (void)buf; (void)len; (void)flags;
    return E_NOSPT;
}

WERR so_recv(W s, B *buf, W len, W flags)
{
    (void)s; (void)buf; (void)len; (void)flags;
    return E_NOSPT;
}

ERR so_close(W s)
{
    (void)s;
    return E_NOSPT;
}

WERR so_select(W nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *tmout)
{
    (void)nfds; (void)rfds; (void)wfds; (void)efds; (void)tmout;
    return E_NOSPT;
}

ERR so_gethostbyname(B *nam, struct hostent *hp, B *buf)
{
    (void)nam; (void)hp; (void)buf;
    return E_NOSPT;
}

ERR so_gethostname(B *name, W nlen)
{
    (void)name; (void)nlen;
    return E_NOSPT;
}
