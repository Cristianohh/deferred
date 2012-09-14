/*! @file assert.h
 *  @brief Assert
 *  @author kcweicht
 *  @date 9/14/2012 2:00:01 PM
 *  @copyright Copyright (c) 2012 kcweicht. All rights reserved.
 */
#ifndef __assert_h__
#define __assert_h__

/*! @brief Breaks into the debugger
 */
#if defined( _MSC_VER )
    #define debugBreak() __debugbreak()
    #pragma warning(disable:4127) /* Conditional expression is constant (the do-while) */
#elif defined( __GNUC__ )
    #define debugBreak() __asm__( "int $3\n" : : )
#else
    #error Unsupported compiler
#endif

/*! @brief Assert
 */
#ifndef assert
    #define assert(condition)   \
        do {                    \
            if(!(condition)) {  \
                debugBreak();   \
            }                   \
        } while(__LINE__ == -1)
#endif
#ifndef ASSERT
    #define ASSERT assert
#endif

#endif /* include guard */
