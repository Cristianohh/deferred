/*! @file world.cpp
 *  @author kcweicht
 *  @date 9/27/2012 10:05:46 AM
 *  @copyright Copyright (c) 2012 kcweicht. All rights reserved.
 */
#include "world.h"
#include "assert.h"

/*
 * Internal 
 */
 #define CREATE_ID(id, index) ((id << 16) | (index & 0xFFFF))
namespace {

struct IDWrapper {
    IDWrapper(EntityID e) 
    {
        id = e >> 16;
        index = e & 0xFFFF;
    }
    uint16_t id;
    uint16_t index;

    operator EntityID() { return CREATE_ID(id, index); }
};


}
/*
 * External
 */
Entity* Entity::add_component(const Component& component) {
    ComponentSystem* system = _world->_get_system(component.type());
    system->add_component(this, component);
    return this;
}
void Entity::remove_component(ComponentType type) {
    ComponentSystem* system = _world->_get_system(type);
    system->remove_component(this);
}
void Entity::activate_component(ComponentType type) {
    ComponentSystem* system = _world->_get_system(type);
    system->activate_component(this);
}
void Entity::deactivate_component(ComponentType type) {
    ComponentSystem* system = _world->_get_system(type);
    system->deactivate_component(this);
}

World::World()
    : _new_id(0)
{
    _entities.reserve(1024*16);
    for(int ii=0;ii<kNUM_COMPONENTS;++ii) {
        _systems[ii] = NULL;
    }
    add_system(new SimpleSystem<NullData>(), kNullComponent);
}
World::~World() {
    for(int ii=0;ii<kNUM_COMPONENTS;++ii) {
        if(_systems[ii])
            delete _systems[ii];
    }
}
void World::add_system(ComponentSystem* system, ComponentType type) {
    _systems[type] = system;
}
int World::is_id_valid(EntityID id) const {
    IDWrapper wrapper(id);

    if(wrapper.index < 0 || wrapper.index >= _entities.size())
        return 0;

    if(IDWrapper(_entities[wrapper.index].id()).id != 0xFFFF)
        return 1;

    return 0;
}
void World::update(float elapsed_time) {
    for(int ii=0;ii<kNUM_COMPONENTS;++ii) {
        if(_systems[ii])
            _systems[ii]->update(elapsed_time);
    }
}
ComponentSystem* World::_get_system(ComponentType type) {
    assert(type >= 0 && type <= kNUM_COMPONENTS);
    assert(_systems[type]);
    return _systems[type];
}
EntityID World::create_entity(void) {
    uint16_t index;
    uint16_t id = _new_id++;
    assert(_new_id < USHRT_MAX);

    if(_free_entities.size()) {
        index = (uint16_t)_free_entities.back();
        _free_entities.pop_back();
    } else {
        _entities.push_back(Entity());
        index = (uint16_t)_entities.size()-1;
        assert(_entities.size() < USHRT_MAX);
    }

    EntityID eid = CREATE_ID(id, index);
    _entities[index]._id = eid;
    _entities[index]._transform = TransformZero();
    _entities[index]._world = this;

    return eid;
}
Entity* World::entity(EntityID id) {
    if(!is_id_valid(id))
        return NULL;

    return &_entities[IDWrapper(id).index];
}
void World::destroy_entity(EntityID id) {
    IDWrapper wrapper(id);

    if(is_id_valid(id)) {
        wrapper.id = 0xFFFF;
        _entities[wrapper.index]._id = wrapper;
        _free_entities.push_back(wrapper.index);
    }
}
