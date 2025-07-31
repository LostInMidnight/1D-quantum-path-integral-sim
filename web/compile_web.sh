#!/bin/bash

if ! command -v emcc &> /dev/null; then
    echo "Error: Emscripten not found. Please install and activate Emscripten:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

echo "Available HTML templates:"
echo "  1. shell_minimal.html (full-featured with controls)"
echo "  2. shell_bare.html (minimal template for custom implementation)"
echo ""
read -p "Choose template (1 or 2, default=1): " template_choice

if [ "$template_choice" = "2" ]; then
    SHELL_FILE="shell_bare.html"
    OUTPUT_NAME="custom"
    echo "Using minimal template for custom implementation..."
else
    SHELL_FILE="shell_minimal.html"
    OUTPUT_NAME="index"
    echo "Using full-featured template..."
fi

echo "Compiling 1D Quantum Path Integral Simulation for web..."

emcc ../src/main_web.cpp -o ${OUTPUT_NAME}.html \
  -s USE_WEBGL2=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_FUNCTIONS="['_main','_setLatticeSize','_setTimeSteps','_setNumPaths','_setHbar','_setMass','_setDt','_setDx','_regeneratePaths']" \
  -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" \
  --shell-file ${SHELL_FILE} \
  -O2 -std=c++11

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo "Files generated:"
    echo "  - ${OUTPUT_NAME}.html"
    echo "  - ${OUTPUT_NAME}.js"
    echo "  - ${OUTPUT_NAME}.wasm"
    echo ""
    echo "To run the simulation:"
    echo "  python -m http.server 8000"
    echo "  Open browser to http://localhost:8000/${OUTPUT_NAME}.html"

    if [ "$template_choice" = "2" ]; then
        echo ""
        echo "Note: You're using the minimal template."
        echo "Edit ${OUTPUT_NAME}.html to customize the interface."
        echo "See comments in the file for available functions and examples."
    fi
else
    echo "Compilation failed!"
    exit 1
fi
