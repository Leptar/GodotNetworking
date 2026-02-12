#ifndef GN_NETWORK_MANAGER_HPP
#define GN_NETWORK_MANAGER_HPP

// 1. INCLUDES
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
	
	struct GDReplicatedNode {
        ObjectID node_id;
        std::vector<StringName> properties;
        
        // Check if node is still valid
        bool is_valid() const {
            return Object::cast_to<Node>(ObjectDB::get_instance(node_id)) != nullptr;
        }
        
        Node* get_node() const {
            return Object::cast_to<Node>(ObjectDB::get_instance(node_id));
        }
    };

    class GDNetworkManager : public Node {
        GDCLASS(GDNetworkManager, Node)
	private:
        SOCKET udp_socket = INVALID_SOCKET;
        
        // Internal helper to set socket to non-blocking mode
        void _set_non_blocking(SOCKET sock);
        
        // Internal helper to close socket safely on all platforms
        void _close_socket();

        std::vector<GDReplicatedNode> replicated_nodes;

    protected:
        static void _bind_methods();

    public:
        GDNetworkManager();
        ~GDNetworkManager();

        void _process(double delta) override;

        // Bind to a port to receive data (Server or P2P Peer)
        bool bind_port(int port);

        // Send a packet to a specific IP/Port
        void send_packet(String ip, int port, PackedByteArray data);

        // Check for incoming packets (Call this in _process)
        void poll();

        void register_node(Node* p_node);
        PackedByteArray serialize_snapshot();
    };
}
#endif