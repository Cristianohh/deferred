/*! @file resource_manager_test.cpp
 *  @author Kyle Weicht
 *  @date 9/24/12
 *  @copyright Copyright (c) 2012 Kyle Weicht. All rights reserved.
 */

#include "unit_test.h"
#include "resource_manager.h"
#include <stdlib.h>
#include <stdio.h>

namespace {

TEST(CreateResourceManager) {
    ResourceManager manager;
    CHECK_EQUAL(0, manager.num_resources());
}
struct ResourceManagerFixture {
    ResourceManagerFixture() {
    }
    ~ResourceManagerFixture() {
    }

    ResourceManager manager;
};
int _test_loader(const char*, void* data, Resource* resource) {
    resource->i = rand();
    int* i = (int*)data;
    (*i) += resource->i;

    printf("LOADING");

    return 0;
}
void _test_unloader(Resource* resource, void* data) {
    int* i = (int*)data;
    (*i) -= resource->i;
}
TEST_FIXTURE(ResourceManagerFixture, SetLoader) {
    int test_int = 0;
    CHECK_EQUAL(0, manager.num_handlers());
    manager.add_handlers("test", _test_loader, _test_unloader, &test_int);
    CHECK_EQUAL(1, manager.num_handlers());
}
TEST_FIXTURE(ResourceManagerFixture, Load) {
    int test_int = 0;
    manager.add_handlers("test", _test_loader, _test_unloader, &test_int);
    Resource r = manager.get_resource("file.test");
    CHECK_NOT_EQUAL(kInvalidResource, r.i);
    CHECK_EQUAL(1, manager.num_resources());
}
TEST_FIXTURE(ResourceManagerFixture, Reload) {
    int test_int = 0;
    manager.add_handlers("test", _test_loader, _test_unloader, &test_int);
    Resource r1 = manager.get_resource("file.test");
    CHECK_NOT_EQUAL(kInvalidResource, r1.i);
    CHECK_EQUAL(1, manager.num_resources());
    
    Resource r2 = manager.get_resource("file2.test");
    CHECK_NOT_EQUAL(kInvalidResource, r2.i);
    CHECK_EQUAL(2, manager.num_resources());
    
    Resource r3 = manager.get_resource("file.test");
    CHECK_EQUAL(r1.i, r3.i);
    CHECK_EQUAL(2, manager.num_resources());
}
IGNORE_TEST_FIXTURE(ResourceManagerFixture, AddManualResource) {
    Resource resource;
    resource.i = 42;
    manager.add_resource(resource, "name");
    CHECK_EQUAL(1, manager.num_resources());
    
    Resource r = manager.get_resource("name");
    CHECK_EQUAL(resource.i, r.i);
    CHECK_EQUAL(1, manager.num_resources());   
}

}
