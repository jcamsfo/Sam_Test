#ifndef OPENCVGTKWINDOWCONTROLLER_HPP
#define OPENCVGTKWINDOWCONTROLLER_HPP

/**
 * @file OpenCVGtkWindowController.hpp
 */

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

/**
 * @file OpenCVGtkWindowController.hpp
 *
 * @note
 * OpenCV highgui module API only allows modification of windows by title, not
 *   by pointer or reference (see warning for method [attach](#attach)). Some
 *   protection is implemented in this header by using [GCC poison] to prohibit
 *   using the functions [cv::setWindowTitle] and [gtk_window_set_title].
 *
 *   [GCC poison]: https://gcc.gnu.org/onlinedocs/cpp/Pragmas.html#index-_0023pragma-GCC-poison
 *   [cv::setWindowTitle]: https://docs.opencv.org/4.10.0/d7/dfc/group__highgui.html#ga56f8849295fd10d0c319724ddb773d96
 *   [gtk_window_set_title]: https://docs.gtk.org/gtk3/method.Window.set_title.html
 */
#ifdef _MSC_VER
  #warning "OpenCVGtkWindowController requires not renaming windows with cv::setWindowTitle or gtk_window_set_title"
#else  // __GNUC__ || __clang__
  #pragma GCC poison /*void cv::*/setWindowTitle/*(const String &winname, const String &title)*/
  #pragma GCC poison /*void */gtk_window_set_title/*(GtkWindow* window, const gchar* title)*/
#endif

/**
 * @brief Provides more fine-grained control of a [highgui] window in [OpenCV]
 *   when it is using [GTK3] as the UI framework.
 *
 * [OpenCV]'s [highgui] module can use one of several UI frameworks (testable
 *   with [cv::currentUIFramework] in v4.10+.) When [highgui] is using [GTK3],
 *   often the default when [OpenCV] is built for Linux, this class leverages
 *   the [GTK3]/[GDK3] API to provide window control not available via the
 *   [highgui] API.
 *
 * [OpenCV]: https://docs.opencv.org/4.x/index.html
 * [highgui]: https://docs.opencv.org/4.x/d7/dfc/group__highgui.html
 * [cv::currentUIFramework]: https://docs.opencv.org/4.x/d7/dfc/group__highgui.html#ga8dda596f9d75991b9abe811c438e24e3
 * [GTK3]: https://docs.gtk.org/gtk3/
 * [GDK3]: https://docs.gtk.org/gdk3/
 */
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

    /**
     * @brief [GCallback] function used to update OpenCVGtkWindowController state
     *   on GTK/GDK mouse events.
     *
     * @param[in]     widget    GTK entity that experienced the event
     * @param[in]     event     GTK event data
     * @param[in,out] user_data `void*` to contain user data modified during event handling
     *
     * @note
     * Could not be a OpenCVGtkWindowController member function due to
     *   an implicit `this` param making the signature not [GCallback]-castable.
     *   The signature used here is modified from [icvOnMouse], but it appears
     *   that `g_signal_connect` [GCallback] param in docs is
     *   generic/polymorphic, and actual signature may [differ by signal].
     *
     * [GCallback]: https://docs.gtk.org/gobject/callback.Callback.html
     * [icvOnMouse]: https://github.com/opencv/opencv/blob/4.10.0/modules/highgui/src/window_gtk.cpp#L607
     * [differ by signal]: https://docs.gtk.org/gobject/signals.html#handlers
     */
    friend void _onMouseEvent(GtkWidget* widget, GdkEvent* event,
                              gpointer user_data);

    //
    // fullscreen toggling
    //
    bool        _fullscreen {};

public:

    OpenCVGtkWindowController() = delete;

    /**
     * @brief Default constructor (cannot instantiate without window to
     *   [attach](#attach)).
     *
     * @param[in] opencv_window_name C-string title of OpenCV window
     * @see [attach](#attach)
     */
    OpenCVGtkWindowController(const char* opencv_window_name);

    /**
     * @brief Default destructor.
     */
    ~OpenCVGtkWindowController();

    /**
     * @brief Grants this controller instance access to an OpenCV highgui
     *   window via its "handle," or GUI framework representation.
     *
     * @param[in] opencv_window_name C-string title of OpenCV window
     *
     * @warning
     * OpenCV highgui module API only allows modification of windows by
     *   title, not by pointer or reference, and window titles can be updated,
     *   so there is no guarantee that a given title will continue to map to
     *   the same window.
     */
    void attach(const char* opencv_window_name);

    /**
     * @brief Checks if current attached window has been deallocated.
     *
     * @return `true` if current attached window has been deallocated
     */
    inline bool windowIsDestroyed() {
        return gdk_window_is_destroyed(_window);
    }

    /**
     * @brief Makes current mouse pointer invisible while over attached window.
     *
     * @ingroup cursor_hiding
     */
    void hideCursor();

    /**
     * @brief Restores visibility of current mouse while over attached window.
     *
     * @ingroup cursor_hiding
     */
    void showCursor();

    /**
     * @brief Auto-hiding cursor delay setter.
     *
     * @param[in] frame_delay new delay (in frames)
     *
     * @ingroup cursor_hiding
     */
    inline void autoHidingCursorDelay(const uint32_t frame_delay) {
        _auto_hiding_cursor_delay =
            (frame_delay < _MAX_AUTO_HIDING_CURSOR_DELAY) ?
            frame_delay : _MAX_AUTO_HIDING_CURSOR_DELAY;
    }

    /**
     * @brief Auto-hiding cursor delay getter.
     *
     * @return current auto-hiding cursor delay (in frames)
     *
     * @ingroup cursor_hiding
     */
    inline uint32_t autoHidingCursorDelay() {
        return _auto_hiding_cursor_delay;
    }

    /**
     * @brief Toggles on cursor auto-hiding.
     *
     * @param[in] frame_delay delay (in frames)
     *
     * @ingroup cursor_hiding
     */
    void enableAutoHidingCursor(const uint32_t frame_delay =
                                _DEFAULT_AUTO_HIDING_CURSOR_DELAY);
    /**
     * @brief Toggles off cursor auto-hiding.
     *
     * @ingroup cursor_hiding
     */
    void disableAutoHidingCursor();

    /**
     * @brief Increments state of auto-hiding cursor by one frame.
     *
     * @ingroup cursor_hiding
     */
    void decrementFramesUntilHidingCursor();

    /**
     * @brief Gets number of monitors currently visible to display manager.
     *
     * @return number of monitors
     *
     * @ingroup fullscreen_toggling
     */
    inline uint32_t getNMonitors() {
        return gdk_display_get_n_monitors(_display);
    }

    /**
     * @brief Sets window to fullscreen on a monitor.
     *
     * @param monitor_i system monitor index (starts at 0)
     *
     * @ingroup fullscreen_toggling
     */
    void fullscreenOnMonitor(const uint32_t monitor_i);

    /**
     * @brief Toggles fullscreen state of window on current monitor.
     *
     * @ingroup fullscreen_toggling
     */
    void toggleFullscreen();
};


#endif  // OPENCVGTKWINDOWCONTROLLER_HPP
