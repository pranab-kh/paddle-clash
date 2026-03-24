#include "math3d.h"
#include "vaovbo.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <graphicslibrary.h>
#include <string>

// Adjust the viewport to match the new window size
int windowWidth;
int windowHeight;
int mouseMoved = 0;
bool firstMouseMove = true;
// Default rotation unit in degrees
const int rotationUnit = 5; 

int playerScore = 0;
int opponentScore = 0;
bool playerServing = true;  // Track whose turn it is to serve
float serveTimer = 0.0f;    // Timer for opponent serve delay
const float SERVE_DELAY = 1.0f;  // 1 second delay before opponent serves

// Helper function to determine who should serve
bool isPlayerServing() {
    int totalPoints = playerScore + opponentScore;
    // Every 2 points, switch server (0-1: player, 2-3: opponent, 4-5: player, etc.)
    return (totalPoints / 2) % 2 == 0;
}

// Player Paddle declared here so that mouseCallBack will be able to access it
float paddleRadius = 0.5f;
Paddle playerPaddle(paddleRadius, paddleRadius ,0, 0, 0.0f, 0);
#include "stb_easy_font.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

//Game state management
enum GameState {
    SERVING,
    PLAYING,
    RUNNING,
    PAUSED
};
enum PauseMenuOption
{
    Resume,
    New_Game,
    Quit
};
enum GameEndState
{
    IN_PROGRESS,
    PLAYER_WON,
    OPPONENT_WON
};

// Check win condition according to official table tennis rules
// First to 11 points wins, but must win by 2 points after 10-10 deuce
GameEndState checkWinCondition(int playerScore, int opponentScore) {
    // If both players are below 10, continue playing
    if(playerScore < 10 && opponentScore < 10) {
        return IN_PROGRESS;
    }
    
    // If one player reached 11 and opponent is below 10, that player wins
    if(playerScore >= 11 && opponentScore < 10) {
        return PLAYER_WON;
    }
    if(opponentScore >= 11 && playerScore < 10) {
        return OPPONENT_WON;
    }
    
    // Deuce situation: both at 10+ points, must win by 2
    if(playerScore >= 10 && opponentScore >= 10) {
        if(playerScore - opponentScore >= 2) return PLAYER_WON;
        if(opponentScore - playerScore >= 2) return OPPONENT_WON;
    }
    
    return IN_PROGRESS;
}
GameState gamestate = SERVING;    // Handles SERVING and PLAYING states
GameState state = RUNNING;         // Handles RUNNING and PAUSED states
PauseMenuOption selectedMenuOption = Resume;
GameEndState gameEndState = IN_PROGRESS;
bool escapeKeyPressed = false;

// Minimal header-only audio (miniaudio): looping background music from file.
static ma_engine gAudioEngine;
static ma_sound gBgmSound;
static ma_sound gHitSound;
static bool gAudioReady = false;

static bool initAudio() {
    if(ma_engine_init(NULL, &gAudioEngine) != MA_SUCCESS) {
        std::cout << "Audio engine init failed." << std::endl;
        return false;
    }

    const char* musicPath = "include/background_music.mp3";
    if(ma_sound_init_from_file(&gAudioEngine, musicPath, MA_SOUND_FLAG_STREAM, NULL, NULL, &gBgmSound) != MA_SUCCESS) {
        std::cout << "Could not load background music: " << musicPath << std::endl;
        ma_engine_uninit(&gAudioEngine);
        return false;
    }

    ma_sound_set_looping(&gBgmSound, MA_TRUE);
    ma_sound_set_volume(&gBgmSound, 0.35f);
    ma_sound_start(&gBgmSound);

    // Load collision sound
    const char* hitSoundPath = "include/ball_hit.mp3";
    if(ma_sound_init_from_file(&gAudioEngine, hitSoundPath, 0, NULL, NULL, &gHitSound) != MA_SUCCESS) {
        std::cout << "Could not load collision sound: " << hitSoundPath << std::endl;
    } else {
        ma_sound_set_volume(&gHitSound, 0.4f);
    }

    gAudioReady = true;
    std::cout << "Background music started." << std::endl;
    return true;
}

static void shutdownAudio() {
    if(gAudioReady) {
        ma_sound_uninit(&gHitSound);
        ma_sound_uninit(&gBgmSound);
        ma_engine_uninit(&gAudioEngine);
        gAudioReady = false;
    }
}

static void playBallHitSound() {
    if(gAudioReady) {
        ma_sound_seek_to_pcm_frame(&gHitSound, 0);
        ma_sound_start(&gHitSound);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    windowWidth = width;
    windowHeight = height;
}

//checks keyboard input each frame
void processInput(GLFWwindow* window, GameState& state, PauseMenuOption& selectedOption, GameEndState& endState, GameState& gamestate, Ball& ball, Paddle& player, Paddle& opponent, int& playerScore, int& opponentScore, bool& playerServing, float& serveTimer) {
    bool escapePressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    
    // Toggle pause on ESC (with debounce)
    if(escapePressed && !escapeKeyPressed) {
        if(state == RUNNING) {
            state = PAUSED;
            selectedOption = Resume;  // Reset to first option
        }
    }
    escapeKeyPressed = escapePressed;
    
    // Menu navigation when paused
    if(state == PAUSED) {
        bool upPressed = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
        bool downPressed = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
        bool enterPressed = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        
        static bool menuKeyPressed = false;
        
        // Navigate menu with arrow keys (with debounce)
        if((upPressed || downPressed) && !menuKeyPressed) {
            if(upPressed) {
                selectedOption = (selectedOption == Resume) ? Quit : (PauseMenuOption)(selectedOption - 1);
            } else if(downPressed) {
                selectedOption = (selectedOption == Quit) ? Resume : (PauseMenuOption)(selectedOption + 1);
            }
            menuKeyPressed = true;
        } else if(!upPressed && !downPressed) {
            menuKeyPressed = false;
        }
        
        // Select option with ENTER or SPACE
        if(enterPressed || spacePressed) {
            if(selectedOption == Resume) {
                state = RUNNING;
            } else if(selectedOption == New_Game) {
                // Reset game for new match
                playerScore = 0;
                opponentScore = 0;
                endState = IN_PROGRESS;
                gamestate = SERVING;
                state = RUNNING;
                playerServing = isPlayerServing();
                serveTimer = 0.0f;
                ball.outOfBounds = false;
                
                if(playerServing) {
                    ball.resetToServe(player);
                } else {
                    ball.resetToServe(opponent);
                }
                
                // Update window title with reset score
                std::string title = "Table Tennis  |  You: " + std::to_string(playerScore) + "  AI: " + std::to_string(opponentScore);
                glfwSetWindowTitle(window, title.c_str());
            } else if(selectedOption == Quit) {
                glfwSetWindowShouldClose(window, true);
            }
        }
    }
}
//shaders
//##--no perspective shader--##
// const char* vertexShaderSource = R"(
//     #version 330 core
//     layout(location = 0) in vec3 aPos;  

//     void main() {
//         gl_Position = vec4(aPos, 1.0);
//     }
// )";

const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;

    uniform mat4 mvp;

    void main() {
        gl_Position = mvp * vec4(aPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec4 color;

    void main() {
        FragColor = color;
    }
)";

// Shader compiler
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Shader compile error:\n" << infoLog << std::endl;
    }
    return shader;
}

// Links vertex and fragment shader together
unsigned int createShaderProgram(const char* vertSrc, const char* fragSrc) {
    unsigned int vert = compileShader(GL_VERTEX_SHADER, vertSrc);
    unsigned int frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "Shader link error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    return program;
}

void mousePosCallback(GLFWwindow* window, double posx, double posy){
    playerPaddle.mouseCurrentPos = Vec3(posx - windowWidth/2, -1 * (posy - windowHeight/2));

        if(firstMouseMove) {
        playerPaddle.mousePrevPos = playerPaddle.mouseCurrentPos;  
        firstMouseMove = false;                           
    }
    mouseMoved = 1;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if(gamestate == SERVING && playerServing) {
            gamestate = PLAYING;
        }
    }
}

void cursorEnterCallback(GLFWwindow* window, int entered) {
    if (entered) {
        // The cursor just entered the window
        // Reset the flag so the next mouse movement doesn't cause a warp.
        firstMouseMove = true;
    }
}

void keyPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
    // Only allow paddle movement when game is RUNNING
    if(state != RUNNING) {
        return;
    }
    
    // Rotation towards left
    if(key == GLFW_KEY_A && playerPaddle.rotation > -30)
    {
        playerPaddle.rotation -= rotationUnit;
    }
    // Rotation towards right
    else if(key == GLFW_KEY_D  && playerPaddle.rotation < 30)
    {
        playerPaddle.rotation += rotationUnit;
    }
    // std::cout<<"Rotation unit: "<<playerPaddle.rotation;
}

// Render a single menu button box
void renderMenuButton(float centerX, float centerY, float width, float height, 
                      bool isSelected, int colorLoc) {
    // Define box vertices in screen space (-1 to 1)
    GLfloat boxVertices[] = {
        centerX - width/2,  centerY - height/2,  0.0f,  // bottom-left
        centerX + width/2,  centerY - height/2,  0.0f,  // bottom-right
        centerX + width/2,  centerY + height/2,  0.0f,  // top-right
        centerX - width/2,  centerY + height/2,  0.0f   // top-left
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color: yellow if selected, white if not
    if(isSelected) {
        glUniform4f(colorLoc, 1.0f, 1.0f, 0.0f, 1.0f);  // Yellow
    } else {
        glUniform4f(colorLoc,0,0,0,1);  // Black
    }

    glDrawArrays(GL_LINE_LOOP, 0, 4);  // Draw box outline

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

// Render semi-transparent overlay background
void renderOverlay(int colorLoc, int mvpLocation) {
    // Set orthographic projection for 2D UI
    Mat4 orthoModel = identity();
    Mat4 orthoView = identity();
    Mat4 orthoProj = identity();
    Mat4 orthoMVP = orthoProj * orthoView * orthoModel;
    
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, orthoMVP.m);

    // Draw semi-transparent overlay (dark background)
    GLfloat overlayVertices[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };

    unsigned int overlayVAO, overlayVBO;
    glGenVertexArrays(1, &overlayVAO);
    glGenBuffers(1, &overlayVBO);

    glBindVertexArray(overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(overlayVertices), overlayVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUniform4f(colorLoc, 0, 0, 0, 0.3f);  // white, 50% opaque (50% transparent)
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);  // Draw filled rectangle

    glDeleteBuffers(1, &overlayVBO);
    glDeleteVertexArrays(1, &overlayVAO);
}

// Render simple UI text using stb_easy_font in normalized device coordinates.
void renderEasyText(const char* text, float centerX, float centerY, float scale,
                    int colorLoc, int screenWidth, int screenHeight) {
    static unsigned int textVAO = 0;
    static unsigned int textVBO = 0;

    if(textVAO == 0) {
        glGenVertexArrays(1, &textVAO);
        glGenBuffers(1, &textVBO);
    }

    char stbBuffer[99999];
    int quadCount = stb_easy_font_print(0.0f, 0.0f, const_cast<char*>(text), NULL, stbBuffer, sizeof(stbBuffer));
    if(quadCount <= 0) {
        return;
    }

    const float* stbVerts = reinterpret_cast<const float*>(stbBuffer);
    int textWidthPx = stb_easy_font_width(const_cast<char*>(text));
    float textHeightPx = 12.0f;  // stb_easy_font default glyph height

    float centerXPx = (screenWidth * 0.5f) + (centerX * screenWidth * 0.5f);
    float centerYPx = (screenHeight * 0.5f) - (centerY * screenHeight * 0.5f);

    float originXPx = centerXPx - (textWidthPx * scale * 0.5f);
    float originYPx = centerYPx - (textHeightPx * scale * 0.5f);

    std::vector<GLfloat> triVertices;
    triVertices.reserve(quadCount * 6 * 3);

    const int idx[6] = {0, 1, 2, 0, 2, 3};
    for(int q = 0; q < quadCount; ++q) {
        for(int k = 0; k < 6; ++k) {
            int v = q * 4 + idx[k];
            float px = originXPx + (stbVerts[v * 4 + 0] * scale);
            float py = originYPx + (stbVerts[v * 4 + 1] * scale);

            float ndcX = (px / static_cast<float>(screenWidth)) * 2.0f - 1.0f;
            float ndcY = 1.0f - (py / static_cast<float>(screenHeight)) * 2.0f;

            triVertices.push_back(ndcX);
            triVertices.push_back(ndcY);
            triVertices.push_back(0.0f);
        }
    }

    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, triVertices.size() * sizeof(GLfloat), triVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triVertices.size() / 3));

    glBindVertexArray(0);
}

// Render the entire pause menu with overlay + buttons + text labels
void renderPauseMenu(int colorLoc, int mvpLocation, PauseMenuOption selected, int screenWidth, int screenHeight, GameEndState endState) {
    // Draw the dark overlay first
    renderOverlay(colorLoc, mvpLocation);

    // Display status message at top
    const float statusScale = 3.0f;
    if(endState == PLAYER_WON) {
        renderEasyText("YOU WIN!", 0.0f, 0.5f, statusScale, colorLoc, screenWidth, screenHeight);
    } else if(endState == OPPONENT_WON) {
        renderEasyText("YOU LOST", 0.0f, 0.5f, statusScale, colorLoc, screenWidth, screenHeight);
    } else {
        renderEasyText("GAME PAUSED", 0.0f, 0.5f, statusScale, colorLoc, screenWidth, screenHeight);
    }

    // Draw three menu buttons (vertically stacked)
    float buttonWidth = 0.4f;
    float buttonHeight = 0.12f;

    renderMenuButton(0.0f,  0.2f, buttonWidth, buttonHeight, (selected == Resume),   colorLoc);
    renderMenuButton(0.0f, -0.1f, buttonWidth, buttonHeight, (selected == New_Game), colorLoc);
    renderMenuButton(0.0f, -0.4f, buttonWidth, buttonHeight, (selected == Quit),     colorLoc);
    
    // Larger text scale for better readability.
    const float labelScale = 2.6f;
    renderEasyText("RESUME",   0.0f,  0.2f, labelScale, colorLoc, screenWidth, screenHeight);
    renderEasyText("NEW GAME", 0.0f, -0.1f, labelScale, colorLoc, screenWidth, screenHeight);
    renderEasyText("QUIT",     0.0f, -0.4f, labelScale, colorLoc, screenWidth, screenHeight);
}

int main() {
    // Initialize glfw
    if(!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Table Tennis", NULL, NULL);
    if(window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mousePosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyPressCallback);
    glfwSetCursorEnterCallback(window, cursorEnterCallback);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    initAudio();

    windowWidth = 800;
    windowHeight = 600;
    glViewport(0, 0, windowWidth, windowHeight);

    GLfloat tableVertices[] = {
        -3.0f, 0.0f, -5.0f,
         3.0f, 0.0f, -5.0f,
        -3.0f, 0.0f,  5.0f,
         3.0f, 0.0f, -5.0f,   
         3.0f, 0.0f,  5.0f,
        -3.0f, 0.0f,  5.0f 
    };
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

// const GLfloat scale = 1.0;

    GLfloat lineVertices[] = {
        -3.0f, 0.01f, -0.05f, 
         3.0f, 0.01f, -0.05f, 
        -3.0f, 0.01f,  0.05f, 
         3.0f, 0.01f, -0.05f, 
         3.0f, 0.01f,  0.05f, 
        -3.0f, 0.01f,  0.05f  
    };

    GLfloat borderVertices[] = {
        -3.0f, 0.01f, -5.10f,  3.0f, 0.01f, -5.10f, -3.0f, 0.01f, -4.90f,
         3.0f, 0.01f, -5.10f,  3.0f, 0.01f, -4.90f, -3.0f, 0.01f, -4.90f,
        -3.0f, 0.01f,  4.90f,  3.0f, 0.01f,  4.90f, -3.0f, 0.01f,  5.10f,
         3.0f, 0.01f,  4.90f,  3.0f, 0.01f,  5.10f, -3.0f, 0.01f,  5.10f,
        -3.10f, 0.01f, -5.0f, -2.90f, 0.01f, -5.0f, -3.10f, 0.01f,  5.0f,
        -2.90f, 0.01f, -5.0f, -2.90f, 0.01f,  5.0f, -3.10f, 0.01f,  5.0f,
         2.90f, 0.01f, -5.0f,  3.10f, 0.01f, -5.0f,  2.90f, 0.01f,  5.0f,
         3.10f, 0.01f, -5.0f,  3.10f, 0.01f,  5.0f,  2.90f, 0.01f,  5.0f
    };

    float netVertices[] = {
        -3.0f, 0.0f,  -0.05f,  3.0f, 0.0f,  -0.05f, -3.0f, 0.5f,  -0.05f,
         3.0f, 0.0f,  -0.05f,  3.0f, 0.5f,  -0.05f, -3.0f, 0.5f,  -0.05f 
    };
    
    float ballRadius = 0.1f;
    Ball ballEllipsoid(ballRadius, 0, 0, 0);

    // Paddle for the opponent
    Paddle opponentPaddleObject(paddleRadius, paddleRadius, 0.0f, 0.0f, 0.0f, 0.0f);

    VAOVBO table(tableVertices, sizeof(tableVertices));
    VAOVBO line(lineVertices, sizeof(lineVertices));
    VAOVBO border(borderVertices, sizeof(borderVertices));
    VAOVBO net(netVertices, sizeof(netVertices));

    // VAO VBO for ball
    VAOVBO ball(ballEllipsoid.points, ballEllipsoid.size * sizeof(GLfloat));
    ballEllipsoid.changeCenterCoords(0, 0.5, 0);
    
    // Initialize serving state
    playerServing = isPlayerServing();
    if(playerServing) {
        ballEllipsoid.resetToServe(playerPaddle);
    } else {
        ballEllipsoid.resetToServe(opponentPaddleObject);
    }


    // Paddle
    VAOVBO paddle(playerPaddle.points, playerPaddle.size * sizeof(GLfloat));
    playerPaddle.changeCenterCoords(0, 0.1f, 3.5f);
    VAOVBO handle(playerPaddle.handleVertices, playerPaddle.handleVertexCount * sizeof(GLfloat));

    // Opponent Paddle
    VAOVBO opponentPaddle(opponentPaddleObject.points, opponentPaddleObject.size * sizeof(GLfloat));
    opponentPaddleObject.changeCenterCoords(0, 0.1f, -4.7f);
    VAOVBO opponentHandle(opponentPaddleObject.handleVertices, opponentPaddleObject.handleVertexCount * sizeof(GLfloat));

    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    
    float timeOfPreviousFrame = 0;
    float deltaTime = 0;

    Vec3 playerPos(0.0f,0.0f,4.0f);
    Vec3 opponentPos(0.0f, 0.0f, -4.0f);
    while(!glfwWindowShouldClose(window)) {
        // delta time 
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - timeOfPreviousFrame;
        timeOfPreviousFrame = currentFrame;

        // check for input
        processInput(window, state, selectedMenuOption, gameEndState, gamestate, ballEllipsoid, playerPaddle, opponentPaddleObject, playerScore, opponentScore, playerServing, serveTimer);

        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Always render game (3D perspective)
        // build MVP
        Mat4 model = identity();
        Mat4 view = lookAt(
            Vec3(0.0f, 9.0f, 10.0f),
            Vec3(0.0f, 0.0f, 0.0f),
            Vec3(0.0f, 1.0f, 0.0f)
        );

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;

        Mat4 proj = perspective(45.0f, aspect, 0.1f, 100.0f);
        Mat4 mvp   = proj * view * model;

        int mvpLocation = glGetUniformLocation(shaderProgram, "mvp");
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvp.m);

        int colorLoc = glGetUniformLocation(shaderProgram, "color");
        
        // Draw Table
        glUniform4f(colorLoc, 0.1f, 0.5f, 0.2f, 1.0f);
        table.VAO::Bind(); 
        glDrawArrays(GL_TRIANGLES, 0, 6); 

        // Draw Center Line
        glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
        line.VAO::Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw Border
        glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
        border.VAO::Bind();
        glDrawArrays(GL_TRIANGLES, 0, 24); 

        // Draw Net
        glUniform4f(colorLoc, 0.9f, 0.9f, 0.9f, 1.0f);
        net.VAO::Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // --- PLAYER PADDLE ---
        if(mouseMoved && state == RUNNING) {
            playerPaddle.movePaddle(windowWidth, windowHeight);
            playerPaddle.mousePrevPos = playerPaddle.mouseCurrentPos;
            mouseMoved = 0;
        }

        Vec3 paddleModel = playerPaddle.pos;
        mvp = proj * view * rotateY(playerPaddle.rotation, playerPaddle.pos) *  generateTranslateMatrix(paddleModel);
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvp.m);

        rgb paddleColor(220, 20, 30);
        glUniform4f(colorLoc, paddleColor.r, paddleColor.g, paddleColor.b, 0.8f);
        paddle.VAO::Bind();
        glDrawArrays(GL_LINE_LOOP, 0, (playerPaddle.triangleStartIdx)/3);
        
        rgb paddleTriangleColor(220, 185, 158);
        glUniform4f(colorLoc, paddleTriangleColor.r, paddleTriangleColor.g, paddleTriangleColor.b, 0.8f);
        glDrawArrays(GL_LINE_LOOP, (playerPaddle.triangleStartIdx)/3, (playerPaddle.size - playerPaddle.triangleStartIdx)/3);

        rgb handleColor(69, 72, 81);
        glUniform4f(colorLoc, handleColor.r, handleColor.g, handleColor.b, 0.8f);
        handle.VAO::Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // --- AI LOGIC UPDATE ---
        // We calculate the AI's logic BEFORE generating the matrix so the GPU places it in the right spot!
        if(state == RUNNING) {
            updateAI(opponentPaddleObject, ballEllipsoid, deltaTime);
        }

        // --- OPPONENT PADDLE ---
        Vec3 oppPaddleModel = opponentPaddleObject.pos;
        mvp = proj * view * generateTranslateMatrix(oppPaddleModel);
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvp.m);

        rgb oppPaddleColor(220, 20, 30);
        glUniform4f(colorLoc, oppPaddleColor.r, oppPaddleColor.g, oppPaddleColor.b, 0.8f);
        opponentPaddle.VAO::Bind();
        glDrawArrays(GL_LINE_LOOP, 0, (opponentPaddleObject.triangleStartIdx)/3);
        
        rgb oppPaddleTriangleColor(220, 185, 158);
        glUniform4f(colorLoc, oppPaddleTriangleColor.r, oppPaddleTriangleColor.g, oppPaddleTriangleColor.b, 0.8f);
        glDrawArrays(GL_LINE_LOOP, (opponentPaddleObject.triangleStartIdx)/3, (opponentPaddleObject.size - opponentPaddleObject.triangleStartIdx)/3);

        rgb opponentHandleColor(69, 72, 81);
        glUniform4f(colorLoc, opponentHandleColor.r, opponentHandleColor.g, opponentHandleColor.b, 0.8f);
        opponentHandle.VAO::Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);

       
        // Reset
        if(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            gamestate = SERVING;
            ballEllipsoid.outOfBounds = false;
            playerServing = isPlayerServing();
            serveTimer = 0.0f;
            
            if(playerServing) {
                ballEllipsoid.resetToServe(playerPaddle);
            } else {
                ballEllipsoid.resetToServe(opponentPaddleObject);
            }
        }

        // --- BALL ---
    if(state == RUNNING) {
        if(gamestate == SERVING) {
            // Ball follows whoever is serving
            if(playerServing) {
                ballEllipsoid.followPaddle(playerPaddle);
            } else {
                ballEllipsoid.followPaddle(opponentPaddleObject);
                
                // AI auto-serves after delay
                serveTimer += deltaTime;
                if(serveTimer >= SERVE_DELAY) {
                    gamestate = PLAYING;
                    serveTimer = 0.0f;
                }
            }
        } else {
            ballEllipsoid.updateKinematics(deltaTime, playerPaddle, opponentPaddleObject);
        
            // Play collision sound
            if(ballEllipsoid.collisionWithPaddle || ballEllipsoid.collisionWithTable) {
                playBallHitSound();
                ballEllipsoid.collisionWithPaddle = false;
                ballEllipsoid.collisionWithTable = false;
            }
        
            if(ballEllipsoid.outOfBounds) {
                if(ballEllipsoid.playerScored) {
                    playerScore++;
                    ballEllipsoid.playerScored = false;
                }
                if(ballEllipsoid.opponentScored) {
                    opponentScore++;
                    ballEllipsoid.opponentScored = false;
                }
                ballEllipsoid.outOfBounds = false;
                
                // Check for winner
                gameEndState = checkWinCondition(playerScore, opponentScore);
                
                if(gameEndState == IN_PROGRESS) {
                    // Game continues, prepare next serve
                    gamestate = SERVING;
                    playerServing = isPlayerServing();
                    serveTimer = 0.0f;
                    
                    // Reset ball to correct serving paddle
                    if(playerServing) {
                        ballEllipsoid.resetToServe(playerPaddle);
                    } else {
                        ballEllipsoid.resetToServe(opponentPaddleObject);
                    }
                } else {
                    // Game over - pause the game
                    state = PAUSED;
                }

                // Update window title with new score
                std::string title = "Table Tennis  |  You: " + std::to_string(playerScore) + "  AI: " + std::to_string(opponentScore);
                glfwSetWindowTitle(window, title.c_str());
            }
        }
    }

    Vec3 ballModel = ballEllipsoid.pos;
    mvp = proj * view * generateTranslateMatrix(ballModel);
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvp.m);

    rgb ballColor(254, 170, 45);
    glUniform4f(colorLoc, ballColor.r, ballColor.g, ballColor.b, 0.8f);
    ball.VAO::Bind();
    glDrawArrays(GL_LINE_LOOP, 0, ballEllipsoid.size/3);
        // Render pause menu overlay if paused (on top of game)
        if(state == PAUSED) {
            int mvpLocation = glGetUniformLocation(shaderProgram, "mvp");
            int colorLoc = glGetUniformLocation(shaderProgram, "color");
            renderPauseMenu(colorLoc, mvpLocation, selectedMenuOption, width, height, gameEndState);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    table.Delete();
    line.Delete();
    border.Delete();
    net.Delete();
    
    glDeleteProgram(shaderProgram);

    shutdownAudio();

    glfwTerminate();
    return 0;
}