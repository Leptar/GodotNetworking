#include <iostream>
#include <thread>
#include <chrono>
#include "server_network.h"

#pragma comment(lib, "winmm.lib")
#include <windows.h>

int main() {
    std::cout << "[Server] Starting..." << std::endl;

    ServerNetworkManager network(8050);

    if (!network.start()) {
        return 1;
    }


    timeBeginPeriod(1); // précision de 1ms

    bool running = true;

    const double TICK_RATE = 1.0 / 60.0;
    double accumulator = 0.0;

    auto previous_time = std::chrono::steady_clock::now();

    while (running) {

        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = current_time - previous_time;
        previous_time = current_time;
        accumulator += elapsed.count();


        network.poll();

        while (accumulator >= TICK_RATE) {
            network.check_timeouts();

            network.update_game(static_cast<float>(TICK_RATE));
            network.broadcast_world_state();

            accumulator -= TICK_RATE;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    timeEndPeriod(1);

    return 0;
}