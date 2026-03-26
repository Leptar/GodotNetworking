#include "entt_manager.h"

#include "game_types.h"


EnttManager::EnttManager() {

}

EnttManager::~EnttManager() {

}

void EnttManager::create_entity(uint32_t network_id, uint32_t type_id) {
    entt::entity entity = registry.create();
    registry.emplace<position>(entity, 0.f, 0.f);
    registry.emplace<typeID>(entity, (int)type_id);
    registry.emplace<PlayerInput>(entity, 0, 0, 0.0f, 0.0f);
    registry.emplace<speed>(entity, 300.f);

    EntityContext Context = {network_id, entity};
    network_to_local_map[network_id] = Context;

}

void EnttManager::destroy_entity(uint32_t network_id) {
    auto it = network_to_local_map.find(network_id);
    if (it != network_to_local_map.end()) {
        registry.destroy(it->second.local_entity); // Détruit l'entité dans EnTT
        network_to_local_map.erase(it);            // Retire l'ID de la map
    }
}

void EnttManager::update_player_input(uint32_t network_id, uint32_t sequence, uint8_t keys, float aim_x, float aim_y) {
    auto it = network_to_local_map.find(network_id);
    if (it == network_to_local_map.end()) {
        return;
    }

    PlayerInput& input = registry.get<PlayerInput>(it->second.local_entity);
    if (input.last_sequence >= sequence) {
        return;
    }

    input.last_sequence = sequence;
    input.current_keys = keys;
    input.aim_x = aim_x;
    input.aim_y = aim_y;

}

void EnttManager::update(float deltatime) {
    auto view = registry.view<PlayerInput, position, speed>();

    for (auto entity : view) {
        PlayerInput& input = view.get<PlayerInput>(entity);
        position& pos = view.get<position>(entity);
        speed& entity_speed = view.get<speed>(entity);


        if (input.current_keys & 1 << 0) {
            pos.y -= entity_speed.s * deltatime; // je nai pas de delta et la speed peut etre definis quelque part
        }
        if (input.current_keys & 1 << 1) {
            pos.y += entity_speed.s * deltatime;
        }
        if (input.current_keys & 1 << 2) {
            pos.x -= entity_speed.s * deltatime;
        }
        if (input.current_keys & 1 << 3) {
            pos.x += entity_speed.s * deltatime;
        }
    }
}


