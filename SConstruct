#!/usr/bin/env python
import os

# 1. Initialisation de l'environnement via godot-cpp
# NOTE : On pointe vers le nouveau dossier lib/
env = SConscript("lib/godot-cpp/SConstruct")

# 2. Ajout des chemins d'inclusion (Include Paths)
# On inclut src/common pour les paquets partagés
# On inclut src/client pour les headers internes
# On inclut lib/entt/src pour l'ECS
env.Append(CPPPATH=["src/common", "src/client", "lib/entt/src"])

# 3. Flags spécifiques (Tes optimisations conservées)
if env["platform"] == "macos":
    # Force ARM64 pour M1/M2/M3/M4 si nécessaire
    env.Append(CCFLAGS=["-std=c++20", "-arch", "arm64"])
    env.Append(LINKFLAGS=["-arch", "arm64"])
    
elif env["platform"] == "windows":
    env.Append(CCFLAGS=["/std:c++20", "/Zc:__cplusplus", "/O2"]) 
    env.Append(LIBS=["ws2_32", "user32", "gdi32", "winmm"]) # ws2_32 est vital pour les sockets
    
elif env["platform"] == "linux":
    env.Append(CCFLAGS=["-std=c++20", "-O3"])

# 4. Récupération des fichiers sources
# On compile tout ce qui est dans client ET common
sources = Glob("src/client/*.cpp") + Glob("src/common/*.cpp")

if not sources:
    print("ERREUR: Aucun fichier source trouvé dans src/client ou src/common !")
    Exit(1)

# 5. Configuration de la sortie (Output)
# On veut que la DLL finisse dans game/bin/ pour que Godot la trouve
# Le nom de la lib sera déterminé par godot-cpp (ex: libgdexample.windows.template_debug.x86_64.dll)
# Assure-toi que le dossier game/bin existe
if not os.path.exists("game/bin"):
    os.makedirs("game/bin")

library = env.SharedLibrary(
    target="game/bin/godot_network_lab", # Nom de base de ta lib
    source=sources
)

Default(library)