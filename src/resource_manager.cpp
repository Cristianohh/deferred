/*! @file resource_manager.cpp
 *  @author Kyle Weicht
 *  @date 9/24/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#include "resource_manager.h"
#include <string.h>

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
    : _num_resources(0)
    , _num_handlers(0)
    , _free_resource_index(MAX_RESOURCES-1)
{
    for(int ii=0;ii<MAX_RESOURCES;++ii)
        _free_resources[ii] = (MAX_RESOURCES-1)-ii;
}
ResourceManager::~ResourceManager() {
}

void ResourceManager::add_handlers(const char* extension,
                                   ResourceLoader* loader,
                                   ResourceUnloader* unloader,
                                   void* user_data)
{
    int index = _num_handlers++;
    strlcat(_handlers[index].ext, extension, sizeof(_handlers[index].ext));
    _handlers[index].loader = loader;
    _handlers[index].unloader = unloader;
    _handlers[index].ud = user_data;
}
ResourceID ResourceManager::load(const char* filename) {
    int resource_index = _free_resources[_free_resource_index--];
    char lower_filename[256];
    _to_lower(lower_filename, filename, sizeof(lower_filename));

    _num_resources++;
    return resource_index;
}
