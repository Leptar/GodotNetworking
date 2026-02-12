#ifndef ENTT_MANAGER_H
#define ENTT_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <entt/entt.hpp> // L'include magique

namespace godot {

    class EnttManager : public Node {
        GDCLASS(EnttManager, Node)

    private:
        entt::registry registry; // Le coeur de EnTT

    protected:
        static void _bind_methods();

    public:
        EnttManager();
        ~EnttManager();

        void _process(double delta) override;
    
        // Exemple : Créer une entité depuis GDScript
        int create_entity();
    };

}

#endif