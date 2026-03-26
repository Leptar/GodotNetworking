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
    Ref<PackedScene> own_player_scene = ResourceLoader::get_singleton()->load("res://scenes/ownplayer.tscn");
    Ref<PackedScene> other_player_scene = ResourceLoader::get_singleton()->load("res://scenes/otherplayer.tscn");
    register_type(OWN_PLAYER, own_player_scene);
    register_type(OTHER_PLAYER, other_player_scene);



    // TODO : recuperer l'instance du entt_Manager (surement)

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
            PingPacket.resize(16);
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


    uint8_t current_keys = 0;
    if (Input::get_singleton()->is_action_pressed("ui_up")) current_keys |= (1 << 0);
    if (Input::get_singleton()->is_action_pressed("ui_down")) current_keys |= (1 << 1);
    if (Input::get_singleton()->is_action_pressed("ui_left")) current_keys |= (1 << 2);
    if (Input::get_singleton()->is_action_pressed("ui_right")) current_keys |= (1 << 3);

    Vector2 mouse_pos = get_viewport()->get_mouse_position();
    FrameInput frame_input{current_keys, mouse_pos.x, mouse_pos.y};

    if (input_history.size() >= 20) {
        input_history.pop_front();
    }

    input_history.push_back(frame_input);
    current_sequence++;

    InputPacket input_packet{};
    input_packet.packet_type = INPUT;
    input_packet.network_id = local_network_id;
    input_packet.sequence = current_sequence;
    int i = 0;
    for (; i < input_history.size(); i++) {
        input_packet.keys[i] = input_history[i].keys;
        input_packet.aim_x[i] = input_history[i].aim_x;
        input_packet.aim_y[i] = input_history[i].aim_y;
    }

    PackedByteArray data;
    data.resize(sizeof(InputPacket));

    memcpy(data.ptrw(), &input_packet, sizeof(InputPacket));

    send_packet("127.0.0.1", 8050, data);
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
        
        UtilityFunctions::print("Received ", len, "bytes");
        emit_signal("packet_received", String(sender_ip), sender_port, received_data);
		
    }
}

void GDNetworkManager::_on_packet_received(const String& sender_ip, int sender_port, const PackedByteArray& data)
{
    uint32_t packet_type = data.decode_u32(0);
    UtilityFunctions::print("Paquet recu de type : ", packet_type);
    
    switch (packet_type)
    {
        case SPAWN: {

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

                if (Node2D *node_2d = Object::cast_to<Node2D>(new_entity)) {
                    node_2d->set_position(Vector2(x, y));

                    bool is_mine = netID == local_network_id;
                    node_2d->set("is_local_authority", is_mine);
                }
                if (SceneTree *tree = get_tree()) {
                    Window *root = tree->get_root();
                    root->add_child(new_entity);
                    register_node(netID, new_entity);
                }

                debug_print_nodes();
            } else {
                UtilityFunctions::print("Received unknown typeID");
            }

            break;
        }

        case LEAVE: {
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
            uint32_t ping_id = data.decode_u32(4);
            uint64_t t0 = data.decode_u64(8);
            uint64_t t1 = data.decode_u64(16);

            auto now = std::chrono::steady_clock::now().time_since_epoch();
            uint64_t t2 = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

            uint64_t rtt = t2 - t0;

            if (rtt_history.size() > 10) {
                rtt_history.erase(rtt_history.begin());
            }
            rtt_history.push_back(rtt);

            latency = std::accumulate(rtt_history.begin(), rtt_history.end(), 0) / rtt_history.size();

            emit_signal("_latency_updated", latency);
            UtilityFunctions::print("Actual Latency : ", latency, " ms");

            break;
		}

        case WORLD_STATE: {
            uint32_t num_entities = data.decode_u32(4);

            for (uint32_t i = 0; i < num_entities; i++) {
                uint32_t net_id = data.decode_u32(8 + i * 12);
                float x = data.decode_float(12 + i * 12);
                float y = data.decode_float(16 + i * 12);
                auto it = replicated_nodes.find(net_id);

                if (it == replicated_nodes.end()) {
                    continue;
                    // TODO : si le noeud existe pas le creer mais il faudrait envoyer le typeID de l'objet pour le spawn
                }
                if (it->second.is_valid()) {
                    cast_to<Node2D>(it->second.get_node())->set_global_position(Vector2(x, y));
                }
            }

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
            LeavePacket.encode_u32(8, OWN_PLAYER);

            send_packet("127.0.0.1", 8050, LeavePacket);

            break;
        }

        default: break;
    }

}