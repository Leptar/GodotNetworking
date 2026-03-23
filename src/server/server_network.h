#ifndef GODOTNETWORKINGLAB_SERVER_NETWORK_H
#define GODOTNETWORKINGLAB_SERVER_NETWORK_H

#ifndef SERVER_NETWORK_H
#define SERVER_NETWORK_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <map>
#include <iostream>

// On inclut nos types communs (définis précédemment)
#include "../common/game_types.h"

#pragma comment(lib, "Ws2_32.lib")

class ServerNetworkManager {

    struct ClientInfo {
        std::string ip;
        int port;
    };
    uint32_t next_network_id = 100;
    std::map<uint32_t, ClientInfo> connected_clients;

    SOCKET udp_socket = INVALID_SOCKET;
    uint16_t port;

    // Helper interne pour configurer Winsock
    void _set_non_blocking(SOCKET sock);

public:
    ServerNetworkManager(uint16_t port);
    ~ServerNetworkManager();

    // Lance le serveur (équivalent de ton bind_port)
    bool start();

    // Vérifie les paquets entrants (équivalent de ton poll)
    // On renvoie true si un paquet a été traité
    bool poll();

    // Envoi de données
    void send_packet(const std::string& ip, int port, const std::vector<uint32_t>& data);

private:
    // La fonction qui traitera le paquet reçu (logique serveur)
    void _on_packet_received(const std::string& sender_ip, int sender_port, std::vector<uint8_t>& data);
};

#endif

#endif //GODOTNETWORKINGLAB_SERVER_NETWORK_H