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
#include <map>
#include <queue>
#include "vec_math.h"

typedef uint32_t EntityID;
class World;
class Entity;
class Component;

enum ComponentType {
   kNullComponent,
   kTestComponent,
   kRenderComponent,
   kLightComponent,

   kNUM_COMPONENTS
};



class Entity {
public:
    Entity() { }
    ~Entity() { }

    Entity* add_component(const Component& component);
    void remove_component(ComponentType type);
    void activate_component(ComponentType type);
    void deactivate_component(ComponentType type);

    EntityID id(void) const { return _id; }
    Transform transform(void) const { return _transform; }
    World* world(void) const { return _world; }

    Entity* set_transform(const Transform& transform) { _transform = transform; return this; }

private:
    friend class World;
    friend class ComponentSystem;
    template<class T> friend class SimpleSystem;

    Transform   _transform;
    World*      _world;
    EntityID    _id;
};

class Component {
public:
    virtual ~Component() { }
    virtual void* data(void) const = 0;
    virtual ComponentType type(void) const = 0;
};

template<class T, ComponentType Type>
class SimpleComponent : public Component {
public:
    SimpleComponent(T t) 
        : _t(t)
    {
    }
    void* data(void) const { return (void*)&_t; }
    ComponentType type(void) const { return Type; }

    T _t;
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


struct NullData
{
    NullData() : t(0.0f) { }
    NullData(float _t) : t(_t) { }
    float t;
};

typedef SimpleComponent<NullData, kNullComponent> NullComponent;

template<typename T>
class SimpleSystem : public ComponentSystem {
public:
    SimpleSystem() { }
    ~SimpleSystem() { }

    void update(float elapsed_time) {
        typename std::map<Entity*,std::pair<bool,T> >::iterator iter = _components.begin();
        while(iter != _components.end()) {
            Entity* e = iter->first;
            if(iter->second.first == true)
            {
                _update(e, &iter->second.second, elapsed_time);
            }
            ++iter;
        }
    }
    void _update(Entity* entity, T* data, float elapsed_time) {
        entity->_transform.position.y += elapsed_time*data->t;
    }
    void add_component(Entity* entity,const Component& component) {
        const T* data = (T*)component.data();
        typename std::map<Entity*,std::pair<bool,T> >::iterator iter = _components.find(entity);
        if(iter == _components.end()) {
            _components[entity] = std::make_pair(true,*data);
        }
    }
    void remove_component(Entity* entity) {
        typename std::map<Entity*,std::pair<bool,T> >::iterator iter = _components.find(entity);
        if(iter != _components.end())
            _components.erase(iter);
    }
    void activate_component(Entity* entity) {
        typename std::map<Entity*,std::pair<bool,T> >::iterator iter = _components.find(entity);
        if(iter != _components.end())
            iter->second.first = true;
    }
    void deactivate_component(Entity* entity) { 
        typename std::map<Entity*,std::pair<bool,T> >::iterator iter = _components.find(entity);
        if(iter != _components.end())
            iter->second.first = false;
    }

private:
    std::map<Entity*,std::pair<bool,T> >  _components;
};

class World {
public:
    World();
    ~World();

    void add_system(ComponentSystem* system, ComponentType type);

    EntityID create_entity(void);
    void destroy_entity(EntityID id);

    void update(float elapsed_time);

    Entity* entity(EntityID id);

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
