#include "kodigo.h"

#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstring>
#include <cassert>
#include <chrono>
#ifdef _DEBUG
#include <string>
#endif


std::thread _thread;
std::mutex _mutex;
std::condition_variable _cond;

std::atomic_flag _spin{ 0 };
kod_event _events[KOD_EVENT_BUFFER_SIZE];
size_t _event_size{ 0 };

ANativeWindow* _window{ nullptr };
ANativeWindow* _pending_window{ nullptr };

EGLContext _context{ EGL_NO_CONTEXT };
EGLDisplay _display{ EGL_NO_DISPLAY };
EGLSurface _surface{ EGL_NO_SURFACE };


inline void _push_event(const kod_event& event) {
	while (_spin.test_and_set(std::memory_order_acquire));
	if (_event_size == KOD_EVENT_BUFFER_SIZE) {
		_spin.clear(std::memory_order_release);
		std::unique_lock<std::mutex> lock{ _mutex };
		_cond.notify_all();
		while (_event_size == KOD_EVENT_BUFFER_SIZE) _cond.wait(lock);
	}
	else {
		_events[_event_size++] = event;
		_spin.clear(std::memory_order_release);
	}
}

kod_app_state _app_state{ KOD_APP_UNKNOWN };

inline void _on_event(const kod_event& event) {
	switch (event.type)
	{
	case KOD_APPEVENT: {
		_app_state = event.app.state;
		break;
	}
	case KOD_WINDOWEVENT: {
		switch (event.window.state)
		{
		case KOD_WINDOW_INIT: {
			if (_pending_window) {
				_window = _pending_window;
				_pending_window = nullptr;

				const EGLint attribs[] = {
						EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
						EGL_BLUE_SIZE, 8,
						EGL_GREEN_SIZE, 8,
						EGL_RED_SIZE, 8,
						EGL_NONE
				};
				EGLint format;
				EGLint numConfigs;
				EGLConfig config;
				_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

				eglInitialize(_display, 0, 0);

				/* Here, the application chooses the configuration it desires.
				 * find the best match if possible, otherwise use the very first one
				 */
				eglChooseConfig(_display, attribs, nullptr, 0, &numConfigs);
				std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
				assert(supportedConfigs);
				eglChooseConfig(_display, attribs, supportedConfigs.get(), numConfigs,
					&numConfigs);
				assert(numConfigs);
				auto i = 0;
				for (; i < numConfigs; i++) {
					auto& cfg = supportedConfigs[i];
					EGLint r, g, b, d;
					if (eglGetConfigAttrib(_display, cfg, EGL_RED_SIZE, &r) &&
						eglGetConfigAttrib(_display, cfg, EGL_GREEN_SIZE, &g) &&
						eglGetConfigAttrib(_display, cfg, EGL_BLUE_SIZE, &b) &&
						eglGetConfigAttrib(_display, cfg, EGL_DEPTH_SIZE, &d) &&
						r == 8 && g == 8 && b == 8 && d == 0) {

						config = supportedConfigs[i];
						break;
					}
				}
				if (i == numConfigs) {
					config = supportedConfigs[0];
				}

				/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
				 * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
				 * As soon as we picked a EGLConfig, we can safely reconfigure the
				 * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
				eglGetConfigAttrib(_display, config, EGL_NATIVE_VISUAL_ID, &format);
				_surface = eglCreateWindowSurface(_display, config, _window, NULL);
				_context = eglCreateContext(_display, config, NULL, NULL);

				if (eglMakeCurrent(_display, _surface, _surface, _context) ==
					EGL_FALSE) {
					return;
				}
			}
			break;
		}
		case KOD_WINDOW_DESTROY: {
			if (_display != EGL_NO_DISPLAY) {
				eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
				if (_context != EGL_NO_CONTEXT) {
					eglDestroyContext(_display, _context);
				}
				if (_surface != EGL_NO_SURFACE) {
					eglDestroySurface(_display, _surface);
				}
				eglTerminate(_display);
			}
			_window = nullptr;
			_display = EGL_NO_DISPLAY;
			_context = EGL_NO_CONTEXT;
			_surface = EGL_NO_SURFACE;
			break;
		}
		default:
			break;
		}
	}
	default:
		break;
	}
}

extern "C" {
	JNIEXPORT void JNICALL
		Java_app_kodigo_gj_pixelized_MainActivity_nativeOnCreate(JNIEnv* env, jobject activity) {

		__android_log_print(ANDROID_LOG_DEBUG, TAG, "onCreate()");

		kod_event event;
		event.type = KOD_APPEVENT;
		event.app.state = KOD_APP_INIT;
		_push_event(event);

		_thread = std::thread{ [] {

			std::chrono::system_clock::time_point a, b { std::chrono::system_clock::now() };
			const double ms_per_frame{ 1000.0 / KOD_FRAME_PER_SECOND };
			bool run{ true };

			do {
				// read inputs
				if (!_spin.test_and_set(std::memory_order_acquire)) {
					for (size_t i = 0; i < _event_size; i++)
					{
						const kod_event& event = _events[i];
						_on_event(event);
						kod_on_event(event);
					}
					_event_size = 0;
					_spin.clear(std::memory_order_release);
					_cond.notify_all();
				}

				switch (_app_state)
				{
				case KOD_APP_RESUME: {
					a = std::chrono::system_clock::now();
					std::chrono::duration<double, std::milli> work_time = a - b;
					const double work_time_count = work_time.count();
					if (work_time_count <= ms_per_frame) {
						std::chrono::duration<double, std::milli> delta_ms{
								ms_per_frame - work_time_count };
						std::this_thread::sleep_for(delta_ms);
					}
					b = std::chrono::system_clock::now();
					std::chrono::duration<double, std::milli> sleep_time{ b - a };


					kod_on_update((work_time + sleep_time).count() * 0.001);
					break;
				}
				case KOD_APP_DESTROY: {
					run = false;
					break;
				}
				default: {
					std::unique_lock<std::mutex> lock{ _mutex };
					_cond.wait(lock);
					b = std::chrono::system_clock::now();
					break;
				}
				}
			} while (run);
		} };


		std::unique_lock<std::mutex> lock{ _mutex };
		_cond.notify_all();
		_cond.wait(lock);
	}

	JNIEXPORT void JNICALL
		Java_app_kodigo_gj_pixelized_MainActivity_nativeOnResume(JNIEnv* env, jobject activity) {
		kod_event event;
		event.type = KOD_APPEVENT;
		event.app.state = KOD_APP_RESUME;
		_push_event(event);

		std::unique_lock<std::mutex> lock{ _mutex };
		_cond.notify_all();
		_cond.wait(lock);
	}

	JNIEXPORT void JNICALL
		Java_app_kodigo_gj_pixelized_MainActivity_nativeOnFocusChanged(JNIEnv* env, jobject activity,
			jboolean focus) {
		kod_event event;
		event.type = KOD_WINDOWEVENT;
		event.window.state = KOD_WINDOW_FOCUS_CHANGE;
		event.window.focus = focus;
		_push_event(event);

		std::unique_lock<std::mutex> lock{ _mutex };
		_cond.notify_all();
		_cond.wait(lock);
	}

	JNIEXPORT void JNICALL
		Java_app_kodigo_gj_pixelized_MainActivity_nativeOnPause(JNIEnv* env, jobject activity) {
		kod_event event;
		event.type = KOD_APPEVENT;
		event.app.state = KOD_APP_PAUSE;
		_push_event(event);

		std::unique_lock<std::mutex> lock{ _mutex };
		_cond.notify_all();
		_cond.wait(lock);
	}

	JNIEXPORT void JNICALL
		Java_app_kodigo_gj_pixelized_MainActivity_nativeOnDestroy(JNIEnv* env, jobject activity) {
		std::unique_lock<std::mutex> lock{ _mutex };
		kod_event event;
		event.type = KOD_APPEVENT;
		event.app.state = KOD_APP_DESTROY;
		_push_event(event);

		_cond.notify_all();
		_cond.wait(lock);
		_thread.join();
	}

	JNIEXPORT void JNICALL
		Java_app_kodigo_gj_pixelized_MainActivity_nativeSurfaceChanged(JNIEnv* env, jobject activity,
			jobject surface) {
		std::unique_lock<std::mutex> lock{ _mutex };
		_pending_window = ANativeWindow_fromSurface(env, surface);
		if (_window) {
			ANativeWindow_release(_window);
			kod_event event;
			event.type = KOD_WINDOWEVENT;
			event.window.state = KOD_WINDOW_DESTROY;
			_push_event(event);

			_cond.notify_all();
			_cond.wait(lock);
		}
		kod_event event;
		event.type = KOD_WINDOWEVENT;
		event.window.state = KOD_WINDOW_INIT;
		_push_event(event);

		_cond.notify_all();
		_cond.wait(lock);
	}

	JNIEXPORT void
		JNICALL Java_app_kodigo_gj_pixelized_MainActivity_nativeSurfaceDestroyed(JNIEnv* env,
			jobject activity,
			jobject
			surface) {
		std::unique_lock<std::mutex> lock{ _mutex };

		ANativeWindow_release(_window);
		kod_event event;
		event.type = KOD_WINDOWEVENT;
		event.window.state = KOD_WINDOW_DESTROY;
		_push_event(event);

		_cond.notify_all();
		_cond.wait(lock);
	}
	JNIEXPORT void JNICALL
		Java_app_kodigo_gj_pixelized_MainActivity_nativeOnLayout(JNIEnv* env, jobject activity,
			jint view_left,
			jint view_right, jint view_top,
			jint view_bottom, jint usable_left,
			jint usable_right, jint usable_top,
			jint usable_bottom) {
		kod_event event;
		event.type = KOD_WINDOWEVENT;
		event.window.state = KOD_WINDOW_ON_LAYOUT;
		event.window.width = view_right - view_left;
		event.window.height = view_bottom - view_top;
		event.window.pad_left = view_left - usable_left;
		event.window.pad_right = view_right - usable_right;
		event.window.pad_top = view_top - usable_top;
		event.window.pad_bottom = view_bottom - usable_bottom;
		_push_event(event);
	}
	JNIEXPORT void JNICALL
		Java_app_kodigo_gj_pixelized_MainActivity_nativeOnTouchEvent(JNIEnv* env, jobject activity,
			jint state,
			jint index, jfloat x, jfloat y,
			jfloat raw_x, jfloat raw_y) {
		kod_event event;
		event.type = KOD_TOUCHEVENT;
		event.touch = { KOD_TOUCH_DOWN, index, x, y, raw_x, raw_y };
		_push_event(event);
	}
}

bool glSwapBuffers() {
	return eglSwapBuffers(_display, _surface);
}