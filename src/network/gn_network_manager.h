#pragma once

#include <godot_cpp/classes/node.hpp>

namespace godot {
    class GDNetworkManager : public Node {
        GDCLASS(GDNetworkManager, Node)

    protected:
        static void _bind_methods();

    public:
        GDNetworkManager();
        ~GDNetworkManager();

        void _process(double delta) override;
    };
}