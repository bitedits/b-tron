/*
 * B-TRON Chat (BeOS Chat / Blabber Port): include/btron/chat.h
 * Conforming to BTRON3 SPEC 3.20, TRON HMI, and CHAT.md
 */

#ifndef BTRON_CHAT_H
#define BTRON_CHAT_H

#include <btron/types.h>
#include <btron/dp.h>
#include <btron/wnd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHAT_MAX_CLIENTS       16
#define CHAT_MAX_ROSTER        32
#define CHAT_MAX_MSG_HISTORY   64
#define CHAT_MAX_LINE_LEN      256
#define CHAT_MAX_NICK_LEN      32
#define CHAT_MAX_JID_LEN       64
#define CHAT_MAX_ROOM_LEN      32
#define CHAT_DEFAULT_ROOM      "#btron-hall"
#define CHAT_DEFAULT_SERVER    "btron.org"

/* Presence Status */
typedef enum {
    CHAT_STATUS_OFFLINE = 0,
    CHAT_STATUS_ONLINE  = 1,
    CHAT_STATUS_AWAY    = 2,
    CHAT_STATUS_DND     = 3
} ChatStatus;

/* Message Type */
typedef enum {
    CHAT_MSG_CHAT       = 0,  /* 1-on-1 private message */
    CHAT_MSG_GROUPCHAT  = 1,  /* Multi-User Chat room message */
    CHAT_MSG_SYSTEM     = 2,  /* System announcement / join-part notification */
    CHAT_MSG_TOPIC      = 3   /* Room topic message */
} ChatMsgType;

/* Single Message Entry in Transcript History */
typedef struct {
    char sender_nick[CHAT_MAX_NICK_LEN];
    char timestamp[12];     /* "HH:MM:SS" */
    char text[CHAT_MAX_LINE_LEN];
    ChatMsgType type;
    COLOR color;
} ChatMessage;

/* Buddy item in Roster */
typedef struct {
    char jid[CHAT_MAX_JID_LEN];
    char nick[CHAT_MAX_NICK_LEN];
    char group[32];
    ChatStatus status;
    char status_text[64];
} ChatRosterItem;

/* Window Type */
typedef enum {
    CHAT_WND_ROSTER   = 0,
    CHAT_WND_MUC      = 1,
    CHAT_WND_PRIVATE  = 2,
    CHAT_WND_BUDDY    = 3,
    CHAT_WND_PREFS    = 4
} ChatWndType;

/* Chat Client Instance */
typedef struct ChatClient {
    ID client_id;
    char nick[CHAT_MAX_NICK_LEN];
    char jid[CHAT_MAX_JID_LEN];
    char server[32];
    ChatStatus status;
    char status_msg[64];
    BOOL connected;

    /* Roster list */
    ChatRosterItem roster[CHAT_MAX_ROSTER];
    int roster_count;

    /* Windows associated with this client */
    WND *roster_wnd;
    WND *muc_wnd;
    WND *private_wnds[8];
    int private_wnd_count;

    struct ChatClient *next;
} ChatClient;

/* Context attached to active WND->user_data */
typedef struct {
    ChatWndType wnd_type;
    ChatClient *client;
    char target_room_or_nick[CHAT_MAX_ROOM_LEN];
    
    /* Transcript buffer */
    ChatMessage history[CHAT_MAX_MSG_HISTORY];
    int history_count;
    int scroll_y;

    /* Input text box */
    char input_buf[CHAT_MAX_LINE_LEN];
    int input_len;
    int cursor_pos;

    /* Roster UI selection in Main Window */
    int selected_roster_idx;
    int selected_group_idx;
    BOOL group_expanded[4]; /* General, Developers, Friends, MUC Rooms */

    /* Add Buddy / Preferences dialog state */
    char field_jid[CHAT_MAX_JID_LEN];
    char field_nick[CHAT_MAX_NICK_LEN];
    char field_group[32];
    int active_field_idx;
} ChatWndContext;

/* ── Random Word-Nick Generator ── */
void chat_generate_random_nick(char *out_nick, int max_len);

/* ── TRON IPC Pub/Sub Message Bus ── */
void chat_ipc_init(void);
ChatClient* chat_ipc_register_client(const char *preferred_nick);
void chat_ipc_unregister_client(ChatClient *client);
void chat_ipc_broadcast_presence(ChatClient *client);
void chat_ipc_send_muc_message(ChatClient *sender, const char *room, const char *text);
void chat_ipc_send_private_message(ChatClient *sender, const char *target_nick, const char *text);
void chat_ipc_broadcast_system(const char *room, const char *text);
void chat_ipc_poll(void);

/* ── Window Launchers ── */
WND* open_chat_main_window(ChatClient *client);
WND* open_chat_muc_window(ChatClient *client, const char *room_name);
WND* open_chat_private_window(ChatClient *client, const char *target_nick);
WND* open_chat_add_buddy_dialog(ChatClient *client);
WND* open_chat_preferences_dialog(ChatClient *client);

/* Global App Launcher */
WND* launch_beos_chat(void);

#ifdef __cplusplus
}
#endif

#endif /* BTRON_CHAT_H */
