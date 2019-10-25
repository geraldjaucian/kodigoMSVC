#include "kodigo.h"
#include <android/log.h>
#include <GLES2\gl2.h>
#include <string>

void on_init();
void on_start();
void on_stop();
void on_destroy();

void kod_on_event(const kod_event& event) {
	switch (event.type)
	{
	case KOD_APPEVENT: {
		switch (event.app.state)
		{
		case KOD_APP_INIT: {
			on_init();
			break;
		}
		case KOD_APP_DESTROY: {
			on_destroy();
			break;
		}
		default:
			break;
		}
		break;
	}
	case KOD_WINDOWEVENT: {
		switch (event.window.state)
		{
		case KOD_WINDOW_INIT: {
			on_start();
			break;
		}
		case KOD_WINDOW_DESTROY: {
			on_stop();
			break;
		}
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
}

void on_init() {}
void on_start() {}
void on_stop() {}
void on_destroy() {}

void kod_on_update(double dt) {
	__android_log_print(ANDROID_LOG_DEBUG, TAG, "FPS: %f", 1.0 / dt);
	printf("FRAME_PER_SECOND: %f", 1.0 / dt);
	glClearColor(0.4f, 0.1f, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glSwapBuffers();
}