#include "math3d.h"
#include "vaovbo.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <graphicslibrary.h>

//adjust the viewport to match the new window size
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

//checks keyboard input each frame
void processInput(GLFWwindow* window) {
    //esc close window case
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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
    

    // size of the rendering window
    glViewport(0, 0, 800, 600);

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
    float paddleRadius = 1.0f;
    Paddle playerPaddle(paddleRadius, 0, paddleRadius, 0, -5, 0);

    // Paddle for the opponent
    Paddle opponentPaddleObject(paddleRadius, 0, paddleRadius, 0, 1, 0);

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

    while(!glfwWindowShouldClose(window)) {

        // check for input
        processInput(window);

        // set color to clear the screen
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f); // dark grey

        // clear the screen and assign new color
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);      // activate  shader

        // build MVP
        Mat4 model = identity();

        Mat4 view = lookAt(
            Vec3(0.0f, 8.0f, 8.0f),   // camera is above and behind player side
            Vec3(0.0f, 0.0f, 0.0f),   // looking at center of table
            Vec3(0.0f, 1.0f, 0.0f)    // up direction
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

    glfwTerminate();
    return 0;
}