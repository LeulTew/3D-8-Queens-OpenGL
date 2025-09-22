#include <GL/glut.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <cstdint>
#include <vector>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// Store the texture IDs separately
std::vector<GLuint> queenTextures;

// Window dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Table dimensions
float tableWidth = 3.3f;     // Width of the table
float tableLength = 2.5f;    // Length of the table
float thickness = 0.1f;      // Thickness of the table top
float tableTopHeight = 0.9f; // Height of the table top from the ground

// Chessboard properties
const int BOARD_SIZE = 8;
const float SQUARE_SIZE = static_cast<float>(BOARD_SIZE) / 32.0f;

// Camera properties
float cameraAngleX = 30.0f;  // Angle of camera around x-axis
float cameraAngleY = 45.0f;  // Angle of camera around y-axis
float cameraDistance = 5.0f; // Distance of the camera from the origin

// Mouse interaction
int lastMouseX = 0;      // Last mouse x position
int lastMouseY = 0;      // Last mouse y position
bool isRotating = false; // Flag to check if rotation is happening

// Queen positions
std::vector<std::pair<int, int>> queens; // Positions of placed queens

// Function prototypes
void display();
void reshape(int w, int h);
void resetBoard();
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void drawChessboard();
void drawQueen(float x, float y, float z);
bool isValidMove(int row, int col);
void placeQueen(int row, int col);
void animateQueen(int value);
void renderBitmapString(float x, float y, float z, void *font, const char *string);
void drawGuide();
void drawScore();
void loadTextures();
bool loadQueenModel();
void drawTable();
void drawGroundPlane();
bool fileExists(const std::string &filename);
void loadHighScore();
void saveHighScore();

void drawRoom();

// sound
void initOpenAL();
void playSound(const char *filename);
void cleanupOpenAL();
void loadWAV(const char *filename, std::vector<uint8_t> &soundData, ALsizei &size, ALsizei &freq, ALenum &format);

// Texture IDs
GLuint chessboardTexture;
GLuint tableTexture;
GLuint groundTexture;

// Queen model data
tinyobj::attrib_t queenAttrib;
std::vector<tinyobj::shape_t> queenShapes;
std::vector<tinyobj::material_t> queenMaterials;

// Game state
bool gameWon = false;
bool showTryAgainWarning = false;
int tryAgainTimer = 0;
std::vector<std::pair<int, int>> undoStack;
int numberOfTries = 0;
int highScore;
const std::string highScoreFile = "highscore.txt";

// SOLVE!
bool solveQueens(int col);
void autoSolve();
void highlightSquare(int row, int col);
void clearHighlight();

// more global variables
bool isSolving = false;
int currentSolveCol = 0;
int solveDelay = 500; // milliseconds
bool isComputerSolved = false;
bool winSoundPlayed = false;

// Animation properties
const int ANIMATION_DURATION = 500; // milliseconds
bool isAnimating = false;
float animationStartTime;
std::pair<int, int> animationStartPos;
std::pair<int, int> animationEndPos;

// Global variables for OpenAL
ALCdevice *device;
ALCcontext *context;
ALuint source;
ALuint source2;
ALuint buffer;
ALuint buffer2;
int main(int argc, char **argv)
{
    // Initialize OpenAL
    initOpenAL();

    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE); // Enable anti-aliasing GLUT_MULTISAMPLE
    glEnable(GL_MULTISAMPLE);

    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("3D 8-Queen Chess");

    loadHighScore();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    // Add multiple light sources with different colors:
    glEnable(GL_LIGHT1);
    GLfloat light1_position[] = {-1.0, 1.0, 0.5, 0.0};
    GLfloat light1_diffuse[] = {0.5, 0.5, 1.0, 1.0};
    glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);

    glEnable(GL_LIGHT2);
    GLfloat light2_position[] = {0.5, -0.5, 1.0, 0.0};
    GLfloat light2_diffuse[] = {1.0, 0.5, 0.5, 1.0};
    glLightfv(GL_LIGHT2, GL_POSITION, light2_position);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, light2_diffuse);

    loadTextures();
    if (!loadQueenModel())
    {
        std::cerr << "Failed to load queen model." << std::endl;
        cleanupOpenAL();
        return 1;
    }

    glutMainLoop();

    // Cleanup OpenAL resources
    cleanupOpenAL();

    return 0;
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Set up camera
    gluLookAt(
        cameraDistance * sin(cameraAngleY * M_PI / 180.0) * cos(cameraAngleX * M_PI / 180.0),
        cameraDistance * sin(cameraAngleX * M_PI / 180.0) + tableTopHeight - 0.5f,
        cameraDistance * cos(cameraAngleY * M_PI / 180.0) * cos(cameraAngleX * M_PI / 180.0),
        0.0, tableTopHeight - thickness, 0.0,
        0.0, 1.0, 0.0);

    // Draw the skybox
    glDisable(GL_DEPTH_TEST);
    drawRoom();
    glEnable(GL_DEPTH_TEST);

    // Draw other objects
    drawGroundPlane();
    drawTable();
    drawChessboard();
    drawGuide();

    // Draw queens
    for (size_t i = 0; i < queens.size(); ++i)
    {
        float x = (queens[i].second - BOARD_SIZE / 2 + 0.5f) * SQUARE_SIZE;
        float z = (queens[i].first - BOARD_SIZE / 2 + 0.5f) * SQUARE_SIZE;
        drawQueen(x, 0, z);
    }

    // Draw animated queen if animating
    if (isAnimating)
    {
        float currentTime = glutGet(GLUT_ELAPSED_TIME);
        float t = (currentTime - animationStartTime) / ANIMATION_DURATION;
        t = std::min(t, 1.0f);

        float startX = (animationStartPos.second - BOARD_SIZE / 2 + 0.5f) * SQUARE_SIZE;
        float startZ = (animationStartPos.first - BOARD_SIZE / 2 + 0.5f) * SQUARE_SIZE;
        float endX = (animationEndPos.second - BOARD_SIZE / 2 + 0.5f) * SQUARE_SIZE;
        float endZ = (animationEndPos.first - BOARD_SIZE / 2 + 0.5f) * SQUARE_SIZE;

        float x = startX + t * (endX - startX);
        float z = startZ + t * (endZ - startZ);
        float y = SQUARE_SIZE / 2.0f + sin(t * M_PI) * SQUARE_SIZE / 2.0f;

        drawQueen(x, y, z);
    }

    drawScore();

    if (isSolving)
    {
        // Highlight the current square being considered
        if (!queens.empty())
        {
            int row = queens.back().first;
            int col = queens.back().second;
            highlightSquare(row, col);
        }

        // Display "Solving..." message
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.0f, 0.5f, 1.0f);
        renderBitmapString(-1.0f, tableTopHeight + 2 * SQUARE_SIZE, 0.0f, GLUT_BITMAP_HELVETICA_18, "Solving...");
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
    }
    else if (gameWon)
    {
        float messageX = -1.0f;
        float messageY = tableTopHeight + 2 * SQUARE_SIZE;
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.2f, 1.0f, 0.2f);
        renderBitmapString(messageX, messageY, 0.0f, GLUT_BITMAP_HELVETICA_18, "Congratulations! You solved the 8-queen puzzle!");
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
    }

    if (showTryAgainWarning)
    {
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        if (currentTime - tryAgainTimer < 2000)
        {
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);
            glColor3f(1.0f, 0.0f, 0.0f);
            renderBitmapString(-1.6f, tableTopHeight + thickness + 0.5f, 0.0f, GLUT_BITMAP_HELVETICA_18, "Invalid move. Try again.");
            glEnable(GL_LIGHTING);
            glEnable(GL_TEXTURE_2D);
        }
        else
        {
            showTryAgainWarning = false;
        }
    }

    glutSwapBuffers();
}

void drawRoom()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Floor: Refined marble tiles for a sleek modern look
    glBegin(GL_QUADS);
    glColor3f(0.95f, 0.95f, 0.95f); // Light marble texture
    for (float i = -10.0f; i < 10.0f; i += 1.0f)
    {
        for (float j = -10.0f; j < 10.0f; j += 1.0f)
        {
            glVertex3f(i, 0.0f, j);
            glVertex3f(i + 1.0f, 0.0f, j);
            glVertex3f(i + 1.0f, 0.0f, j + 1.0f);
            glVertex3f(i, 0.0f, j + 1.0f);
        }
    }
    glEnd();

    // Back Wall: Modern textured wall with enhanced metallic strips
    glBegin(GL_QUADS);
    glColor3f(0.85f, 0.85f, 0.85f); // Smooth, light gray
    glVertex3f(-10.0f, 0.0f, 10.0f);
    glVertex3f(10.0f, 0.0f, 10.0f);
    glVertex3f(10.0f, 5.0f, 10.0f);
    glVertex3f(-10.0f, 5.0f, 10.0f);
    glEnd();

    // Refined metallic strips for the back wall
    glColor3f(0.3f, 0.3f, 0.3f); // Darker metallic strips
    for (float y = 0.6f; y < 5.0f; y += 0.8f)
    {
        glBegin(GL_QUADS);
        glVertex3f(-10.0f, y, 9.99f);
        glVertex3f(10.0f, y, 9.99f);
        glVertex3f(10.0f, y + 0.05f, 9.99f);
        glVertex3f(-10.0f, y + 0.05f, 9.99f);
        glEnd();
    }

    // Left Wall: Complex design with paneling and texture
    glBegin(GL_QUADS);
    glColor3f(0.85f, 0.85f, 0.85f); // Light gray base for panels
    glVertex3f(-10.0f, 0.0f, -10.0f);
    glVertex3f(-10.0f, 0.0f, 10.0f);
    glVertex3f(-10.0f, 5.0f, 10.0f);
    glVertex3f(-10.0f, 5.0f, -10.0f);
    glEnd();

    // Paneling details on the left wall
    for (float y = 0.5f; y < 5.0f; y += 1.0f)
    {
        for (float z = -10.0f; z < 10.0f; z += 3.0f)
        {
            glBegin(GL_QUADS);
            glColor3f(0.75f, 0.75f, 0.75f); // Slightly darker panel sections
            glVertex3f(-9.99f, y, z);
            glVertex3f(-9.99f, y + 0.8f, z);
            glVertex3f(-9.99f, y + 0.8f, z + 2.5f);
            glVertex3f(-9.99f, y, z + 2.5f);
            glEnd();
        }
    }

    // Right Wall: Complex paneling and texture like the left wall
    glBegin(GL_QUADS);
    glColor3f(0.85f, 0.85f, 0.85f); // Light gray base
    glVertex3f(10.0f, 0.0f, -10.0f);
    glVertex3f(10.0f, 0.0f, 10.0f);
    glVertex3f(10.0f, 5.0f, 10.0f);
    glVertex3f(10.0f, 5.0f, -10.0f);
    glEnd();

    // Paneling details on the right wall
    for (float y = 0.5f; y < 5.0f; y += 1.0f)
    {
        for (float z = -10.0f; z < 10.0f; z += 3.0f)
        {
            glBegin(GL_QUADS);
            glColor3f(0.75f, 0.75f, 0.75f); // Slightly darker panel sections
            glVertex3f(9.99f, y, z);
            glVertex3f(9.99f, y + 0.8f, z);
            glVertex3f(9.99f, y + 0.8f, z + 2.5f);
            glVertex3f(9.99f, y, z + 2.5f);
            glEnd();
        }
    }

    // Front Wall: Improved modern window design with enhanced outside view
    glBegin(GL_QUADS);
    glColor3f(0.75f, 0.75f, 0.75f); // Metallic light gray wall
    glVertex3f(-10.0f, 0.0f, -10.0f);
    glVertex3f(10.0f, 0.0f, -10.0f);
    glVertex3f(10.0f, 5.0f, -10.0f);
    glVertex3f(-10.0f, 5.0f, -10.0f);
    glEnd();

    // Smaller Window Glass: Refined with more transparency and depth
    glColor4f(0.2f, 0.5f, 0.8f, 0.5f); // Light blue-tinted glass
    glBegin(GL_QUADS);
    glVertex3f(-4.0f, 1.0f, -9.99f);
    glVertex3f(4.0f, 1.0f, -9.99f);
    glVertex3f(4.0f, 2.5f, -9.99f); // Smaller height
    glVertex3f(-4.0f, 2.5f, -9.99f);
    glEnd();

    // More Detailed Window Frame: Sleeker metallic frame with added design
    glColor3f(0.3f, 0.3f, 0.3f); // Dark metallic frame
    glBegin(GL_QUADS);
    glVertex3f(-4.1f, 0.9f, -9.98f);
    glVertex3f(4.1f, 0.9f, -9.98f);
    glVertex3f(4.1f, 3.6f, -9.98f);
    glVertex3f(-4.1f, 3.6f, -9.98f);
    glEnd();

    // Outside View: Enhanced cityscape with more details
    glBegin(GL_QUADS);
    glColor3f(0.5f, 0.6f, 0.7f); // Skyline with refined details
    glVertex3f(-4.0f, 1.0f, -9.97f);
    glVertex3f(4.0f, 1.0f, -9.97f);
    glVertex3f(4.0f, 3.0f, -9.97f);
    glVertex3f(-4.0f, 3.0f, -9.97f);
    glEnd();

    // Ceiling: Modern lighting and design
    glBegin(GL_QUADS);
    glColor3f(0.9f, 0.9f, 0.9f); // Bright white ceiling
    glVertex3f(-10.0f, 5.0f, -10.0f);
    glVertex3f(10.0f, 5.0f, -10.0f);
    glVertex3f(10.0f, 5.0f, 10.0f);
    glVertex3f(-10.0f, 5.0f, 10.0f);
    glEnd();

    // Modern Ceiling Lights
    glColor3f(1.0f, 1.0f, 0.8f); // Warm light color
    for (float x = -4.0f; x <= 4.0f; x += 2.0f)
    {
        glBegin(GL_QUADS);
        glVertex3f(x - 0.2f, 5.0f, -5.0f);
        glVertex3f(x + 0.2f, 5.0f, -5.0f);
        glVertex3f(x + 0.2f, 5.0f, -4.5f);
        glVertex3f(x - 0.2f, 5.0f, -4.5f);
        glEnd();
    }

    // Upright Pillars: Clearly defined with elegant design
    glColor3f(0.8f, 0.8f, 0.8f); // Light gray for pillars
    for (float x = -8.0f; x <= 8.0f; x += 8.0f)
    {
        glPushMatrix();
        glTranslatef(x, 0.0f, 10.0f);
        GLUquadric *quadric = gluNewQuadric();
        gluCylinder(quadric, 0.3f, 0.3f, 5.0f, 32, 32); // Pillar height and width
        glPopMatrix();
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w / (float)h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void resetBoard()
{
    queens.clear();
    numberOfTries = 0;
    undoStack.clear();
    showTryAgainWarning = false;
    winSoundPlayed = false;  // Ensure win sound is reset
    gameWon = false;         // Ensure gameWon is reset
    isComputerSolved = false;
    isSolving = false;       // Ensure auto-solve state is reset
    isRotating = false;
    isAnimating = false;     // Stop any ongoing animation
    loadHighScore();         // Load high score when the board is reset
    glutPostRedisplay();     // Redraw the scene
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'r':
    case 'R':
        resetBoard();
        break;
    case 27: // ESC key
        exit(0);
        break;
    case 'u':
    case 'U':
        if (!undoStack.empty())
        {
            queens.pop_back();
            undoStack.pop_back();
            gameWon = false;
            glutPostRedisplay();
        }
        break;
    case 's':
    case 'S':
        if (!isSolving)
        {
            autoSolve();
        }
        break;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
    // if (gameWon) return; // Stop input after 8 queens
    if (button == GLUT_LEFT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            isRotating = true;
            lastMouseX = x;
            lastMouseY = y;
        }
        else
        {
            isRotating = false;
        }
    }
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP && !isRotating)
    {
        GLint viewport[4];
        GLdouble modelview[16], projection[16];
        GLfloat winX, winY, winZ;
        GLdouble posX, posY, posZ;

        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
        glGetDoublev(GL_PROJECTION_MATRIX, projection);

        winX = (float)x;
        winY = (float)viewport[3] - (float)y;
        glReadPixels(x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);

        gluUnProject(winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);

        int row = int((posZ / SQUARE_SIZE) + BOARD_SIZE / 2);
        int col = int((posX / SQUARE_SIZE) + BOARD_SIZE / 2);

        if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE)
        {
            if (isValidMove(row, col))
            {
                placeQueen(row, col);
                if (queens.size() == 8)
                {
                    std::cout << "Congratulations! You solved the 8-queen puzzle!" << std::endl;
                }
            }
            else
            {
                numberOfTries++; // Increment the number of tries on invalid move
                std::cout << "Invalid move. Try again." << std::endl;
                showTryAgainWarning = true;
                tryAgainTimer = glutGet(GLUT_ELAPSED_TIME);
            }
        }
    }
    glutPostRedisplay();
}

void motion(int x, int y)
{
    if (isRotating)
    {
        int deltaX = x - lastMouseX;
        int deltaY = y - lastMouseY;

        cameraAngleY += deltaX * 0.5f;
        cameraAngleX += deltaY * 0.5f;

        if (cameraAngleX > 89.0f)
            cameraAngleX = 89.0f;
        if (cameraAngleX < -89.0f)
            cameraAngleX = -89.0f;

        lastMouseX = x;
        lastMouseY = y;

        glutPostRedisplay();
    }
}

void drawChessboard()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, chessboardTexture);

    glPushMatrix();
    glTranslatef(-BOARD_SIZE * SQUARE_SIZE / 2, tableTopHeight + 0.12f, -BOARD_SIZE * SQUARE_SIZE / 2); // Adjust height
    glBegin(GL_QUADS);

    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(0, 0, 0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(BOARD_SIZE * SQUARE_SIZE, 0, 0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(BOARD_SIZE * SQUARE_SIZE, 0, BOARD_SIZE * SQUARE_SIZE);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(0, 0, BOARD_SIZE * SQUARE_SIZE);
    glEnd();

    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
}

// Function to draw the queen model
void drawQueen(float x, float y, float z)
{
    glPushMatrix();
    glTranslatef(x, y + tableTopHeight + thickness + 0.15f, z);

    float scale = SQUARE_SIZE / 1.5f;
    glScalef(scale, scale, scale);

    // Bind the texture
    if (!queenTextures.empty())
    {
        glBindTexture(GL_TEXTURE_2D, queenTextures[0]); // Use the first texture for simplicity
    }

    // Enable texture mapping
    glEnable(GL_TEXTURE_2D);

    // Set the color to white
    GLfloat material_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material_color);

    // Draw the queen model
    for (size_t i = 0; i < queenShapes.size(); i++)
    {
        glBegin(GL_TRIANGLES);
        for (size_t f = 0; f < queenShapes[i].mesh.indices.size() / 3; f++)
        {
            for (size_t v = 0; v < 3; v++)
            {
                tinyobj::index_t idx = queenShapes[i].mesh.indices[3 * f + v];
                glNormal3f(
                    queenAttrib.normals[3 * idx.normal_index + 0],
                    queenAttrib.normals[3 * idx.normal_index + 1],
                    queenAttrib.normals[3 * idx.normal_index + 2]);

                if (idx.texcoord_index >= 0) // Check if texture coordinates are available
                {
                    glTexCoord2f(
                        queenAttrib.texcoords[2 * idx.texcoord_index + 0],
                        queenAttrib.texcoords[2 * idx.texcoord_index + 1]);
                }

                glVertex3f(
                    queenAttrib.vertices[3 * idx.vertex_index + 0],
                    queenAttrib.vertices[3 * idx.vertex_index + 1],
                    queenAttrib.vertices[3 * idx.vertex_index + 2]);
            }
        }
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}

// Function to initialize OpenAL
void initOpenAL()
{
    device = alcOpenDevice(nullptr);
    if (!device)
    {
        std::cerr << "Failed to open OpenAL device" << std::endl;
        return;
    }

    context = alcCreateContext(device, nullptr);
    if (!context)
    {
        std::cerr << "Failed to create OpenAL context" << std::endl;
        alcCloseDevice(device);
        return;
    }

    if (!alcMakeContextCurrent(context))
    {
        std::cerr << "Failed to make OpenAL context current" << std::endl;
        alcDestroyContext(context);
        alcCloseDevice(device);
        return;
    }

    alGenSources(1, &source);
    alGenSources(1, &source2);
    alGenBuffers(1, &buffer);
    alGenBuffers(1, &buffer2);

    // Load placement sound
    {
        std::vector<uint8_t> soundData;
        ALsizei size, freq;
        ALenum format;
        loadWAV("resources/queen_placed.wav", soundData, size, freq, format);
        alBufferData(buffer, format, soundData.data(), size, freq);
    }

    // Load win sound
    {
        std::vector<uint8_t> soundData;
        ALsizei size, freq;
        ALenum format;
        loadWAV("resources/win_sound.wav", soundData, size, freq, format);
        alBufferData(buffer2, format, soundData.data(), size, freq);
    }
}

// Function to load WAV file
void loadWAV(const char *filename, std::vector<uint8_t> &soundData, ALsizei &size, ALsizei &freq, ALenum &format)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open WAV file: " << filename << std::endl;
        return;
    }

    // Read WAV header
    char header[44];
    if (!file.read(header, sizeof(header)))
    {
        std::cerr << "Failed to read WAV header" << std::endl;
        return;
    }

    // Check WAV format
    if (std::strncmp(header, "RIFF", 4) != 0 || std::strncmp(header + 8, "WAVE", 4) != 0)
    {
        std::cerr << "Invalid WAV file format" << std::endl;
        return;
    }

    // Extract audio format information
    uint16_t audioFormat = *reinterpret_cast<uint16_t *>(header + 20);
    uint16_t numChannels = *reinterpret_cast<uint16_t *>(header + 22);
    uint32_t sampleRate = *reinterpret_cast<uint32_t *>(header + 24);
    uint16_t bitsPerSample = *reinterpret_cast<uint16_t *>(header + 34);

    // Set OpenAL format
    if (numChannels == 1)
    {
        if (bitsPerSample == 8)
            format = AL_FORMAT_MONO8;
        else if (bitsPerSample == 16)
            format = AL_FORMAT_MONO16;
    }
    else if (numChannels == 2)
    {
        if (bitsPerSample == 8)
            format = AL_FORMAT_STEREO8;
        else if (bitsPerSample == 16)
            format = AL_FORMAT_STEREO16;
    }
    else
    {
        std::cerr << "Unsupported WAV format" << std::endl;
        return;
    }

    // Read audio data
    uint32_t dataSize = *reinterpret_cast<uint32_t *>(header + 40);
    soundData.resize(dataSize);
    if (!file.read(reinterpret_cast<char *>(soundData.data()), dataSize))
    {
        std::cerr << "Failed to read WAV data" << std::endl;
        return;
    }

    size = dataSize;
    freq = sampleRate;
}

// Function to load and play sound
void playSound(const char *filename)
{
    alSourceStop(source); // Stop any current playback
    std::vector<uint8_t> soundData;
    ALsizei size, freq;
    ALenum format;
    loadWAV(filename, soundData, size, freq, format);

    alBufferData(buffer, format, soundData.data(), size, freq);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcef(source, AL_GAIN, 1.0f); // Set volume to maximum
    alSourcePlay(source);
}

// Function to play placement sound
void playPlacementSound()
{
    alSourceStop(source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcef(source, AL_GAIN, 1.0f);
    alSourcePlay(source);
}

// Function to play win sound
void playWinSound()
{
    alSourceStop(source2);
    alSourcei(source2, AL_BUFFER, buffer2);
    alSourcef(source2, AL_GAIN, 1.0f);
    alSourcePlay(source2);
}

// Modified placeQueen function
void placeQueen(int row, int col)
{
    if (gameWon)
        return; // Stop input if game is won

    float x = (col - BOARD_SIZE / 2 + 0.5f) * SQUARE_SIZE - 0.085f; // Adjust for edge
    float z = (row - BOARD_SIZE / 2 + 0.5f) * SQUARE_SIZE - 0.085f; // Adjust for edge
    float y = (SQUARE_SIZE / 2.0f);                                 // Adjust for the height of the table and the chessboard

    // Start animation
    isAnimating = true;
    animationStartTime = glutGet(GLUT_ELAPSED_TIME);
    animationStartPos = queens.empty() ? std::make_pair(row, col) : queens.back();
    animationEndPos = std::make_pair(row, col);

    // The queen will be added to the queens vector after the animation completes

    undoStack.push_back({row, col});
    glutPostRedisplay();

    // Start the animation timer
    glutTimerFunc(16, animateQueen, 0); // 60 FPS

    // Play queen placed sound effect
    playPlacementSound();

    // Check if the game is won after placing the queen
    if (queens.size() == 7 && isValidMove(row, col))
    {
        gameWon = true;
        winSoundPlayed = false; // Reset the flag
    }
}

// Function to clean up OpenAL resources
void cleanupOpenAL()
{
    alDeleteSources(1, &source);
    alDeleteSources(1, &source2);
    alDeleteBuffers(1, &buffer);
    alDeleteBuffers(1, &buffer2);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

void animateQueen(int value)
{
    float currentTime = glutGet(GLUT_ELAPSED_TIME);
    float t = (currentTime - animationStartTime) / ANIMATION_DURATION;

    if (t >= 1.0f)
    {
        // Animation complete
        isAnimating = false;
        queens.push_back(animationEndPos);

        if (queens.size() == 8)
        {
            gameWon = true;
            if (!isComputerSolved) // Only update high score if not solved by computer
            {
                if (numberOfTries < highScore || highScore == 0)
                {
                    highScore = numberOfTries;
                    saveHighScore();
                }
            }
            isComputerSolved = false; // Reset for the next game
            // Play win sound
            if (!winSoundPlayed)
            {
                playWinSound();
                winSoundPlayed = true;
            }
        }
    }
    else
    {
        // Continue animation
        glutTimerFunc(16, animateQueen, 0);
    }

    glutPostRedisplay();
}

void renderBitmapString(float x, float y, float z, void *font, const char *string)
{
    const char *c;
    glRasterPos3f(x, y, z);
    for (c = string; *c != '\0'; c++)
    {
        glutBitmapCharacter(font, *c);
    }
}

// drawGuide function
void drawGuide()
{
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 1.0f, 1.0f); // Bright cyan color

    std::string guideText[] = {
        "Mouse LMB + Drag: Rotate",
        "Left click to place a queen. Press 'U': Undo",
        "Press 'R': Restart",
        "Press 'S': Auto-solve",
        "Press 'ESC': Exit"};

    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);

    int yOffset = 20;
    for (const auto &line : guideText)
    {
        renderBitmapString(10, WINDOW_HEIGHT - yOffset, 0, GLUT_BITMAP_HELVETICA_18, line.c_str());
        yOffset += 20;
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING);
}

void drawScore()
{
    std::ostringstream scoreText;
    scoreText << "Queens placed: " << queens.size();
    scoreText << "\nNumber of tries: " << numberOfTries;

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.2f); //  color for the score
    renderBitmapString(-1.6f, tableTopHeight + thickness + 0.3f, 0.0f, GLUT_BITMAP_HELVETICA_18, scoreText.str().c_str());

    // Display high score at the top right
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);

    glColor3f(0.5f, 1.0f, 0.2f); // color for the high score
    std::string highScoreText = "Best Score: " + std::to_string(highScore);
    renderBitmapString(WINDOW_WIDTH - 150, WINDOW_HEIGHT - 20, 0.0f, GLUT_BITMAP_HELVETICA_18, highScoreText.c_str());

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING);
}

// Load Textures Function
void loadTextures()
{
    int width, height, nrChannels;
    unsigned char *data;

    // Chessboard Texture
    if (!fileExists("/mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/resources/chessboard.jpg"))
    {
        std::cerr << "Chessboard texture file not found!" << std::endl;
        return;
    }
    glGenTextures(1, &chessboardTexture);
    glBindTexture(GL_TEXTURE_2D, chessboardTexture);
    data = stbi_load("/mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/resources/chessboard.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    else
    {
        std::cerr << "Failed to load chessboard texture" << std::endl;
    }
    stbi_image_free(data);

    // Table Texture
    if (!fileExists("/mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/resources/wood.jpg"))
    {
        std::cerr << "Table texture file not found!" << std::endl;
        return;
    }
    glGenTextures(1, &tableTexture);
    glBindTexture(GL_TEXTURE_2D, tableTexture);
    data = stbi_load("/mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/resources/wood.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    else
    {
        std::cerr << "Failed to load table texture" << std::endl;
    }
    stbi_image_free(data);

    // Ground Texture
    if (!fileExists("/mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/resources/ground.jpg"))
    {
        std::cerr << "Ground texture file not found!" << std::endl;
        return;
    }
    glGenTextures(1, &groundTexture);
    glBindTexture(GL_TEXTURE_2D, groundTexture);
    data = stbi_load("/mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/resources/ground.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    else
    {
        std::cerr << "Failed to load ground texture" << std::endl;
    }
    stbi_image_free(data);

    // Ground Texture
    if (!fileExists("/mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/resources/ground.jpg"))
    {
        std::cerr << "Ground texture file not found!" << std::endl;
        return;
    }
    glGenTextures(1, &groundTexture);
    glBindTexture(GL_TEXTURE_2D, groundTexture);
    data = stbi_load("/mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/resources/ground.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    else
    {
        std::cerr << "Failed to load ground texture" << std::endl;
    }
    stbi_image_free(data);
}

// Function to load the queen model and textures
bool loadQueenModel()
{
    // Path to the model and material files
    std::string inputfile = "/mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/resources/Queen/Queen_Chess_Piece_0825091405.obj";
    std::string mtlBaseDir = "/mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/resources/Queen/";

    std::string err, warn;
    // Load the model and materials
    bool ret = tinyobj::LoadObj(&queenAttrib, &queenShapes, &queenMaterials, &warn, &err, inputfile.c_str(), mtlBaseDir.c_str());

    // Output warnings and errors if any
    if (!warn.empty())
    {
        std::cerr << warn << std::endl;
    }
    if (!err.empty())
    {
        std::cerr << err << std::endl;
    }
    if (!ret)
    {
        std::cerr << "Failed to load queen model." << std::endl;
        return false;
    }

    // Reserve space for texture IDs
    queenTextures.resize(queenMaterials.size());

    // Load textures for each material
    for (size_t i = 0; i < queenMaterials.size(); i++)
    {
        if (!queenMaterials[i].diffuse_texname.empty())
        {
            std::string texPath = mtlBaseDir + queenMaterials[i].diffuse_texname;
            std::cout << "Loading texture from: " << texPath << std::endl;

            glGenTextures(1, &queenTextures[i]); // Generate texture ID
            glBindTexture(GL_TEXTURE_2D, queenTextures[i]);

            int width, height, nrChannels;
            unsigned char *data = stbi_load(texPath.c_str(), &width, &height, &nrChannels, 0);

            if (data)
            {
                GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
                // Load texture data
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                gluBuild2DMipmaps(GL_TEXTURE_2D, (nrChannels == 3 ? GL_RGB : GL_RGBA), width, height, (nrChannels == 3 ? GL_RGB : GL_RGBA), GL_UNSIGNED_BYTE, data);

                // Set texture parameters
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                stbi_image_free(data);
            }
            else
            {
                std::cerr << "Failed to load texture: " << texPath << std::endl;
                stbi_image_free(data);
            }
        }
    }

    return true;
}

void drawTable()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tableTexture);

    // Draw table top as a thicker rectangular box
    glPushMatrix();
    glTranslatef(0, tableTopHeight + thickness / 2, 0); // Adjust to place the table correctly
    glScalef(tableWidth, thickness, tableLength);
    glBegin(GL_QUADS);

    // Top face
    glNormal3f(0, 1, 0);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-0.5f, 0.5f, 0.5f);

    // Bottom face
    glNormal3f(0, -1, 0);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-0.5f, -0.5f, 0.5f);

    // Front face
    glNormal3f(0, 0, 1);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-0.5f, 0.5f, 0.5f);

    // Back face
    glNormal3f(0, 0, -1);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-0.5f, 0.5f, -0.5f);

    // Left face
    glNormal3f(-1, 0, 0);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-0.5f, 0.5f, -0.5f);

    // Right face
    glNormal3f(1, 0, 0);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(0.5f, 0.5f, -0.5f);

    glEnd();
    glPopMatrix();

    // Draw table legs
    float legWidth = 0.1f;
    float legHeight = tableTopHeight + thickness; // Adjusted to table top height
    float legPositions[4][3] = {
        {-tableWidth / 2 + legWidth / 2, tableTopHeight, -tableLength / 2 + legWidth / 2},
        {tableWidth / 2 - legWidth / 2, tableTopHeight, -tableLength / 2 + legWidth / 2},
        {tableWidth / 2 - legWidth / 2, tableTopHeight, tableLength / 2 - legWidth / 2},
        {-tableWidth / 2 + legWidth / 2, tableTopHeight, tableLength / 2 - legWidth / 2}};

    for (int i = 0; i < 4; ++i)
    {
        glPushMatrix();
        glTranslatef(legPositions[i][0], legPositions[i][1] - legHeight / 2, legPositions[i][2]);
        glScalef(legWidth, legHeight, legWidth);
        glBegin(GL_QUADS);

        // Top face
        glNormal3f(0, 1, 0);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-0.5f, 0.5f, 0.5f);

        // Bottom face
        glNormal3f(0, -1, 0);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-0.5f, -0.5f, 0.5f);

        // Front face
        glNormal3f(0, 0, 1);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-0.5f, 0.5f, 0.5f);

        // Back face
        glNormal3f(0, 0, -1);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(0.5f, 0.5f, -0.5f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-0.5f, 0.5f, -0.5f);

        // Left face
        glNormal3f(-1, 0, 0);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-0.5f, -0.5f, 0.5f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(-0.5f, 0.5f, 0.5f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-0.5f, 0.5f, -0.5f);

        // Right face
        glNormal3f(1, 0, 0);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(0.5f, -0.5f, -0.5f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(0.5f, -0.5f, 0.5f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(0.5f, 0.5f, 0.5f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(0.5f, 0.5f, -0.5f);

        glEnd();
        glPopMatrix();
    }

    glDisable(GL_TEXTURE_2D);
}

void drawGroundPlane()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, groundTexture);

    float groundSize = 50.0f;
    float repeatFactor = groundSize / 2.0f; // Adjust this to control the tiling

    glPushMatrix();
    glTranslatef(0, -0.15f, 0);
    glScalef(groundSize, 0.1f, groundSize);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1, 0, -1);
    glTexCoord2f(repeatFactor, 0.0f);
    glVertex3f(1, 0, -1);
    glTexCoord2f(repeatFactor, repeatFactor);
    glVertex3f(1, 0, 1);
    glTexCoord2f(0.0f, repeatFactor);
    glVertex3f(-1, 0, 1);
    glEnd();
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
}

bool fileExists(const std::string &filename)
{
    std::ifstream file(filename);
    return file.good();
}

bool isValidMove(int row, int col)
{
    //  (row, col) is valid
    for (const auto &queen : queens)
    {
        int qRow = queen.first;
        int qCol = queen.second;
        if (qRow == row || qCol == col || abs(qRow - row) == abs(qCol - col))
        {
            return false;
        }
    }
    return true;
}

// Save
//  Function to load high score from a file
void loadHighScore()
{
    std::ifstream inputFile("/mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/resources/highscore.txt");
    if (inputFile.is_open())
    {
        inputFile >> highScore;
        inputFile.close();
    }
    else
    {
        highScore = 0;
    }
}

// Function to save high score to a file
void saveHighScore()
{
    std::ofstream outputFile("/mnt/c/Users/Leul/Downloads/Telegram Desktop/Senior/Sem 1/CS489 - CG/Proj/8 Queen 6.0/resources/highscore.txt");
    if (outputFile.is_open())
    {
        outputFile << highScore;
        outputFile.close();
    }
}

bool solveQueens(int col)
{
    if (col >= BOARD_SIZE)
    {
        return true; // All queens are placed successfully
    }

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        if (isValidMove(row, col))
        {
            queens.push_back({row, col});

            if (solveQueens(col + 1))
            {
                return true;
            }

            // If placing queen in the current position doesn't lead to a solution,
            // remove the queen and try the next row
            queens.pop_back();
        }
    }

    return false; // No solution found for this column
}

void autoSolve()
{
    resetBoard();
    isSolving = true;
    isComputerSolved = true;

    if (solveQueens(0))
    {
        gameWon = true;
        playWinSound();
        winSoundPlayed = true;
    }

    isSolving = false;
    glutPostRedisplay();
}

void highlightSquare(int row, int col)
{
    glPushMatrix();
    glTranslatef(-SQUARE_SIZE * BOARD_SIZE / 2 + SQUARE_SIZE / 2 + col * SQUARE_SIZE,
                 0.01f, // Slightly above the board
                 -SQUARE_SIZE * BOARD_SIZE / 2 + SQUARE_SIZE / 2 + row * SQUARE_SIZE);
    glColor4f(1.0f, 1.0f, 0.0f, 0.5f); // Yellow with 50% opacity
    glBegin(GL_QUADS);
    glVertex3f(-SQUARE_SIZE / 2, 0, -SQUARE_SIZE / 2);
    glVertex3f(SQUARE_SIZE / 2, 0, -SQUARE_SIZE / 2);
    glVertex3f(SQUARE_SIZE / 2, 0, SQUARE_SIZE / 2);
    glVertex3f(-SQUARE_SIZE / 2, 0, SQUARE_SIZE / 2);
    glEnd();
    glPopMatrix();
}

void clearHighlight()
{
    // This function doesn't need to do anything as the highlighting is temporary
    // and will be cleared when the board is redrawn
}

void delay(int milliseconds)
{
    clock_t start_time = clock();
    while (clock() < start_time + milliseconds)
        ;
}