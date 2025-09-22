#!/bin/bash
cd "$(dirname "$0")"
g++ main.cpp -o main -I/home/leul/include/stb -I/home/leul/include/tinyobjloader -I/usr/include/freetype2 -I/usr/include/AL -lglut -lGLU -lGL -lfreetype -lopenal && ./main