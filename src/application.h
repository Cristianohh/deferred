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
    kEventResize
} SystemEventType;

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
    } data;
} SystemEvent;

/*! @brief Retrives an event from the system queue */
const SystemEvent* app_pop_event(void);

void _app_push_event(SystemEvent event);

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
