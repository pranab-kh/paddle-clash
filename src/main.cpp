#include "math3d.h"
#include "vaovbo.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <graphicslibrary.h>
#include "stb_easy_font.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"


//Game state management
enum GameState {
    Running,
    Paused
};
enum PauseMenuOption
{
    Resume,
    New_Game,
    Quit
};
GameState gamestate = Paused;
PauseMenuOption selectedMenuOption = Resume;
bool escapeKeyPressed = false;

// Minimal header-only audio (miniaudio): looping background music from file.
static ma_engine gAudioEngine;
static ma_sound gBgmSound;
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

    gAudioReady = true;
    std::cout << "Background music started." << std::endl;
    return true;
}

static void shutdownAudio() {
    if(gAudioReady) {
        ma_sound_uninit(&gBgmSound);
        ma_engine_uninit(&gAudioEngine);
        gAudioReady = false;
    }
}

//adjust the viewport to match the new window size
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

//checks keyboard input each frame
void processInput(GLFWwindow* window, GameState& state, PauseMenuOption& selectedOption) {
    bool escapePressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    
    // Toggle pause on ESC (with debounce)
    if(escapePressed && !escapeKeyPressed) {
        if(state == Running) {
            state = Paused;
            selectedOption = Resume;  // Reset to first option
        }
    }
    escapeKeyPressed = escapePressed;
    
    // Menu navigation when paused
    if(state == Paused) {
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
                state = Running;
            } else if(selectedOption == New_Game) {
                // TODO: Reset game here
                state = Running;
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


//runs once per pixel
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec4 color;

    void main() {
        FragColor = color;
    }
)";


//shader compiler: error boilerplates
//takes a piece of GLSL code as str and compiles it on the gpu
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check for errors
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Shader compile error:\n" << infoLog << std::endl;
    }

    return shader;
}

//links vertex and fragment shader together
unsigned int createShaderProgram(const char* vertSrc, const char* fragSrc) {
    unsigned int vert = compileShader(GL_VERTEX_SHADER, vertSrc);
    unsigned int frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    //creates an empty shader program on the GPU
    unsigned int program = glCreateProgram();
    //attach both shaders to the program
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "Shader link error:\n" << infoLog << std::endl;
    }

    // after linking, individual shaders are no longer needed
    glDeleteShader(vert);
    glDeleteShader(frag);

    return program;
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
void renderPauseMenu(int colorLoc, int mvpLocation, PauseMenuOption selected, int screenWidth, int screenHeight) {
    // Draw the dark overlay first
    renderOverlay(colorLoc, mvpLocation);

    // Draw three menu buttons (vertically stacked)
    float buttonWidth = 0.4f;
    float buttonHeight = 0.12f;

    renderMenuButton(0.0f,  0.3f, buttonWidth, buttonHeight, (selected == Resume),   colorLoc);
    renderMenuButton(0.0f,  0.0f, buttonWidth, buttonHeight, (selected == New_Game), colorLoc);
    renderMenuButton(0.0f, -0.3f, buttonWidth, buttonHeight, (selected == Quit),     colorLoc);
    
    // Larger text scale for better readability.
    const float labelScale = 2.6f;
    renderEasyText("RESUME",   0.0f,  0.3f, labelScale, colorLoc, screenWidth, screenHeight);
    renderEasyText("NEW GAME", 0.0f,  0.0f, labelScale, colorLoc, screenWidth, screenHeight);
    renderEasyText("QUIT",     0.0f, -0.3f, labelScale, colorLoc, screenWidth, screenHeight);
}

int main() {
    // initialize glfw
    if(!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // specify GLFW the opengl v 3.3core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Table Tennis", NULL, NULL);
    if(window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // make this window the current opengl context
    glfwMakeContextCurrent(window);

    // register the resize callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // initialize glad after setting context
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    initAudio();

    // size of the rendering window
    glViewport(0, 0, 800, 600);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

// const GLfloat scale = 1.0;

GLfloat tableVertices[] = {
    // Triangle 1
    -3.0f, 0.0f, -5.0f,  // far left
     3.0f, 0.0f, -5.0f,  // far right
    -3.0f, 0.0f,  5.0f,  // near left

    // Triangle 2
     3.0f, 0.0f, -5.0f,  // far right   
     3.0f, 0.0f,  5.0f,  // near right
    -3.0f, 0.0f,  5.0f   // near left
};

// center line 
GLfloat lineVertices[] = {
    -3.0f, 0.01f, -0.05f,   // far left
     3.0f, 0.01f, -0.05f,   // far right
    -3.0f, 0.01f,  0.05f,   // near left

     3.0f, 0.01f, -0.05f,   // far right
     3.0f, 0.01f,  0.05f,   // near right
    -3.0f, 0.01f,  0.05f    // near left
};

// Table border — 4 edges (top, bottom, left, right)
// Y=0.02 halved → 0.01
GLfloat borderVertices[] = {
    // Far edge: AI side 
    -3.0f, 0.01f, -5.10f,
     3.0f, 0.01f, -5.10f,
    -3.0f, 0.01f, -4.90f,
     3.0f, 0.01f, -5.10f,
     3.0f, 0.01f, -4.90f,
    -3.0f, 0.01f, -4.90f,

    // Near edge: player side
    -3.0f, 0.01f,  4.90f,
     3.0f, 0.01f,  4.90f,
    -3.0f, 0.01f,  5.10f,
     3.0f, 0.01f,  4.90f,
     3.0f, 0.01f,  5.10f,
    -3.0f, 0.01f,  5.10f,

    // Left edge 
    -3.10f, 0.01f, -5.0f,
    -2.90f, 0.01f, -5.0f,
    -3.10f, 0.01f,  5.0f,
    -2.90f, 0.01f, -5.0f,
    -2.90f, 0.01f,  5.0f,
    -3.10f, 0.01f,  5.0f,

    // Right edge 
     2.90f, 0.01f, -5.0f,
     3.10f, 0.01f, -5.0f,
     2.90f, 0.01f,  5.0f,
     3.10f, 0.01f, -5.0f,
     3.10f, 0.01f,  5.0f,
     2.90f, 0.01f,  5.0f
};

float netVertices[] = {
    -3.0f, 0.0f,  -0.05f,   // bottom left
     3.0f, 0.0f,  -0.05f,   // bottom right
    -3.0f, 0.5f,  -0.05f,   // top left

     3.0f, 0.0f,  -0.05f,   // bottom right
     3.0f, 0.5f,  -0.05f,   // top right
    -3.0f, 0.5f,  -0.05f    // top left
};
    
    float ballRadius = 0.1f;
    Ellipsoid ballEllipsoid(ballRadius, ballRadius, ballRadius);

    // Paddle is just an ellipse
    // float paddleRadius = 1.0f;
    float paddleRadius = 0.5f;
    // Paddle playerPaddle(paddleRadius, 0, paddleRadius, 0, -5, 0);
    Paddle playerPaddle(paddleRadius, 0, paddleRadius, 0, 0, 0);

    // Paddle for the opponent
    // Paddle opponentPaddleObject(paddleRadius, 0, paddleRadius, 0, 0.01f, -4.5f);
    Paddle opponentPaddleObject(paddleRadius, 0, paddleRadius, 0, 0, 0);

    // Paddle opponentPaddleObject(paddleRadius, 0, paddleRadius, 0, 1, 0);

    // VAO VBO for the table
    VAOVBO table(tableVertices, sizeof(tableVertices));
    
    // VAO VBO for center line
    VAOVBO line(lineVertices, sizeof(lineVertices));

    //  VAO VBO for border
    VAOVBO border(borderVertices, sizeof(borderVertices));

    // VAO VBO for net
    VAOVBO net(netVertices, sizeof(netVertices));

    // VAO VBO for ball
    VAOVBO ball(ballEllipsoid.points, ballEllipsoid.size * sizeof(GLfloat));

    // Paddle
    VAOVBO paddle(playerPaddle.points, playerPaddle.size * sizeof(GLfloat));

    // Paddle Handle
    VAOVBO handle(playerPaddle.handleVertices, playerPaddle.handleVertexCount * sizeof(GLfloat));

    // Opponent Paddle
    VAOVBO opponentPaddle(opponentPaddleObject.points, opponentPaddleObject.size * sizeof(GLfloat));

    // Opponent Handle
    VAOVBO opponentHandle(opponentPaddleObject.handleVertices, opponentPaddleObject.handleVertexCount * sizeof(GLfloat));


    

    //calls helper defined above to compile both shaders and links them together
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    Vec3 playerPos(0.0f,0.0f,4.0f);
    Vec3 opponentPos(0.0f, 0.0f, -4.0f);
    while(!glfwWindowShouldClose(window)) {

        // check for input
        processInput(window, gamestate, selectedMenuOption);

        // set color to clear the screen
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f); // dark grey

        // clear the screen and assign new color
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);      // activate  shader

        // Always render game (3D perspective)
        // build MVP
        Mat4 model = identity();

        // Mat4 view = lookAt(
        //     Vec3(0.0f, 8.0f, 8.0f),   // camera is above and behind player side
        //     Vec3(0.0f, 0.0f, 0.0f),   // looking at center of table
        //     Vec3(0.0f, 1.0f, 0.0f)    // up direction
        // );

        Mat4 view = lookAt(
            Vec3(0.0f, 8.0f, 9.0f),
            Vec3(0.0f, 0.0f, 0.0f),
            Vec3(0.0f, 1.0f, 0.0f)
        );

        // projection: mapping 3d to 2d screen
        // 45 degree fov, 800/600 aspect ratio, near=0.1, far=100
        // Mat4 proj = perspective(45.0f, 800.0f /600.0f, 0.1f, 100.0f);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;

        Mat4 proj = perspective(45.0f, aspect, 0.1f, 100.0f);

        Mat4 mvp   = proj * view * model;

        //send to shader
        int mvpLocation = glGetUniformLocation(shaderProgram, "mvp");
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvp.m);

        // send color to shader
        int colorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform4f(colorLoc, 0.1f, 0.5f, 0.2f, 1.0f); //dark green table
        
        table.VAO::Bind(); // use  vertex data
        glDrawArrays(GL_TRIANGLES, 0, 6); // draw 6 vertices(2triangles)

        // draw center line in white
        glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
        line.VAO::Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // draw border in white
        glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
        border.VAO::Bind();
        glDrawArrays(GL_TRIANGLES, 0, 24); 

        // draw net
        glUniform4f(colorLoc, 0.9f, 0.9f, 0.9f, 1.0f);
        net.VAO::Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // draw ball
        rgb ballColor(254, 170, 45);
        glUniform4f(colorLoc, ballColor.r, ballColor.g, ballColor.b, 0.8f);
        ball.VAO::Bind();
        glDrawArrays(GL_LINE_LOOP, 0, ballEllipsoid.size/3);

        // draw paddle 
        rgb paddleColor(220, 20, 30);
        glUniform4f(colorLoc, paddleColor.r, paddleColor.g, paddleColor.b, 0.8f);
        paddle.VAO::Bind();
        glDrawArrays(GL_LINE_LOOP, 0, (playerPaddle.triangleStartIdx)/3);
        
        rgb paddleTriangleColor(220, 185, 158);
        // The lower part of paddle that resembles a triangle
        glUniform4f(colorLoc, paddleTriangleColor.r, paddleTriangleColor.g, paddleTriangleColor.b, 0.8f);
        glDrawArrays(GL_LINE_LOOP, (playerPaddle.triangleStartIdx)/3, (playerPaddle.size - playerPaddle.triangleStartIdx)/3);

        // draw handle
        rgb handleColor(69, 72, 81);
        glUniform4f(colorLoc, handleColor.r, handleColor.g, handleColor.b, 0.8f);
        handle.VAO::Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);


        // draw opponent's paddle 
        rgb oppPaddleColor(220, 20, 30);
        glUniform4f(colorLoc, oppPaddleColor.r, oppPaddleColor.g, oppPaddleColor.b, 0.8f);
        opponentPaddle.VAO::Bind();
        glDrawArrays(GL_LINE_LOOP, 0, (opponentPaddleObject.triangleStartIdx)/3);
        
        rgb oppPaddleTriangleColor(220, 185, 158);
        // The lower part of paddle that resembles a triangle
        glUniform4f(colorLoc, oppPaddleTriangleColor.r, oppPaddleTriangleColor.g, oppPaddleTriangleColor.b, 0.8f);
        glDrawArrays(GL_LINE_LOOP, (opponentPaddleObject.triangleStartIdx)/3, (opponentPaddleObject.size - opponentPaddleObject.triangleStartIdx)/3);

        // draw handle
        rgb opponentHandleColor(69, 72, 81);
        glUniform4f(colorLoc, opponentHandleColor.r, opponentHandleColor.g, opponentHandleColor.b, 0.8f);
        opponentHandle.VAO::Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Render pause menu overlay if paused (on top of game)
        if(gamestate == Paused) {
            int mvpLocation = glGetUniformLocation(shaderProgram, "mvp");
            int colorLoc = glGetUniformLocation(shaderProgram, "color");
            renderPauseMenu(colorLoc, mvpLocation, selectedMenuOption, width, height);
        }

        // swap front and back buffers
        glfwSwapBuffers(window);

        // check if any events happened since the last frame
        glfwPollEvents();
    }
    
    //cleanup
    table.Delete();
    line.Delete();
    border.Delete();
    net.Delete();
    
    glDeleteProgram(shaderProgram);

    shutdownAudio();

    glfwTerminate();
    return 0;
}