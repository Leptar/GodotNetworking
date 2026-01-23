#!/bin/bash

echo "🚀 1. Building Custom Godot Editor (M4 Optimized)..."
cd engine
# dev_build=yes allows deep debugging
scons platform=macos arch=arm64 target=editor -j10
cd ..

echo "📝 2. Dumping API JSON from the fresh binary..."
# We run the binary we just built with --dump-extension-api
./engine/bin/godot.macos.editor.dev.arm64 --headless --dump-extension-api

echo "📦 3. Moving JSON to godot-cpp..."
# We overwrite the default JSON so bindings match YOUR binary exactly
mv extension_api.json src/godot-cpp/gdextension/extension_api.json

echo "🔗 4. Building C++ Bindings..."
cd src/godot-cpp
scons platform=macos arch=arm64 target=template_debug

echo "🔗 4. Generating the compile_commands.json..."
cd ..
scons platform=macos arch=arm64 target=template_debug compiledb=yes

cd ../..

echo "✅ Setup Complete. You are perfectly synced."