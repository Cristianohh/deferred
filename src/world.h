/*! @file world.h
 *  @brief TODO: Add the purpose of this module
 *  @author kcweicht
 *  @date 9/27/2012 10:05:46 AM
 *  @copyright Copyright (c) 2012 kcweicht. All rights reserved.
 */
#ifndef __world_h__
#define __world_h__

#include <stdint.h>
#include <vector>
#include <hash_map>
#include "vec_math.h"

typedef uint32_t EntityID;
class World;
class Entity;
class Component;

enum ComponentType {
   kNullComponent,

   kNUM_COMPONENTS
};



class Entity {
public:
    Entity() { }
    ~Entity() { }

    Entity& add_component(const Component& component);
    void remove_component(ComponentType type);
    void activate_component(ComponentType type);
    void deactivate_component(ComponentType type);

    EntityID id(void) const { return _id; }
    Transform transform(void) const { return _transform; }
    World* world(void) const { return _world; }

private:
    friend class World;
    friend class ComponentSystem;
    friend class NullSystem;

    Transform   _transform;
    World*      _world;
    EntityID    _id;
};

class Component {
public:
    virtual ComponentType type(void) const = 0;
};
class NullComponent : public Component {
public:
    NullComponent(float t) 
        : _t(t)
    {
    }
    ComponentType type(void) const { return kNullComponent; }

    float _t;
};

class ComponentSystem {
public:
    virtual ~ComponentSystem() {}
    virtual void update(float) { }
    virtual void add_component(Entity*,const Component&) {}
    virtual void remove_component(Entity*) { }
    virtual void activate_component(Entity*) { }
    virtual void deactivate_component(Entity*) { }
};

class NullSystem : public ComponentSystem {
public:
    void update(float elapsed_time) {
        _iter = _components.begin();
        while(_iter != _components.end()) {
            Entity* e = _iter->first;
            if(_iter->second.first == true)
                e->_transform.position.y += elapsed_time*_iter->second.second.t;

            ++_iter;
        }
    }
    void add_component(Entity* entity,const Component& component) {
        _iter = _components.find(entity);
        if(_iter == _components.end()) {
            NullData data = { ((const NullComponent&)component)._t };
            _components[entity] = std::make_pair(true,data);
        }
    }
    void remove_component(Entity* entity) {
        _iter = _components.find(entity);
        if(_iter != _components.end())
            _components.erase(_iter);
    }
    void activate_component(Entity* entity) {
        _iter = _components.find(entity);
        if(_iter != _components.end())
            _iter->second.first = true;
    }
    void deactivate_component(Entity* entity) { 
        _iter = _components.find(entity);
        if(_iter != _components.end())
            _iter->second.first = false;
    }

private:
    struct NullData
    {
        float t;
    };
    std::hash_map<Entity*,std::pair<bool,NullData>>             _components;
    std::hash_map<Entity*,std::pair<bool,NullData>>::iterator   _iter;
};

class World {
public:
    World();
    ~World();

    EntityID create_entity(void);
    void destroy_entity(EntityID id);

    void update(float elapsed_time);

    Entity* get_entity(EntityID id);

    int is_id_valid(EntityID id) const;
    int num_entities(void) const { return (int)(_entities.size() - _free_entities.size()); }

private:
    ComponentSystem* _get_system(ComponentType type);
    
private:
    friend class Entity;

    ComponentSystem*    _systems[kNUM_COMPONENTS];
    std::vector<Entity> _entities;
    std::vector<int>    _free_entities;

    uint16_t _new_id;
};

#endif /* include guard */
