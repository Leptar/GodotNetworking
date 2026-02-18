cmake_minimum_required(VERSION 3.20)
project(GodotNetworkingLab CXX)

set(CMAKE_CXX_STANDARD 20) # Tu utilises C++20 dans SCons, donc pareil ici
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- DEPENDANCES ---
include_directories(lib/entt/src)

# --- SOURCES ---
# Common : Tes paquets, ta logique socket partagée
file(GLOB_RECURSE COMMON_SOURCES "src/common/*.cpp" "src/common/*.h")
include_directories(src/common)

# Server : Ton main et ta logique serveur
file(GLOB_RECURSE SERVER_SOURCES "src/server/*.cpp" "src/server/*.h")

# --- EXECUTABLE SERVEUR ---
add_executable(ServerApp ${SERVER_SOURCES} ${COMMON_SOURCES}
        src/common/game_types.h
        src/server/server_network.h
        src/server/server_network.cpp)

# Link des sockets pour Windows (ws2_32)
if(WIN32)
    target_link_libraries(ServerApp ws2_32)
endif()

# --- CLIENT (Juste pour l'IDE CLion) ---
# Ceci permet à CLion de voir tes fichiers GDExtension même s'il ne les compile pas
include_directories(lib/godot-cpp/include)
include_directories(lib/godot-cpp/gen/include)
include_directories(lib/godot-cpp/gdextension)
include_directories(src/client) 

file(GLOB_RECURSE CLIENT_SOURCES "src/client/*.cpp" "src/client/*.h")
add_custom_target(GodotClientSources SOURCES ${CLIENT_SOURCES})