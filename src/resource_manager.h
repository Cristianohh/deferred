/*! @file resource_manager.h
 *  @author Kyle Weicht
 *  @date 9/24/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 *  @todo Resource unloading
 *        Retrying
 */

#ifndef __resource_manager__
#define __resource_manager__

#include <stddef.h>
#include <stdint.h>
#include <map>
#include <string>

union Resource {
    void*       ptr;
    intptr_t    i;
};

typedef int (ResourceLoader)(const char* filename, void* ud, Resource* resource);
typedef void (ResourceUnloader)(Resource* resource, void* ud);

#define kInvalidResource 0xFFFFFFFFFFFFFFFF

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    int num_resources() const { return (int)_resources.size(); }
    int num_handlers() const { return (int)_handlers.size(); }
    void add_handlers(const char* extension,
                      ResourceLoader* loader,
                      ResourceUnloader* unloader,
                      void* user_data);
    Resource get_resource(const char* name);
    void add_resource(Resource resource, const char* name);

private:
    struct ResourceHandler
    {
        ResourceLoader*     loader;
        ResourceUnloader*   unloader;
        void*               ud;
    };
    
    std::map<std::string, ResourceHandler>  _handlers;
    std::map<std::string, Resource>         _resources;
};

#endif /* Include guard */
