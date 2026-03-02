#!/bin/bash
gcc -c src/glad.c -Iinclude -o build/glad.o
g++ src/main.cpp build/glad.o -o build/game -Iinclude -I/opt/homebrew/include -L/opt/homebrew/lib -lglfw -framework OpenGL

# chmod +x build.sh
# ./build.sh && ./build/game