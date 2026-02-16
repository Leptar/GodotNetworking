#ifndef GAME_SERVER_H
#define GAME_SERVER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include "gn_network_manager.h" 
#include "entt_manager.h"

namespace godot {
    
    enum PacketType
    {
        JOIN = 0,
        SPAWN = 1,
    };
    
    struct ClientInfo {
        String ip;
        int port;
    };
    
    class GameServer : public Node {
        GDCLASS(GameServer, Node)

    private:
        // On garde des références vers nos outils
        class GDNetworkManager* network_manager = nullptr;
        class EnttManager* entt_manager = nullptr;

        uint32_t next_network_id = 100; 
        std::map<uint32_t, int> network_to_local; // Le Linking Context simple
        std::vector<ClientInfo> connected_clients; // Pour le broadcast
        
    protected:
        static void _bind_methods();

    public:
        GameServer();
        ~GameServer();

        void _ready() override; // Pour connecter les signaux au démarrage

        // C'est ici que la magie opère
        void _on_packet_received(const String& sender_ip, int sender_port, const PackedByteArray& data);
    };

}

#endif