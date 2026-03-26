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
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t last_time = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    while (running) {
        // A. Réseau
        network.poll();

        auto delta = std::chrono::steady_clock::now().time_since_epoch();
        uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
        float deltatime = (time - last_time) / 1000.0f;
        last_time = time;

        // B. Update Jeu (ECS)
        network.update_game(deltatime);
        network.broadcast_world_state();

        // Sleep ~16ms (60 FPS)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}