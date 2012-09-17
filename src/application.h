/*! @file application.h
 *  @brief Interface with the system
 *  @author Kyle Weicht
 *  @date 9/13/12 12:39 PM
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 *	@addtogroup application application
 *	@{
 */
#ifndef __application_h__
#define __application_h__

#ifdef __cplusplus
extern "C" { /* Use C linkage */
#endif

/*! @brief Main entrypoint for the application
 *  @details This should be the only thing called from `main`. This is in charge
 *    of initializing the application for the specific OS/device. User code
 *    should all be placed in the various `on_XXXXX` handlers.
 *  @code
 *    int main(int argc, const char* argv[]) {
 *        return ApplicationMain(argc,argv);
 *    }
 *  @endcode
 */
int ApplicationMain(int argc, const char* argv[]);

/* User callbacks */
extern int on_init(int argc, const char* argv[]);
extern int on_frame(void);
extern void on_shutdown(void);

typedef enum {
    kEventResize,
    kEventKeyDown
} SystemEventType;

typedef enum {
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    
    KEY_SPACE,
    
    KEY_ESCAPE,
    KEY_SHIFT,
    KEY_CTRL,
    KEY_ALT,

    KEY_SYS, /* Windows/Command key */
    
    KEY_UP,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_LEFT,
    
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    
    KEY_MAX_KEYS
} Key;

typedef struct {
    SystemEventType type;
    union {
        struct {
            int x;
            int y;
        } mouse;
        struct {
            int width;
            int height;
        } resize;
        Key key;
    } data;
} SystemEvent;

/*! @brief Retrives an event from the system queue */
const SystemEvent* app_pop_event(void);

void _app_push_event(SystemEvent event);

int app_is_key_down(Key key);

/*! @brief Gets the OS-specific window object
 *  @details This returns the NSWindow* in OS X and the HWND in Windows
 *  @return The window
 */
void* app_get_window(void);

/*! @brief Output string to debug output */
void debug_output(const char* format, ...);

typedef enum {
    kMBOK,
    kMBRetry,
    kMBCancel
} MessageBoxResult;
/*! @brief Displays a message box to the user */
MessageBoxResult message_box(const char* header, const char* message);

#ifdef __cplusplus
} // extern "C" {
#endif

/* @} */
#endif /* include guard */
