#include "server_network.h"

ServerNetworkManager::ServerNetworkManager(uint16_t p_port) : port(p_port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[Network] Error initializing WSAStartup" << std::endl;
    }
}

ServerNetworkManager::~ServerNetworkManager() {
    if (udp_socket != INVALID_SOCKET) {
        closesocket(udp_socket);
    }
    WSACleanup();
}

void ServerNetworkManager::_set_non_blocking(SOCKET sock) {
    if (sock == INVALID_SOCKET) return;
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
}

bool ServerNetworkManager::start() {
    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket == INVALID_SOCKET) {
        std::cerr << "[Network] Failed to create UDP socket" << std::endl;
        return false;
    }

    _set_non_blocking(udp_socket);

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(udp_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        std::cerr << "[Network] Failed to bind UDP socket on port " << port << std::endl;
        closesocket(udp_socket);
        return false;
    }

    std::cout << "[Network] Server listening on port " << port << std::endl;
    return true;
}

void ServerNetworkManager::send_packet(const std::string& ip, int port, const std::vector<uint8_t>& data) {
    if (udp_socket == INVALID_SOCKET) return;

    sockaddr_in dest_address{};
    dest_address.sin_family = AF_INET;
    dest_address.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &dest_address.sin_addr);

    sendto(udp_socket, (const char*)data.data(), (int)data.size() * sizeof(uint8_t), 0, (struct sockaddr*)&dest_address, sizeof(dest_address));
}

bool ServerNetworkManager::poll() {
    if (udp_socket == INVALID_SOCKET) return false;

    char buffer[65535];
    sockaddr_in sender_address;
    int sender_len = sizeof(sender_address);

    // On boucle tant qu'il y a des paquets à lire
    while (true) {
        int len = recvfrom(udp_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&sender_address, &sender_len);

        if (len < 0) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                return false; // Plus rien à lire
            } else {
                std::cerr << "[Network] Socket error: " << error << std::endl;
                return false;
            }
        }

        // Conversion buffer -> std::vector
        std::vector<uint8_t> received_data(buffer, buffer + len);

        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sender_address.sin_addr, sender_ip, INET_ADDRSTRLEN);
        int sender_port = ntohs(sender_address.sin_port);

        // Au lieu d'émettre un signal, on traite directement
        _on_packet_received(std::string(sender_ip), sender_port, received_data);
        return true; // On a traité au moins un paquet
    }
}

// Helper pour décoder un uint32 depuis un vecteur d'octets (stub pour le serialize j'imagine ?)
uint32_t decode_uint32(const std::vector<uint8_t>& data, size_t offset) {
    if (data.size() < offset + 4) return 0; // Sécurité
    uint32_t value;
    memcpy(&value, data.data() + offset, sizeof(uint32_t));
    return value;
}

uint64_t decode_uint64(const std::vector<uint8_t>& data, size_t offset) {
    if (data.size() < offset + 8) return 0; // Sécurité
    uint64_t value;
    memcpy(&value, data.data() + offset, sizeof(uint64_t));
    return value;
}

void encode_uint32(std::vector<uint8_t>& packet, uint32_t value) {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&value);
    packet.insert(packet.end(), ptr, ptr + sizeof(uint32_t));
}

void encode_uint64(std::vector<uint8_t>& packet, uint64_t value) {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(&value);
    packet.insert(packet.end(), ptr, ptr + sizeof(uint64_t));
}

void ServerNetworkManager::_on_packet_received(const std::string& sender_ip, int sender_port, std::vector<uint8_t>& data) {
    if (data.empty()) return;

    uint32_t packet_type = decode_uint32(data, 0);

    switch (packet_type) {
        case 0:
        { // JOIN
            std::cout << "[Network] Received JOIN from " << sender_ip << ":" << sender_port << std::endl;
            ClientInfo info;
            info.ip = sender_ip;
            info.port = sender_port;
            info.last_activity_time = std::chrono::steady_clock::now();
            connected_clients[next_network_id] = info;

            uint32_t new_client_id = next_network_id;
            next_network_id++;
            std::cout << "[Server] New client connected. Assigned NetworkID: " << new_client_id << std::endl;

            // paquet SPAWN
            std::vector<uint8_t> spawn_otherPlayer_packet;

            encode_uint32(spawn_otherPlayer_packet, 1);// PacketType::SPAWN
            encode_uint32(spawn_otherPlayer_packet, new_client_id); // NetworkID
            encode_uint32(spawn_otherPlayer_packet, 2); // TypeID::OTHERPLAYER
            encode_uint32(spawn_otherPlayer_packet, 0); // X
            encode_uint32(spawn_otherPlayer_packet, 0); // Y

            // envoie tous les joueurs deja co au nouveau client
            for (const auto& [net_id, client] : connected_clients)
            {
                if (net_id == new_client_id) {
                    std::vector<uint8_t> spawn_OwnPlayer_packet;

                    encode_uint32(spawn_OwnPlayer_packet, 1);// PacketType::SPAWN
                    encode_uint32(spawn_OwnPlayer_packet, new_client_id); // NetworkID
                    encode_uint32(spawn_OwnPlayer_packet, 1); // TypeID::OWNPLAYER
                    encode_uint32(spawn_OwnPlayer_packet, 0); // X
                    encode_uint32(spawn_OwnPlayer_packet, 0); // Y

                    send_packet(client.ip, client.port, spawn_OwnPlayer_packet);
                } else {
                    send_packet(client.ip, client.port, spawn_otherPlayer_packet);
                }
            }

            // envoie aux autre client le nouveau client
            for (const auto& [net_id, client] : connected_clients)
            {
                if (net_id == new_client_id) continue;

                std::vector<uint8_t> other_packet;

                encode_uint32(other_packet, 1);
                encode_uint32(other_packet, net_id);
                encode_uint32(other_packet, 2);
                encode_uint32(other_packet, 0); // Stub (en attendant d'avoir un manager entt, qui me donnera la position)
                encode_uint32(other_packet, 0); // Stub (meme chose)

                send_packet(sender_ip, sender_port, other_packet);

            }

            break;
        }
        case 1: // SPAWN
            // ...
            break;


        case 2: { // LEAVE
            uint32_t network_id = decode_uint32(data, 4);
            auto it = connected_clients.find(network_id);
            if (it == connected_clients.end()) {
                std::cout << "[Network] Client " << sender_ip << ":" << sender_port << " already deconnected" << std::endl;
                break;
            }

            std::cout << "[Network] Client " << sender_ip << ":" << sender_port << " deconnected" << std::endl;
            std::vector<uint8_t> LeavePacket;
            encode_uint32(LeavePacket, 2);
            encode_uint32(LeavePacket, it->first);
            encode_uint32(LeavePacket, 1); // Player

            for (const auto& [net_id, client] : connected_clients) {
                if (net_id == it->first) continue;
                send_packet(client.ip, client.port, LeavePacket);
            }

            connected_clients.erase(it);
            break;
        }

        case 3: { //PING
            uint32_t ping_id = decode_uint32(data, 4);
            uint64_t t0 = decode_uint64(data, 8);

            std::cout << "[Network] Received PING from " << sender_ip << ":" << sender_port << " with ID: " << ping_id;
            std::cout << " and timestamp: " << t0 << std::endl;

            auto now = std::chrono::steady_clock::now().time_since_epoch();
            uint64_t t1 = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

            std::vector<uint8_t> PongPacket;
            encode_uint32(PongPacket, 4);
            encode_uint32(PongPacket, ping_id);
            encode_uint64(PongPacket, t0);
            encode_uint64(PongPacket, t1);

            send_packet(sender_ip, sender_port, PongPacket);

            break;
        }

        default:
            std::cout << "[Network] Unknown packet type: " << packet_type << std::endl;
            break;
    }
}


void ServerNetworkManager::check_timeouts() {
    for (auto it = connected_clients.begin(); it != connected_clients.end();) {
        if (std::chrono::steady_clock::now() - it->second.last_activity_time > std::chrono::seconds(TIMEOUT)) {
            std::cout << "[Network] Client " << it->second.ip << ":" << it->second.port << " timed out" << std::endl;
            std::vector<uint8_t> LeavePacket;
            encode_uint32(LeavePacket, 2);
            encode_uint32(LeavePacket, it->first);
            encode_uint32(LeavePacket, 1); // Player

            for (const auto& [net_id, client] : connected_clients) {
                if (net_id == it->first) continue;
                send_packet(client.ip, client.port, LeavePacket);
            }

            it = connected_clients.erase(it);
        } else {
            ++it;
        }
    }
}