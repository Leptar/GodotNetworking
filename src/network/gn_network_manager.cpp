#define WIN32_LEAN_AND_MEAN 
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> 
#include "gn_network_manager.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

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

	ClassDB::bind_method(D_METHOD("bind_port", "port"), &GDNetworkManager::bind_port);
	ClassDB::bind_method(D_METHOD("send_packet", "ip", "port", "data"), &GDNetworkManager::send_packet);
	ClassDB::bind_method(D_METHOD("poll"), &GDNetworkManager::poll);
}

GDNetworkManager::GDNetworkManager() {
    #ifdef _WIN32
    GD_WSASockInitializer();
	#endif
}

GDNetworkManager::~GDNetworkManager() {
    _close_socket();
}

void GDNetworkManager::_process(double delta) {
    poll();
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