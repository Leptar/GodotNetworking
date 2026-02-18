#ifndef ENTT_MANAGER_H
#define ENTT_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <entt/entt.hpp>

namespace godot {
    
    struct EntityContext {
        uint32_t network_id;
        entt::entity local_entity;
    };

    struct position
    {
        int x,y;
    };

    class EnttManager : public Node {
        GDCLASS(EnttManager, Node)

        // Une map pour retrouver l'objet local via son ID réseau
        std::unordered_map<uint32_t, EntityContext> network_to_local_map;
        entt::registry registry; 

    protected:
        static void _bind_methods();

    public:
        EnttManager();
        ~EnttManager();

        void _process(double delta) override;
    
        void create_entity(uint32_t network_id, uint32_t type_id);

    };

}

#endif