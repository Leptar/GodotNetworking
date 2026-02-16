#include "game_server.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void GameServer::_bind_methods() {
    // On rend la fonction accessible pour les signaux
    ClassDB::bind_method(D_METHOD("_on_packet_received", "sender_ip", "sender_port", "data"), &GameServer::_on_packet_received);
}

GameServer::GameServer() {}
GameServer::~GameServer() {}

void GameServer::_ready() {
    // On suppose que NetworkManager est un enfant de ce noeud dans la scène Godot
    network_manager = Object::cast_to<GDNetworkManager>(get_node_or_null("GDNetworkManager"));
    
    if (network_manager) {
        // On connecte le signal du NetworkManager à notre fonction
        network_manager->connect("packet_received", Callable(this, "_on_packet_received"));
        
        // On lance l'écoute sur le port (ex: 4242)
        network_manager->bind_port(4242);
        UtilityFunctions::print("Serveur demarre sur le port 4242");
    }
}

void GameServer::_on_packet_received(const String& sender_ip, int sender_port, const PackedByteArray& data) {
    if (data.size() < 4) {
        return; // Paquet trop petit, on ignore
    }
    
    uint32_t packet_type = data.decode_u32(0);

    UtilityFunctions::print("Paquet recu de type : ", packet_type);

    switch (packet_type)
    {
    case JOIN:
    {
        UtilityFunctions::print("Paquet JOIN reçu !");
        // Ajout du client à la liste
        connected_clients.push_back({sender_ip, sender_port});
        next_network_id++;
        
        if (entt_manager)
        {
            // Ajout de l'entité à l'ECS et stock id local
            int ID_local = entt_manager->create_entity();
            network_to_local[next_network_id] = ID_local;
        }
        // Creation du packet SPAWN
        PackedByteArray packet;
        packet.resize(20);
        packet.encode_u32(0, PacketType::SPAWN);
        packet.encode_u32(4, next_network_id);
        packet.encode_u32(8, 1); // Stub type id : 1 pour player pour le moment
        packet.encode_u32(12, 0); // Stub x
        packet.encode_u32(16, 0); // Stub y
        
        for (const ClientInfo& client : connected_clients)
        {
            network_manager->send_packet(client.ip, client.port, packet);
        }
        
        break;
    }
    case SPAWN:
        UtilityFunctions::print("Paquet SPAWN reçu !");
        break;
            
    default:
        UtilityFunctions::print("Type de paquet inconnu : ", packet_type);
        break;
    }
}