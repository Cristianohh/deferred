/*! @file resource_manager.h
 *  @author Kyle Weicht
 *  @date 9/24/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#ifndef __resource_manager__
#define __resource_manager__

#include <stddef.h>
#include <stdint.h>

union Resource {
    void*       ptr;
    intptr_t    i;
};

typedef int (ResourceLoader)(const char* filename, void* ud, Resource* resource);
typedef void (ResourceUnloader)(Resource* resource, void* ud);
typedef int ResourceID;

enum {
    kInvalidResource = -1
};

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    int num_resources() const { return _num_resources; }
    int num_handlers() const { return _num_handlers; }
    void add_handlers(const char* extension,
                      ResourceLoader* loader,
                      ResourceUnloader* unloader,
                      void* user_data);
    ResourceID load(const char* filename);

private:
    enum { MAX_HANDLERS = 64, MAX_RESOURCES = 1024 };
    struct {
        char                ext[8];
        ResourceLoader*     loader;
        ResourceUnloader*   unloader;
        void*               ud;
    }   _handlers[MAX_HANDLERS];
    struct {
        char        filename[256-sizeof(Resource)];
        Resource    resource;
    }   _resources[MAX_RESOURCES];
    int _free_resources[MAX_RESOURCES];
    int _free_resource_index;
    int _num_resources;
    int _num_handlers;
};

#endif /* Include guard */
