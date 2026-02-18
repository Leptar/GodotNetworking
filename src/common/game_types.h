

#ifndef GODOTNETWORKINGLAB_GAME_TYPES_H
#define GODOTNETWORKINGLAB_GAME_TYPES_H
// src/common/game_types.h
#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <cstdint> // Pour uint32_t

namespace godot {

    enum PacketType {
        JOIN = 0,
        SPAWN = 1,
    };

    enum EntityType { // Renommé pour éviter la confusion avec TypeID
        PLAYER = 1,
        ENEMY = 2,
    };

    struct ClientInfo {
        // On ne peut pas utiliser String de Godot ici si ce fichier est partagé
        // avec un serveur pur C++. Pour l'instant, on laisse ça de côté
        // ou on utilise std::string si le serveur n'utilise pas Godot.
        // Vu que c'est pour le client Godot pour l'instant :
        // (Laisse vide ou adapte plus tard)
    };
}

#endif
#endif //GODOTNETWORKINGLAB_GAME_TYPES_H