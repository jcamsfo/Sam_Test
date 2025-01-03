#ifndef OPENCVGTKWINDOWCONTROLLER_HPP
#define OPENCVGTKWINDOWCONTROLLER_HPP


#include <opencv2/opencv.hpp>
// cvGetWindowHandle is only available via C API
#include <opencv2/highgui/highgui_c.h>

#include <gtk/gtk.h>

#include <string>
#include <cstdint>


#if ! defined (__linux__) || defined (__APPLE__) || defined (_WIN32)
  #error "supports OpenCV when using GTK as UI backend; expects compilation in Linux environment"
#endif

#if CV_VERSION_MAJOR < 4 || (CV_VERSION_MAJOR == 4 && CV_VERSION_MINOR < 10)
  #error "expects OpenCV version 4.10.0 or greater"
#endif

#if GTK_MAJOR_VERSION != 3 || GTK_MINOR_VERSION < 10
  #error "expects GTK version greater than 3.9 and less than 4.0"
#endif

// OpenCV highgui module API only allows modification of windows by title, not
//   by pointer or reference, and window titles can be updated, so there is no
//   guarantee from outside the API that a given title will continue to map to
//   the same window.
#ifdef _MSC_VER
  #warning "OpenCVGtkWindowController requires not renaming windows with cv::setWindowTitle or gtk_window_set_title"
#else  // __GNUC__ || __clang__
  #pragma GCC poison /*void cv::*/setWindowTitle/*(const String &winname, const String &title)*/
  #pragma GCC poison /*void */gtk_window_set_title/*(GtkWindow* window, const gchar* title)*/
#endif


class OpenCVGtkWindowController {
private:
    //
    // window indentifiers
    //
    std::string_view _title   {};
    GdkDisplay*      _display {};
    GtkWidget*       _handle  {};
    GdkWindow*       _window  {};

    //
    // cursor hiding
    //
    GdkCursor*  _cursor             {};
    GdkCursor*  _blank_cursor       {};
    bool        _cursor_hidden      {};
    bool        _auto_hiding_cursor {};
    static constexpr
    uint32_t    _DEFAULT_AUTO_HIDING_CURSOR_DELAY { 50 };  // delay in frames
    static constexpr
    uint32_t    _MAX_AUTO_HIDING_CURSOR_DELAY { 1000 };
    uint32_t    _auto_hiding_cursor_delay     { _DEFAULT_AUTO_HIDING_CURSOR_DELAY };
    uint32_t    _frames_until_hiding_cursor   { _auto_hiding_cursor_delay };
    // signature modified from icvOnMouse():
    //   - https://github.com/opencv/opencv/blob/4.x/modules/highgui/src/window_gtk.cpp#L609
    //   but it appears that g_signal_connect param GCallback in docs is
    //   generic/polymorphic, and actual signature may differ by signal:
    //   - https://docs.gtk.org/gobject/signals.html#handlers
    //   - https://docs.gtk.org/gobject/callback.Callback.html
    friend void _onMouseEvent(GtkWidget* widget, GdkEvent* event,
                              gpointer user_data);

    //
    // fullscreen toggling
    //
    bool        _fullscreen {};

public:
    OpenCVGtkWindowController() = delete;
    OpenCVGtkWindowController(const char* opencv_window_name);
    ~OpenCVGtkWindowController();

    // attach controller to new OpenCV window
    void attach(const char* opencv_window_name);

    void hide_cursor();
    void show_cursor();

    inline bool window_is_destroyed() {
        return gdk_window_is_destroyed(_window);
    }

    inline void auto_hiding_cursor_delay(const uint32_t frame_delay) {
        _auto_hiding_cursor_delay =
            (frame_delay < _MAX_AUTO_HIDING_CURSOR_DELAY) ?
            frame_delay : _MAX_AUTO_HIDING_CURSOR_DELAY;
    }

    inline uint32_t auto_hiding_cursor_delay() {
        return _auto_hiding_cursor_delay;
    }

    void enable_auto_hiding_cursor(const uint32_t frame_delay =
                                   _DEFAULT_AUTO_HIDING_CURSOR_DELAY);
    void disable_auto_hiding_cursor();
    void decrement_frames_until_hiding_cursor();

    inline uint32_t get_n_monitors() {
        return gdk_display_get_n_monitors(_display);
    }
    void fullscreen_on_monitor(const uint32_t monitor_i);
    void toggle_fullscreen();
};


#endif  // OPENCVGTKWINDOWCONTROLLER_HPP
