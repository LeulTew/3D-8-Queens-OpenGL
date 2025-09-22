@echo off
cd /d "%~dp0"
g++ main.cpp -o main -I"C:\Users\Leul\include\stb" -I"C:\Users\Leul\include\tinyobjloader" -I"C:\usr\include\freetype2" -I"C:\usr\include\AL" -lglut -lGLU -lGL -lfreetype -lopenal && main.exe