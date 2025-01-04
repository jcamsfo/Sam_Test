#include "OpenCVGtkWindowController.hpp"

#include <exception>
#include <string>


// could not make a member function due to implicit `this` param making
//   signature not GCallback-castable
void _onMouseEvent(GtkWidget* /*widget*/, GdkEvent* /*event*/,
                   gpointer user_data) {
    if (!user_data)
        return;
    auto controller { static_cast<OpenCVGtkWindowController*>(user_data) };
    controller->showCursor();
    controller->_frames_until_hiding_cursor =
        controller->_auto_hiding_cursor_delay;
}


OpenCVGtkWindowController::OpenCVGtkWindowController(
    const char* opencv_window_name) {

    // (GTK is default UI for OpenCV when compiled on Linux, so HAVE_GTK may not
    //   be defined), see:
    //   - https://docs.opencv.org/4.x/db/d05/tutorial_config_reference.html#tutorial_config_reference_highgui
    if (cv::currentUIFramework() != "GTK3") {
        throw std::runtime_error(
            std::string("OpenCVGtkWindowController(const char*): ") +
            "OpenCV use of GTK3 backend required");
    }

    attach(opencv_window_name);
}

void OpenCVGtkWindowController::attach(const char* opencv_window_name) {
    const std::string exception_stem {
        "OpenCVGtkWindowController::attach(const char*): "
    };

    if (!opencv_window_name) {
        throw std::invalid_argument(
            exception_stem + "null opencv_window_name");
    }
    _title = std::string_view{opencv_window_name};
    _display = gdk_display_get_default();
    if (!_display) {
        throw std::runtime_error(
            exception_stem + "failed to get default GdkDisplay");
    }
    // OpenCV highgui module API only allows modification of windows by title,
    //   not by pointer or reference. Getting the handle here both gives a unique
    //   identifier for the life of the window, and provides a gateway to the
    //   GUI backend APIs, GTK/GDK in this case.
    _handle =
        static_cast<GtkWidget*>(cvGetWindowHandle(opencv_window_name));
    if (!_handle) {
        throw std::invalid_argument(
            exception_stem + "could not find OpenCV window: '" +
            opencv_window_name + '\'');
    }
    _window = gtk_widget_get_window(_handle);
    if (!_window) {
        throw std::runtime_error(
            exception_stem + "failed to get GdkWindow from GtkWidget");
    }
    _blank_cursor =
        gdk_cursor_new_for_display(_display, GDK_BLANK_CURSOR);
    if (!_blank_cursor) {
        throw std::runtime_error(
            exception_stem + "failed to create GDK_BLANK_CURSOR");
    }
}

OpenCVGtkWindowController::~OpenCVGtkWindowController() {
    disableAutoHidingCursor();
}

void OpenCVGtkWindowController::hideCursor() {
    if (windowIsDestroyed() || _cursor_hidden)
        return;
    _cursor = gdk_window_get_cursor(_window);
    gdk_window_set_cursor(_window, _blank_cursor);
    _cursor_hidden = true;
}

void OpenCVGtkWindowController::showCursor() {
    if (windowIsDestroyed() || !_cursor_hidden)
        return;
    // restore previous cursor (if nullptr, should defer to desktop default)
    gdk_window_set_cursor(_window, _cursor);
    _cursor_hidden = false;
}

void OpenCVGtkWindowController::enableAutoHidingCursor(
    const uint32_t frame_delay/* = _DEFAULT_AUTO_HIDING_CURSOR_DELAY*/) {
    autoHidingCursorDelay(frame_delay);
    if (windowIsDestroyed() || _auto_hiding_cursor)
        return;
    _frames_until_hiding_cursor = _auto_hiding_cursor_delay;
    // mimics OpenCV icvOnMouse from:
    //   - https://github.com/opencv/opencv/blob/4.10.0/modules/highgui/src/window_gtk.cpp#L1067-L1074
    // GCallback in GObject 2 docs is generic, actual signature may differ by
    //   signal. Also, unlike OpenCV which can only hold one mouse handler at a
    //   time, these should queue up behind the ones connected by icvOnMouse
    //   in the cv::namedWindow call stack:
    //   - https://docs.gtk.org/gobject/signals.html#handlers
    g_signal_connect(_handle, "button-press-event",
                     G_CALLBACK(_onMouseEvent), this);
    g_signal_connect(_handle, "button-release-event",
                     G_CALLBACK(_onMouseEvent), this);
    g_signal_connect(_handle, "motion-notify-event",
                     G_CALLBACK(_onMouseEvent), this);
    g_signal_connect(_handle, "scroll-event",
                     G_CALLBACK(_onMouseEvent), this);
    _auto_hiding_cursor = true;
}

void OpenCVGtkWindowController::disableAutoHidingCursor() {
    if (windowIsDestroyed() || !_auto_hiding_cursor)
        return;
    // both casts to void* required here to compile, even as not required by
    //   g_signal_connect
    g_signal_handlers_disconnect_by_func(_handle,
                                         (void*)_onMouseEvent, (void*)this);
    showCursor();
    _auto_hiding_cursor = false;
}

void OpenCVGtkWindowController::decrementFramesUntilHidingCursor() {
    if (windowIsDestroyed() || !_auto_hiding_cursor || _cursor_hidden)
        return;
    if (_frames_until_hiding_cursor == 0) {
        hideCursor();
        _cursor_hidden = true;
        return;
    }
    if (_frames_until_hiding_cursor == _auto_hiding_cursor_delay) {
        // mouse event has restarted delay
        showCursor();
        _cursor_hidden = false;
    }
    --_frames_until_hiding_cursor;
}

// header prohibits window retitling to prevent desync of _title and _handle
void OpenCVGtkWindowController::fullscreenOnMonitor(const uint32_t monitor_i) {
    if (windowIsDestroyed())
        return;
    const std::string exception_stem {
        "OpenCVGtkWindowController::fullscreen_on_monitor(const uint32_t): "
    };
    // default is Debian 1-indexing from left, GTK/GDK 0-indexing from left
    const uint32_t n_monitors { getNMonitors() };
    if (n_monitors == 0) {
        throw std::runtime_error(exception_stem + "no monitors detected");
    }
    if (monitor_i >= n_monitors) {
        throw std::invalid_argument(
            exception_stem + "expected monitor_i from 0 to " +
            std::to_string(n_monitors - 1));
    }
    gdk_window_fullscreen_on_monitor(_window, monitor_i);
    // need to also update OpenCV API fullscreen status
    cv::setWindowProperty(_title.data(), cv::WND_PROP_FULLSCREEN,
                          cv::WINDOW_FULLSCREEN);
    _fullscreen = true;
}

// header prohibits window retitling to prevent desync of _title and _handle
void OpenCVGtkWindowController::toggleFullscreen() {
    if (windowIsDestroyed())
        return;
    // similar to fullscreen_on_monitor, we face a tradeoff of needing to
    //   keep the OpenCV window state synced with the underlying Gtk/GdkWindow,
    //   versus the advantages of modifying windows by handle not title
    if (!_fullscreen) {
        //gdk_window_fullscreen(_window);
        cv::setWindowProperty(_title.data(), cv::WND_PROP_FULLSCREEN,
                              cv::WINDOW_FULLSCREEN);
    } else {
        //gdk_window_unfullscreen(_window);
        cv::setWindowProperty(_title.data(), cv::WND_PROP_FULLSCREEN,
                              cv::WINDOW_NORMAL);
    }
    _fullscreen = !_fullscreen;
}
