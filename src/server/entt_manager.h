#ifndef ENTT_MANAGER_H
#define ENTT_MANAGER_H

#include <chrono>
#include <game_types.h>
#include <entt/entt.hpp>


struct EntityContext {
    uint32_t network_id;
    entt::entity local_entity;
};

struct typeID {
    uint32_t type_id;
};

struct position
{
    float x,y;
};

struct speed {
    float s = 300.f;
};

struct PlayerInput {
    int last_sequence = 0;
    int next_sequence = 0;
    int current_keys = 0;
    float aim_x = 0.0f;
    float aim_y = 0.0f;
};

class EnttManager {

    // Une map pour retrouver l'objet local via son ID réseau
    std::unordered_map<uint32_t, EntityContext> network_to_local_map;
    entt::registry registry;

public:
    EnttManager();
    ~EnttManager();

    void create_entity(uint32_t network_id, uint32_t type_id);

    void destroy_entity(uint32_t network_id);

    position& get_entity_pos(uint32_t network_id);

    void update_player_input(uint32_t network_id, uint32_t sequence, uint8_t keys, float aim_x, float aim_y);

    void update(float deltatime);

    std::vector<godot::EntityState> get_world_state();
};


#endif