/*
 * B-System (BTRON 3.20) Language & IME Settings Header: language_settings.h
 * Settings Cabinet Applet for Multilingual Input Method Configuration (EN/JP/TB)
 */

#ifndef _BTRON_LANGUAGE_SETTINGS_H_
#define _BTRON_LANGUAGE_SETTINGS_H_

#include <btron/types.h>
#include <btron/wnd.h>
#include <btron/event.h>
#include <btron/tip.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    WND *wnd;
    TIP_KEY_SETTINGS current_config;
    TIP_KEY_SETTINGS saved_config;
    int focused_control;
    BOOL is_dirty;
} LanguageSettingsApp;

/**
 * Open the Language & IME Settings Application window.
 * Part of the BTRON Settings Cabinet (環境設定キャビネット).
 * @return Pointer to the opened Settings Window.
 */
WND* open_language_settings_window(void);

/**
 * Event handler for the Language & IME Settings Application.
 */
void language_settings_event_handler(WND *wnd, const EVT *evt);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_LANGUAGE_SETTINGS_H_ */
