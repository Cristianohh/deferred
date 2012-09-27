/*! @file resource_manager.cpp
 *  @author Kyle Weicht
 *  @date 9/24/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#include "resource_manager.h"
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include "application.h"

namespace {

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
    std::string lower_extension(extension);
    std::transform(lower_extension.begin(), lower_extension.end(), lower_extension.begin(), ::tolower);
    
    if(_handlers.find(lower_extension) == _handlers.end())
        _handlers[lower_extension] = h;
}
Resource ResourceManager::get_resource(const char* name) {
    std::string lower_name(name);
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    // Check to see if its already loaded
    if(_resources.find(lower_name) != _resources.end())
        return _resources[lower_name];

    
    // Get the extension
    std::string extension = lower_name.substr(lower_name.find_last_of('.')+1);

    // See if there's a handler
    if(_handlers.find(extension) == _handlers.end())
        return kInvalidResource;

    ResourceHandler handler = _handlers[extension];
    Resource resource = { 0 };
    int result = handler.loader(name, handler.ud, &resource);
    if(result == 0)
        _resources[lower_name] = resource;
    else
        resource = kInvalidResource;

    return resource;
}
void ResourceManager::add_resource(Resource resource, const char* name) {
    std::string lower_name(name);
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    // Check to see if its already loaded
    if(_resources.find(lower_name) == _resources.end())
        _resources[lower_name] = resource;
}
