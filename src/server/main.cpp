#include <iostream>
#include <thread>
#include <chrono>
#include "server_network.h"

#pragma comment(lib, "ws2_32.lib")

int main() {
    std::cout << "[Server] Starting..." << std::endl;

    ServerNetworkManager network(8050); // Port 8050 comme dans ton client

    if (!network.start()) {
        return 1;
    }

    bool running = true;
    while (running) {
        // A. Réseau
        network.poll();

        // B. Update Jeu (ECS) - À venir

        // Sleep ~16ms (60 FPS)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}