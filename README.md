# 3D 8-Queens Puzzle Game

A visually stunning 3D implementation of the classic 8-Queens puzzle using OpenGL, featuring interactive gameplay, sound effects, and a modern room environment.

## 🎯 Overview

This project solves the 8-Queens problem in a 3D chessboard environment. The goal is to place 8 queens on an 8x8 chessboard such that no two queens threaten each other (no two queens share the same row, column, or diagonal).

## ✨ Features

- **3D Graphics**: Immersive 3D chessboard with realistic lighting and textures
- **Interactive Gameplay**: Mouse and keyboard controls for placing queens
- **Sound Effects**: Audio feedback for queen placement and winning
- **Auto-Solve**: Automatic solution using backtracking algorithm
- **High Score Tracking**: Tracks the minimum number of invalid moves to solve
- **Modern UI**: On-screen instructions and score display
- **Room Environment**: Detailed 3D room with textured walls, floor, and lighting

## 🎮 Controls

- **Mouse Left Click + Drag**: Rotate the camera view
- **Left Click on Board**: Place a queen on the selected square
- **U Key**: Undo last queen placement
- **R Key**: Reset the board
- **S Key**: Auto-solve the puzzle
- **ESC**: Exit the game

## 🛠️ Prerequisites

### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install build-essential libglut3-dev libglu1-mesa-dev libgl1-mesa-dev libfreetype6-dev libopenal-dev
```

### Dependencies
- OpenGL
- GLUT/FreeGLUT
- OpenAL
- FreeType
- STB Image (included)
- TinyOBJLoader (included)

## 🚀 Building and Running

### Using Make (Recommended)
```bash
make
make run
```

### Manual Compilation
```bash
g++ main.cpp -o main -I/home/leul/include/stb -I/home/leul/include/tinyobjloader -I/usr/include/freetype2 -I/usr/include/AL -lglut -lGLU -lGL -lfreetype -lopenal
./main
```

## 📁 Project Structure

```
├── main.cpp              # Main game source code
├── Makefile              # Build automation
├── .gitignore           # Git ignore rules
├── resources/           # Game assets
│   ├── chessboard.jpg   # Chessboard texture
│   ├── wood.jpg         # Table texture
│   ├── ground.jpg       # Ground texture
│   ├── highscore.txt    # High score storage
│   ├── Queen/           # Queen model files
│   │   ├── *.obj        # 3D model
│   │   └── *.jpg        # Model textures
│   ├── queen_placed.wav # Placement sound effect
│   └── win_sound.wav    # Victory sound effect
├── include/             # Header dependencies
│   ├── stb/            # STB libraries
│   └── tinyobjloader/  # OBJ loader
└── freetype-2.10.1/    # FreeType library (optional)
```

## 🎨 Technical Details

- **Rendering**: OpenGL with GLUT for window management
- **3D Models**: OBJ format with texture mapping
- **Audio**: OpenAL for 3D positional audio
- **Text Rendering**: FreeType for on-screen text
- **Algorithm**: Backtracking for puzzle solving
- **Textures**: STB Image for loading various formats

## 🧩 Game Mechanics

1. **Queen Placement**: Click on empty squares to place queens
2. **Validation**: Invalid moves are rejected with visual/audio feedback
3. **Winning Condition**: Place all 8 queens without conflicts
4. **Scoring**: Tracks attempts and maintains high score
5. **Auto-Solve**: Instant solution demonstration

## 🔧 Customization

- Modify `BOARD_SIZE` in main.cpp for different board sizes
- Adjust camera angles and distances in the code
- Replace sound files in `resources/` directory
- Change textures for different visual themes

## 📈 Future Enhancements

- [ ] Multiplayer mode
- [ ] Different difficulty levels
- [ ] Animated queen movements
- [ ] Save/load game states
- [ ] Custom board themes
- [ ] Sound volume controls

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Open a Pull Request

## 📄 License

This project is open source. Feel free to use and modify.

## 🙏 Acknowledgments

- 3D Queen model from online resources
- Sound effects created for this project
- Open source libraries: OpenGL, OpenAL, FreeType, STB

---

**Enjoy solving the 8-Queens puzzle in 3D!** 👑♟️