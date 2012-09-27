/*! @file world_test.cpp
 *  @author kcweicht
 *  @date 9/27/2012 9:59:53 AM
 *  @copyright Copyright (c) 2012 kcweicht. All rights reserved.
 *
 *  *Create world
 *  *Create entity
 *  *Destroy entity
 *  *Get entity
 *  *Add component to entity
 *  *Remove component from entity
 *  *Update world
 *  Modify component data
 *  Multiple components per entity
 *  *Multiple entities with components
 *  Entity communication
 *  Custom components
 */
#include "unit_test.h"
#include "world.h"

namespace {

struct WorldFixture {
    WorldFixture() {
    }
    ~WorldFixture() {
    }

    World world;
};

TEST(CreateWorld)
{
    World world;
}
TEST_FIXTURE(WorldFixture, CreateEntity)
{
    EntityID id = world.create_entity();
    CHECK_EQUAL(1, world.num_entities());
    Entity* e = world.entity(id);
    CHECK_NOT_NULL(e);
    CHECK_EQUAL(0, e->id());

    CHECK_EQUAL_POINTER(&world, e->world());
}
TEST_FIXTURE(WorldFixture, InvalidID)
{
    Entity* e = world.entity(0);
    CHECK_NULL(e);
}
TEST_FIXTURE(WorldFixture, DestroyEntity)
{
    EntityID id = world.create_entity();
    Entity* e = world.entity(id);
    CHECK_EQUAL(1, world.num_entities());
    CHECK_NOT_NULL(e);
    world.destroy_entity(id);
    CHECK_EQUAL(0, world.num_entities());
}
TEST_FIXTURE(WorldFixture, NewNotOld)
{
    EntityID id_1 = world.create_entity();
    world.destroy_entity(id_1);
    CHECK_EQUAL(0, world.num_entities());

    EntityID id_2 = world.create_entity();
    CHECK_NOT_EQUAL(id_1, id_2);
    CHECK_EQUAL(1, world.num_entities());
    
    EntityID id_3 = world.create_entity();
    CHECK_NOT_EQUAL(id_2, id_3);
    CHECK_EQUAL(2, world.num_entities());
    world.destroy_entity(id_3);
    CHECK_EQUAL(1, world.num_entities());

    EntityID id_4 = world.create_entity();
    CHECK_NOT_EQUAL(id_1, id_4);
    CHECK_NOT_EQUAL(id_2, id_4);
    CHECK_NOT_EQUAL(id_3, id_4);
}
TEST_FIXTURE(WorldFixture, GetEntity)
{
    EntityID id1 = world.create_entity();
    Entity* e1 = world.entity(id1);
    CHECK_NOT_NULL(e1);
    CHECK_EQUAL(id1, e1->id());

    EntityID id2 = world.create_entity();
    Entity* e2 = world.entity(id2);
    CHECK_NOT_NULL(e2);
    CHECK_EQUAL(id2, e2->id());
    CHECK_NOT_EQUAL_POINTER(e2, e1);
    CHECK_NOT_EQUAL(id1, id2);

    world.destroy_entity(id2);
    e2 = world.entity(id2);
    CHECK_NULL(e2);
}
TEST_FIXTURE(WorldFixture, UpdateWorld)
{
    world.update(0.0f);
}
TEST_FIXTURE(WorldFixture, AddComponent)
{
    EntityID id = world.create_entity();
    Entity* e = world.entity(id);
    e->add_component(NullComponent(3.0f));
    CHECK_EQUAL_FLOAT(0.0f, e->transform().position.y);
    world.update(0.5f);
    CHECK_EQUAL_FLOAT(1.5f, e->transform().position.y);
    world.update(0.75f);
    CHECK_EQUAL_FLOAT(1.5f+(0.75f*3.0f), e->transform().position.y);
}
TEST_FIXTURE(WorldFixture, RemoveComponent)
{
    EntityID id = world.create_entity();
    Entity* e = world.entity(id);
    e->add_component(NullComponent(3.0f));
    CHECK_EQUAL_FLOAT(0.0f, e->transform().position.y);
    world.update(0.5f);
    CHECK_EQUAL_FLOAT(1.5f, e->transform().position.y);
    e->remove_component(kNullComponent);
    world.update(0.5f);
    CHECK_EQUAL_FLOAT(1.5f, e->transform().position.y);
    e->remove_component(kNullComponent);
}
TEST_FIXTURE(WorldFixture, DeactivateComponent)
{
    EntityID id = world.create_entity();
    Entity* e = world.entity(id);
    e->add_component(NullComponent(3.0f));
    CHECK_EQUAL_FLOAT(0.0f, e->transform().position.y);
    world.update(0.5f);
    CHECK_EQUAL_FLOAT(1.5f, e->transform().position.y);
    e->deactivate_component(kNullComponent);
    world.update(0.5f);
    CHECK_EQUAL_FLOAT(1.5f, e->transform().position.y);
    e->activate_component(kNullComponent);
    world.update(0.5f);
    CHECK_EQUAL_FLOAT(3.0f, e->transform().position.y);
}
TEST_FIXTURE(WorldFixture, MultipleEntities)
{
    EntityID id1 = world.create_entity();
    EntityID id2 = world.create_entity();
    world.entity(id1)->add_component(NullComponent(3.0f));
    world.entity(id2)->add_component(NullComponent(5.0f));

    world.update(0.5f);
    CHECK_EQUAL_FLOAT(1.5f, world.entity(id1)->transform().position.y);
    CHECK_EQUAL_FLOAT(2.5f, world.entity(id2)->transform().position.y);
    world.entity(id2)->deactivate_component(kNullComponent);
    world.update(0.5f);
    CHECK_EQUAL_FLOAT(3.0f, world.entity(id1)->transform().position.y);
    CHECK_EQUAL_FLOAT(2.5f, world.entity(id2)->transform().position.y);
}
IGNORE_TEST_FIXTURE(WorldFixture, EntityCommunication)
{
    EntityID id1 = world.create_entity();
    EntityID id2 = world.create_entity();
    world.entity(id1)->add_component(NullComponent(3.0f));
    world.entity(id2)->add_component(NullComponent(5.0f));

    world.update(0.5f);
    CHECK_EQUAL_FLOAT(1.5f, world.entity(id1)->transform().position.y);
    CHECK_EQUAL_FLOAT(2.5f, world.entity(id2)->transform().position.y);

    world.update(0.5f);
    CHECK_EQUAL_FLOAT(3.0f, world.entity(id1)->transform().position.y);
    CHECK_EQUAL_FLOAT(5.0f, world.entity(id2)->transform().position.y);
}
struct TestData {
    TestData()
        : x(0)
        , y(0)
    {
    }
    TestData(float _x, float _y)
        : x(_x)
        , y(_y)
    {
    }
    float x;
    float y;
};
//typedef SimpleComponent<TestData, kTestComponent> TestComponent;
//typedef SimpleSystem<TestData> TestSystem;

IGNORE_TEST_FIXTURE(WorldFixture, CustomType)
{
    EntityID id1 = world.create_entity();
    EntityID id2 = world.create_entity();
    world.entity(id1)->add_component(NullComponent(3.0f));
    world.entity(id2)->add_component(NullComponent(5.0f));

    world.update(0.5f);
    CHECK_EQUAL_FLOAT(1.5f, world.entity(id1)->transform().position.y);
    CHECK_EQUAL_FLOAT(2.5f, world.entity(id2)->transform().position.y);
    world.entity(id2)->deactivate_component(kNullComponent);
    world.update(0.5f);
    CHECK_EQUAL_FLOAT(3.0f, world.entity(id1)->transform().position.y);
    CHECK_EQUAL_FLOAT(2.5f, world.entity(id2)->transform().position.y);
}

}
