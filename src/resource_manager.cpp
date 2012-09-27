/*! @file resource_manager.cpp
 *  @author Kyle Weicht
 *  @date 9/24/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#include "resource_manager.h"
#include <string.h>
#include <stdio.h>
#include "application.h"

namespace {

void _to_lower(char* dest, const char* source, size_t size) {
    if(!dest || !source)
        return;
    for(int ii=0;ii<(int)size;++ii) {
        char c = source[ii];
        if(c >= 'A' && c <= 'Z') {
            c += ('a' - 'A');
        }
        dest[ii] = c;
        if(c == '\0')
            break;
    }
    dest[size-1] = '\0';
}

}

ResourceManager::ResourceManager()
{
}
ResourceManager::~ResourceManager() {
}

void ResourceManager::add_handlers(const char* extension,
                                   ResourceLoader* loader,
                                   ResourceUnloader* unloader,
                                   void* user_data)
{
    ResourceHandler h = { loader, unloader, user_data };
    char lower_extension[32];
    _to_lower(lower_extension, extension, sizeof(lower_extension));
    if(_handlers.find(lower_extension) != _handlers.end())
        _handlers[lower_extension] = h;
}
Resource ResourceManager::get_resource(const char* name) {
    char lower_name[256];
    _to_lower(lower_name, name, sizeof(lower_name));

    // Get the extension
    const char* extension = lower_name+strlen(lower_name);
    while(*extension != '.')
        --extension;
    ++extension;

    // Check to see if its already loaded
    if(_resources.find(lower_name) != _resources.end())
        return _resources[lower_name];

    ResourceHandler handler = _handlers[extension];
    Resource resource = {0};
    int result = handler.loader(name, handler.ud, &resource);
    if(result == 0)
        _resources[lower_name] = resource;

    return resource;
}
void ResourceManager::add_resource(Resource resource, const char* name) {
    char lower_name[256];
    _to_lower(lower_name, name, sizeof(lower_name));
    
    // Check to see if its already loaded
    if(_resources.find(lower_name) == _resources.end())
        _resources[lower_name] = resource;
}
