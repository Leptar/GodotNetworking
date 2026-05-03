#define WIN32_LEAN_AND_MEAN 
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> 
#include "gn_network_manager.h"

#include <numeric>
#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/time.hpp>

#include "../../lib/godot-cpp/gen/include/godot_cpp/variant/utility_functions.hpp"
#include "../../lib/godot-cpp/include/godot_cpp/core/math.hpp"
#include "../../lib/godot-cpp/include/godot_cpp/variant/vector2.hpp"

using namespace godot;

#ifdef _WIN32
struct GD_WSASockInitializer
{
    GD_WSASockInitializer()
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            UtilityFunctions::printerr("Error initializing WSAStartup");
        }
    };
    
    ~GD_WSASockInitializer()
    {
        WSACleanup();
    }
};
#endif



void GDNetworkManager::_bind_methods() {
    ADD_SIGNAL(MethodInfo("packet_received", 
    	PropertyInfo(Variant::STRING, "ip"),
    	PropertyInfo(Variant::INT, "port"), 
        PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data")
    ));

    ADD_SIGNAL(MethodInfo("_latency_updated",
        PropertyInfo(Variant::INT, "latency")
    ));

	ClassDB::bind_method(D_METHOD("bind_port", "port"), &GDNetworkManager::bind_port);
	ClassDB::bind_method(D_METHOD("send_packet", "ip", "port", "data"), &GDNetworkManager::send_packet);
	ClassDB::bind_method(D_METHOD("poll"), &GDNetworkManager::poll);

    ClassDB::bind_method(D_METHOD("_on_packet_received", "ip", "port", "data"), &GDNetworkManager::_on_packet_received);
}

void GDNetworkManager::register_type(uint32_t type_id, Ref<PackedScene> scene) {
    if (scene.is_valid()) {
        type_registry[type_id] = scene;
        UtilityFunctions::print("Type enregistré : ", type_id);
    }
}

GDNetworkManager::GDNetworkManager() {
    UtilityFunctions::print("--- GDNetworkManager CONSTRUCTOR CALLED ---");
    #ifdef _WIN32
        static GD_WSASockInitializer wsa_init;
	#endif
}

GDNetworkManager::~GDNetworkManager() {
    _close_socket();
}

void GDNetworkManager::_ready() {
    time = Time::get_singleton();

    Ref<PackedScene> player_scene = ResourceLoader::get_singleton()->load("res://scenes/player.tscn");
    register_type(PLAYER, player_scene);

    this->connect("packet_received", Callable(this, "_on_packet_received"));
    bIsBinded = bind_port(0);

    PackedByteArray JoinPacket;
    JoinPacket.resize(4);
    JoinPacket.encode_u32(0, JOIN);

    send_packet("127.0.0.1", 8050, JoinPacket);

	ping_thread = std::thread([this]() {
    	uint32_t current_ping_id = 0;

    	while (is_running) {
        	// Récupérer le temps actuel en millisecondes (t0)
        	auto now = std::chrono::steady_clock::now().time_since_epoch();
        	uint64_t t0 = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

        	// Construire et envoyer le paquet
        	PackedByteArray PingPacket;
            PingPacket.resize(20);
            PingPacket.encode_u32(0, PING);
    	    PingPacket.encode_u32(4, local_network_id);
            PingPacket.encode_u32(8, current_ping_id);
            PingPacket.encode_u64(12, t0);

        	send_packet("127.0.0.1", 8050, PingPacket);

        	current_ping_id++;

        	// Faire une pause d'une seconde avant la prochaine itération
        	std::this_thread::sleep_for(std::chrono::seconds(1));
    	}
	});
}

void GDNetworkManager::_physics_process(double delta) {
    poll();

    if (!is_clock_synced && frames.size() >= 3) {
        is_clock_synced = true;
        render_frame = frames.back().frame_id - 2.0;
    }

    if (is_clock_synced) {
        double play_speed = 1.0;

        if (frames.size() <= 2) {
            play_speed = 0.90;
        }
        else if (frames.size() >= 5) {
            play_speed = 1.10;
        }

        render_frame += delta * 60.0 * play_speed;
    }

    uint8_t current_keys = 0;
    if (Input::get_singleton()->is_action_pressed("ui_up")) current_keys |= (1 << 0);
    if (Input::get_singleton()->is_action_pressed("ui_down")) current_keys |= (1 << 1);
    if (Input::get_singleton()->is_action_pressed("ui_left")) current_keys |= (1 << 2);
    if (Input::get_singleton()->is_action_pressed("ui_right")) current_keys |= (1 << 3);
    // UtilityFunctions::printerr("current keys : ", current_keys);
    // if (current_keys == 0) { return; }
    Vector2 mouse_pos = get_viewport()->get_mouse_position();
    FrameInput frame_input{current_sequence, current_keys, mouse_pos.x, mouse_pos.y, 0.f,0.f};

    //TODO : appel methode pour mouvoir le joueur local
    Node* player_node = replicated_nodes[local_network_id].get_node();
    if (player_node) {
        Node2D* body = player_node->get_node<Node2D>("Body2D");
        Vector2 position = body->call("perform_simulation", current_keys, delta);
        frame_input.x = position.x;
        frame_input.y = position.y;
    }

    if (input_history.size() >= 20) {
        input_history.pop_front();
    }

    input_history.push_back(frame_input);
    current_sequence++;

    InputPacket input_packet{};
    input_packet.packet_type = INPUT;
    input_packet.last_sequence = current_sequence;
    input_packet.network_id = local_network_id;

    int index = 0;
    for (auto it = input_history.rbegin(); it != input_history.rend(); ++it) {
        input_packet.keys[index] = it->keys;
        input_packet.aim_x[index] = it->aim_x;
        input_packet.aim_y[index] = it->aim_y;
        index++;
    }

    PackedByteArray data;
    data.resize(sizeof(InputPacket));

    memcpy(data.ptrw(), &input_packet, sizeof(InputPacket));

    send_packet("127.0.0.1", 8050, data);

    if (is_clock_synced && frames.size() >= 3) {
        draw();
    }
}

void GDNetworkManager::draw() {
    double FrameActuel = render_frame;
    /*UtilityFunctions::printerr("FA = ", FrameActuel);
    FrameActuel = std::round(FrameActuel * 10.0) / 10.0;*/

    if (FrameActuel > static_cast<double>(frames.back().frame_id)) {
        UtilityFunctions::printerr("Network jitter (lag) : en attente du serveur...");

        WorldStatePacket interpolated_packet = interpolation(frames.at(1), frames.at(2), 1.0);
        drawFrame(interpolated_packet);
        is_clock_synced = false;

    } else if (FrameActuel >= static_cast<double>(frames.front().frame_id)
        && FrameActuel <= static_cast<double>(frames.at(1).frame_id)) {
        WorldStatePacket interpolated_packet;
        interpolated_packet =
            interpolation(frames.at(0), frames.at(1), FrameActuel-frames.at(0).frame_id);
        //UtilityFunctions::printerr("frame draw between 0 and 1");
        drawFrame(interpolated_packet);

    } else if (FrameActuel >= static_cast<double>(frames.at(1).frame_id)
        && FrameActuel <= static_cast<double>(frames.at(2).frame_id)) {
        WorldStatePacket interpolated_packet;
        interpolated_packet =
            interpolation(frames.at(1), frames.at(2), FrameActuel-frames.at(1).frame_id);
        //UtilityFunctions::printerr("frame draw between 1 and 2");
        drawFrame(interpolated_packet);

    } else {
        //UtilityFunctions::printerr("frame dropped");
        is_clock_synced = false;
    }

}


void GDNetworkManager::drawFrame(WorldStatePacket &packet) {
    //UtilityFunctions::printerr("GDNetworkManager::drawFrame : called");

    for (auto entity : packet.entities) {
        Vector2 pos = Vector2(entity.x, entity.y);

        auto it = replicated_nodes.find(entity.network_id);
        if (it == replicated_nodes.end()) {
            // UtilityFunctions::printerr("GDNetworkManager::drawFrame : node not find");
            continue;
            // TODO : si le noeud existe pas le creer
        }

        if (it->second.is_valid()) {
            if (entity.type_id == PLAYER) {
                Node2D* node = cast_to<Node2D>(it->second.get_node());
                Node2D* body = node->get_node<Node2D>("Body2D");

                if (entity.network_id != local_network_id) {
                    Vector2 direction = pos - body->get_global_position();
                    // UtilityFunctions::printerr("set target pos et direction : ", pos, " / ",direction);
                    body->set("target_pos", pos);
                    body->set("direction", direction);
                }
            } else {
                cast_to<Node2D>(it->second.get_node())->set_global_position(pos);
            }
        }
    }

    for (const auto& [key, value] : replicated_nodes) {
        auto it = replicated_nodes.find(key);
        if (it == replicated_nodes.end()) {
            continue; // TODO Delete node
        }
    }
}

WorldStatePacket GDNetworkManager::interpolation(WorldStatePacket packet1, WorldStatePacket packet2, float lambda) {

    if (lambda <= 0.0) return packet1;
    if (lambda >= 1.0) return packet2;

    WorldStatePacket interpolated_packet;
    EntityState entity;

    for (auto packet1_entity : packet1.entities) {
        entity.network_id = packet1_entity.network_id;
        auto it = std::find_if(packet2.entities.begin(), packet2.entities.end(),
            [entity](EntityState ent) {return ent.network_id == entity.network_id; });
        if (it != packet2.entities.end()) {
            entity.type_id = packet1_entity.type_id;
            entity.x = Math::lerp(packet1_entity.x, it->x, lambda);
            entity.y = Math::lerp(packet1_entity.y, it->y, lambda);
        } else {
            entity.type_id = packet1_entity.type_id;
            entity.x = packet1_entity.x;
            entity.y = packet1_entity.y;
        }

        interpolated_packet.entities.push_back(entity);

    }

    interpolated_packet.frame_id = 0;
    interpolated_packet.size = interpolated_packet.entities.size();
    return interpolated_packet;
}

void GDNetworkManager::_close_socket() {
    if (udp_socket != INVALID_SOCKET) {
        closesocket(udp_socket);
        udp_socket = INVALID_SOCKET;
    }
}

void GDNetworkManager::_set_non_blocking(SOCKET sock) {
    if (sock == INVALID_SOCKET) return;
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
}

bool GDNetworkManager::bind_port(int port)
{
    _close_socket();
    udp_socket = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
    if (udp_socket == INVALID_SOCKET)
    {
        UtilityFunctions::printerr("Failed to create UDP socket");
        return false;
    }
    
    _set_non_blocking(udp_socket);
    
    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    
    if (bind(udp_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR)
    {
        UtilityFunctions::printerr("Failed to bind UDP socket");
        _close_socket();
        return false;
    }
    
    UtilityFunctions::print("Binding UDP socket to port ", port);
    return true;
}

void GDNetworkManager::send_packet(String ip, int port, PackedByteArray data)
{
    if (udp_socket == INVALID_SOCKET)
    {
        return;
    }
    
    sockaddr_in dest_address{};
    dest_address.sin_family = AF_INET;
    dest_address.sin_port = htons(port);
    
    //convert String IP to C-String IP
    inet_pton(AF_INET, ip.utf8().get_data(), &dest_address.sin_addr);
    
    int sent_bytes = sendto(udp_socket,
        (const char*)data.ptr(),
        data.size(),
        0,
        (struct sockaddr*)&dest_address,
        sizeof(dest_address));
    
    if (sent_bytes == SOCKET_ERROR)
    {
        UtilityFunctions::printerr("Failed to send data to UDP socket");
    }
}

void GDNetworkManager::poll()
{
    if (udp_socket == INVALID_SOCKET) return;
    
    char buffer[65535];
    while (true)
    {
		sockaddr_in sender_address; 
        int sender_len = sizeof(sender_address);

        int len = recvfrom(udp_socket,
        			buffer,
        			sizeof(buffer),
        			0,
        			(struct sockaddr*)&sender_address,
        			&sender_len);
        
        if (len < 0)
        {
            int error_code = WSAGetLastError();
            if (error_code == WSAEWOULDBLOCK) 
            {
                break; 
            }
            else 
            {
                UtilityFunctions::printerr("Socket error: ", error_code);
                break;
            }
        }

        PackedByteArray received_data;
        received_data.resize(len);
        memcpy(received_data.ptrw(), buffer, len);
        
        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sender_address.sin_addr, sender_ip, INET_ADDRSTRLEN);
        int sender_port = ntohs(sender_address.sin_port);
        
        // UtilityFunctions::print("Received ", len, "bytes");
        emit_signal("packet_received", String(sender_ip), sender_port, received_data);
		
    }
}

void GDNetworkManager::_on_packet_received(const String& sender_ip, int sender_port, const PackedByteArray& data)
{
    uint32_t packet_type = data.decode_u32(0);

    
    switch (packet_type)
    {
        case SPAWN: {
            UtilityFunctions::print("Paquet recu de type : SPAWN");
            uint32_t typeID = data.decode_u32(8);
            uint32_t netID = data.decode_u32(4);
            auto it = type_registry.find(typeID);

            if (local_network_id == 0) {
                local_network_id = netID;
                UtilityFunctions::print(">>> JE SUIS LE JOUEUR ", local_network_id, " <<<");
            }

            if (it != type_registry.end()) {
                Ref<PackedScene> scene = it->second;
                Node* new_entity = scene->instantiate();

                float x = data.decode_float(12);
                float y = data.decode_float(16);
                UtilityFunctions::print("Position : ", x, y);

                if (Node2D *node_2d = cast_to<Node2D>(new_entity)) {
                    node_2d->set_global_position(Vector2(x, y));

                }

                if (SceneTree *tree = get_tree()) {
                    Window *root = tree->get_root();
                    root->add_child(new_entity);
                    new_entity->set_physics_process(true);
                    register_node(netID, new_entity);
                }

                if (typeID == PLAYER) {
                    UtilityFunctions::print(netID == local_network_id);
                    bool local_Player = netID == local_network_id;
                    Node* body = new_entity->get_node<Node>("Body2D");
                    body->call("set_local_player", local_Player);
                    body->call("enable_cam");
                    UtilityFunctions::print(body->get("bIsLocalPlayer"));
                }
                debug_print_nodes();
            } else {
                UtilityFunctions::print("Received unknown typeID");
            }

            break;
        }

        case LEAVE: {
            UtilityFunctions::print("Paquet recu de type : LEAVE");
            uint32_t netID = data.decode_u32(4);
            auto it = replicated_nodes.find(netID);

            if (it != replicated_nodes.end() ) {
                if (it->second.is_valid()) {
                    it->second.get_node()->queue_free();
                }
                replicated_nodes.erase(it);
            }

            debug_print_nodes();
            break;
        }

		case PONG: {
            // UtilityFunctions::print("Paquet recu de type : PONG");
            // uint32_t ping_id = data.decode_u32(4);
            uint64_t t0 = data.decode_u64(8);
            // uint64_t t1 = data.decode_u64(16);

            auto now = std::chrono::steady_clock::now().time_since_epoch();
            uint64_t t2 = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

            uint64_t rtt = t2 - t0;

            if (rtt_history.size() > 10) {
                rtt_history.erase(rtt_history.begin());
            }
            rtt_history.push_back(rtt);

            latency = std::accumulate(rtt_history.begin(), rtt_history.end(), 0ULL) / rtt_history.size();

            emit_signal("_latency_updated", latency);
            // UtilityFunctions::print("Actual Latency : ", latency, " ms");

            break;
		}

        case WORLD_STATE: {

            uint32_t frame_id = data.decode_u32(4);

            if (frames.size() > 0 && frame_id <= frames.back().frame_id) {
                break;
            }

            uint32_t num_entities = data.decode_u32(8);

            //UtilityFunctions::print("Taille reçue : ", data.size());
            //UtilityFunctions::print("Entités annoncées : ", num_entities);

            std::vector<EntityState> entities;
            EntityState entity;

            for (uint32_t i = 0; i < num_entities; i++) {
                entity.network_id = data.decode_u32(12 + i * 20);
                entity.type_id = data.decode_u32(16 + i * 20);
                entity.x = data.decode_float(20 + i * 20);
                entity.y = data.decode_float(24 + i * 20);
                entity.last_processed_sequence = data.decode_u32(28 + i * 20);

                entities.push_back(entity);

                if (entity.network_id != local_network_id) continue;

                auto it = std::find_if(input_history.begin(), input_history.end(),
                    [&entity](const FrameInput& frame) {
                        return frame.sequence == entity.last_processed_sequence;
                    }
                );

                if (it == input_history.end()) continue;

                Vector2 pos_serveur(entity.x, entity.y);
                Vector2 pos_prediction(it->x, it->y);

                float distance_erreur = pos_prediction.distance_to(pos_serveur);
                Vector2 vecteur_erreur = pos_serveur - pos_prediction;

                if (distance_erreur > 5 && distance_erreur < 15) {
                    Node* player_node = replicated_nodes[local_network_id].get_node();
                    if (player_node) {
                        Node2D* body = player_node->get_node<Node2D>("Body2D");
                        body->call("set_error_offset", vecteur_erreur);
                    }

                 } else if (distance_erreur >= 15) {
                     UtilityFunctions::print(distance_erreur);
                     Node* player_node = replicated_nodes[local_network_id].get_node();
                     if (player_node) {
                         // TP
                         Node2D* body = player_node->get_node<Node2D>("Body2D");
                         body->set_global_position(pos_serveur);
                         body->call("set_error_offset", Vector2());

                         // vide input
                         while (!input_history.empty() && input_history.front().sequence <= entity.last_processed_sequence) {
                             input_history.pop_front();
                             UtilityFunctions::print(input_history.size());
                         }

                         // replay
                         for (FrameInput &frame : input_history) {
                             Vector2 position = body->call("perform_simulation", frame.keys, 1.f/60.f);
                             frame.x = position.x;
                             frame.y = position.y;
                         }
                     }
                 }
            }

            WorldStatePacket frame_packet;
            frame_packet.frame_id = frame_id;
            frame_packet.size = num_entities;
            frame_packet.entities = entities;

            if (frames.size() > 3) {
                frames.pop_front();
            }

            frames.push_back(frame_packet);

            break;
        }

        default: break;
    }
}

void GDNetworkManager::register_node(uint32_t net_id,Node* p_node) {
    GDReplicatedNode node;
    node.node_id = p_node->get_instance_id();
    node.properties.push_back("position");

    replicated_nodes[net_id] = node;
}

void GDNetworkManager::debug_print_nodes() {
    UtilityFunctions::print("--- Liste des Objets Répliqués ---");
    for (const auto& pair : replicated_nodes) {
        uint32_t net_id = pair.first;
        // On récupère la valeur (GDReplicatedNode)
        const GDReplicatedNode& node_info = pair.second;

        String status = "Invalide";
        if (node_info.is_valid()) {
            status = node_info.get_node()->get_name();
        }

        UtilityFunctions::print("NetID: ", net_id, " -> Node: ", status);
    }
    UtilityFunctions::print("----------------------------------");
}

void GDNetworkManager::_notification(int p_notification) {
    switch (p_notification) {
        case NOTIFICATION_WM_CLOSE_REQUEST: {

			is_running.store(false);
            ping_thread.join();

            PackedByteArray LeavePacket;
            LeavePacket.resize(12);
            LeavePacket.encode_u32(0, LEAVE);
            LeavePacket.encode_u32(4, local_network_id);
            LeavePacket.encode_u32(8, PLAYER);

            send_packet("127.0.0.1", 8050, LeavePacket);

            break;
        }

        default: break;
    }

}

