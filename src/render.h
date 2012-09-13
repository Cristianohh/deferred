/*! @file render.h
 *  @brief TODO: Add the purpose of this module
 *  @author Kyle Weicht
 *  @date 9/13/12 1:58 PM
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 *	@addtogroup render render
 *	@{
 */
#ifndef __render_h__
#define __render_h__

class Render {
public:
    virtual ~Render() {}

    virtual void initialize(void* window) = 0;
    virtual void shutdown(void) = 0;

    virtual void render(void) = 0;

    virtual void* window(void) = 0;

    static Render* create(void);
    static void destroy(Render* render);
};

/* @} */
#endif /* include guard */
