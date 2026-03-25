

#ifndef GODOTNETWORKINGLAB_GAME_TYPES_H
#define GODOTNETWORKINGLAB_GAME_TYPES_H

#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <cstdint> // Pour uint32_t
#include <string>


namespace godot {

    enum PacketType {
        JOIN = 0,
        SPAWN = 1,
        LEAVE = 2,
        PING = 3,
        PONG = 4
    };

    // Structure de base d'un paquet SPAWN
    #pragma pack(push, 1) // Assure que la struct n'a pas de "padding" mémoire
        struct SpawnPacket {
            uint32_t packet_type = 1; // 1 = SPAWN
            uint32_t network_id;
            uint32_t type_id;
            float x, y;
        };

        struct LeavePacket {
            uint32_t packet_type = 2; // 2 = LEAVE
            uint32_t network_id;
            uint32_t type_id;
        };

        struct PingPacket {
            uint32_t packet_type = 3; // 3 = PING
            uint32_t ping_id;
            uint64_t t0;
        };

        struct PongPacket {
            uint32_t packet_type = 4; // 4 = P0NG
            uint32_t ping_id;
            uint64_t t0;
            uint64_t t1;
        };

    #pragma pack(pop)

    enum TypeID {
        PLAYER = 1,
        ENEMY = 2,
        PROJECTILE = 3
    };

    struct ClientInfo {
        std::string ip;
        int port;
    };
}

#endif
#endif //GODOTNETWORKINGLAB_GAME_TYPES_H