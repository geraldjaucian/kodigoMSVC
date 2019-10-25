#pragma once

#define KOD_EVENT_BUFFER_SIZE 20

#define KOD_FRAME_PER_SECOND 24

#define TAG "native_kodigo"

typedef enum kod_event_type {
	KOD_UNKNOWNEVENT, KOD_APPEVENT, KOD_WINDOWEVENT, KOD_TOUCHEVENT, KOD_MOUSEEVENT, KOD_KEYEVENT
} kod_event_type;

typedef enum kod_app_state {
	KOD_APP_UNKNOWN, KOD_APP_INIT, KOD_APP_RESUME, KOD_APP_PAUSE, KOD_APP_DESTROY
} kod_app_state;

typedef enum kod_window_state {
	KOD_WINDOW_UNKNOWN, KOD_WINDOW_INIT, KOD_WINDOW_FOCUS_CHANGE, KOD_WINDOW_ON_LAYOUT, KOD_WINDOW_DESTROY
} kod_window_state;

typedef enum kod_touch_state {
	KOD_TOUCH_UNKNOWN, KOD_TOUCH_DOWN, KOD_TOUCH_MOVE, KOD_TOUCH_UP
} kod_touch_state;

typedef enum kod_mouse_state {
	KOD_MOUSE_UNKNOWN, KOD_MOUSE_DOWN, KOD_MOUSE_MOVE, KOD_MOUSE_UP
} kod_mouse_state;

typedef enum kod_key_state {
	KOD_KEY_UNKNOWN, KOD_KEY_DOWN, KOD_KEY_UP
} kod_key_state;

typedef struct kod_appevent {
	kod_app_state state;
} kod_appevent;

typedef struct kod_windowevent {
	kod_window_state state;
	union {
		bool focus;
		struct {
			int width, height;
			int pad_left, pad_right, pad_top, pad_bottom;
		};
	};
} kod_windowevent;

typedef struct kod_touchevent {
	kod_touch_state state;
	int index;
	float x, y, raw_x, raw_y;
} kod_touchevent;

typedef struct kod_mouseevent {
	kod_mouse_state state;
	int index;
	float x, y, raw_x, raw_y;
} kod_mouseevent;

typedef struct kod_keyevent {
	kod_key_state state;
} kod_keyevent;

typedef struct kod_event {
	kod_event_type type;
	union {
		kod_appevent app;
		kod_windowevent window;
		kod_touchevent touch;
		kod_mouseevent mouse;
		kod_keyevent key;
	};
} kod_event;

extern "C" {
	void kod_on_event(const kod_event&);
	void kod_on_update(double);
}

bool glSwapBuffers();