#include "math3d.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

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

    float tableVertices[] = {
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
    float lineVertices[] = {
        -3.0f, 0.01f, -0.05f,   // far left
        3.0f, 0.01f, -0.05f,   // far right
        -3.0f, 0.01f,  0.05f,   // near left

        3.0f, 0.01f, -0.05f,   // far right
        3.0f, 0.01f,  0.05f,   // near right
        -3.0f, 0.01f,  0.05f    // near left
    };

    // Table border — 4 edges (top, bottom, left, right)
    // Y=0.01
    float borderVertices[] = {
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

    unsigned int tableVAO, tableVBO;

    // generate them on the GPU
    glGenVertexArrays(1, &tableVAO);
    glGenBuffers(1, &tableVBO);;

    // bind VAO first
    glBindVertexArray(tableVAO);

    // bind VBO and upload the vertex data to the GPU
    glBindBuffer(GL_ARRAY_BUFFER, tableVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tableVertices), tableVertices, GL_STATIC_DRAW);

    // tell OpenGL how to read the data
    // location 0, 3 floats per vertex, no normalization, stride of 3 floats, starts at byte 0 (offset)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // unbind both (good habit)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // VAO VBO for center line
    unsigned int lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);

    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //  VAO VBO for border
    unsigned int borderVAO, borderVBO;
    glGenVertexArrays(1, &borderVAO);
    glGenBuffers(1, &borderVBO);

    glBindVertexArray(borderVAO);
    glBindBuffer(GL_ARRAY_BUFFER, borderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(borderVertices), borderVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //calls helper defined above to compile both shaders and links them together
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    while(!glfwWindowShouldClose(window)) {

        // check for input
        processInput(window);

        // set color to clear the screen
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f); // dark grey

        // clear the screen
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
        // Mat4 proj = perspective(45.0f, 800.0f/600.0f, 0.1f, 100.0f);
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
        
        glBindVertexArray(tableVAO); // use  vertex data
        glDrawArrays(GL_TRIANGLES, 0, 6); // draw 6 vertices(2triangles)

        // draw center line in white
        glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
        glBindVertexArray(lineVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // draw border in white
        glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
        glBindVertexArray(borderVAO);
        glDrawArrays(GL_TRIANGLES, 0, 24); 

        // swap front and back buffers
        glfwSwapBuffers(window);

        // check if any events happened since the last frame
        glfwPollEvents();
    }
    
    //cleanup
    glDeleteVertexArrays(1, &tableVAO);
    glDeleteBuffers(1, &tableVBO);

    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &lineVBO);

    glDeleteVertexArrays(1, &borderVAO);
    glDeleteBuffers(1, &borderVBO);
    
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}