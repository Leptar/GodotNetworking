#include "entt_manager.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

EnttManager::EnttManager() {
    // Initialisation si nécessaire
}

EnttManager::~EnttManager() {
}

void EnttManager::_process(double delta) {
    // Ici tu mettrais tes systèmes EnTT (update physics, network, etc.)
}

void EnttManager::create_entity(uint32_t network_id, uint32_t type_id) {
    entt::entity entity = registry.create();
    registry.emplace<position>(entity, 0, 0);

    // registry.emplace<TypeID>(entity, type_id);
    
    EntityContext Context = {network_id, entity};
    network_to_local_map[network_id] = Context;
    
    UtilityFunctions::print("Entité EnTT créée avec ID: ", (int)entity);
    

}

void EnttManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("create_entity"), &EnttManager::create_entity);
}