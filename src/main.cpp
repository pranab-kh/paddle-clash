#include "math3d.h"
#include "vaovbo.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <graphicslibrary.h>

// Adjust the viewport to match the new window size
int windowWidth;
int windowHeight;
int mouseMoved = 0;
// Default rotation unit in degrees
const int rotationUnit = 5; 

enum GameState { SERVING, PLAYING };
GameState gameState = SERVING;

// Player Paddle declared here so that mouseCallBack will be able to access it
float paddleRadius = 0.5f;
Paddle playerPaddle(paddleRadius, paddleRadius ,0, 0, 0.0f, 0);

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    windowWidth = width;
    windowHeight = height;
}

// Checks keyboard input each frame
void processInput(GLFWwindow* window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// Shaders
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
    mouseMoved = 1;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if(gameState == SERVING) {
            gameState = PLAYING;
        }
    }
}

void keyPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
    // Rotation towards left
    if(key == GLFW_KEY_A) // && playerPaddle.rotation <= -30)
    {
        playerPaddle.rotation -= rotationUnit;
    }
    // Rotation towards right
    else if(key == GLFW_KEY_D) //  && playerPaddle.rotation >= 30)
    {
        playerPaddle.rotation += rotationUnit;
    }
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

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

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
    ballEllipsoid.resetToServe(playerPaddle);


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

    while(!glfwWindowShouldClose(window)) {
        // delta time 
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - timeOfPreviousFrame;
        timeOfPreviousFrame = currentFrame;

        processInput(window);

        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

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
        if(mouseMoved) {
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
        updateAI(opponentPaddleObject, ballEllipsoid, deltaTime);

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

        // --- BALL ---
        // --- BALL ---
    if(gameState == SERVING) {
        ballEllipsoid.followPaddle(playerPaddle);
    } else {
        ballEllipsoid.updateKinematics(deltaTime, playerPaddle, opponentPaddleObject);
        if(ballEllipsoid.outOfBounds) {
            ballEllipsoid.outOfBounds = false;
            gameState = SERVING;
            ballEllipsoid.resetToServe(playerPaddle);
        }
    }

    Vec3 ballModel = ballEllipsoid.pos;
    mvp = proj * view * generateTranslateMatrix(ballModel);
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvp.m);

    rgb ballColor(254, 170, 45);
    glUniform4f(colorLoc, ballColor.r, ballColor.g, ballColor.b, 0.8f);
    ball.VAO::Bind();
    glDrawArrays(GL_LINE_LOOP, 0, ballEllipsoid.size/3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    table.Delete();
    line.Delete();
    border.Delete();
    net.Delete();
    
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}