#ifndef VAOVBO_H
#define VAOVBO_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct VBO{
    GLuint object;
    void Bind(){
        glBindBuffer(GL_ARRAY_BUFFER, object);
    }

    void Unbind()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    VBO()
    {
        glGenBuffers(1, &object);
    }

    void setData(GLfloat* f, size_t x){
        glBufferData(GL_ARRAY_BUFFER, x, f , GL_STATIC_DRAW);
    }

    void Delete(){
        glDeleteBuffers(1, &object);
    }

};

struct VAO{
    GLuint object;
    VAO(){
        glGenVertexArrays(1, &object);
    }   

    void Bind(){
        glBindVertexArray(object);
    }

    void Unbind(){
        glBindVertexArray(0);
    }

    void Enable(){
        glEnableVertexAttribArray(0);
    }

    void InterpetAndEnable(){
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        Enable();
    }
    
    void Delete(){
        glDeleteVertexArrays(1, &object);
    }
};


struct VAOVBO: public VAO, public VBO{
    VAOVBO(GLfloat* dataArray, size_t dataSize){
        // Bind VAO first
        VAO::Bind();
        // Bind VBO
        VBO::Bind();

        // Set data to the current binded VBO
        setData(dataArray, dataSize);
        // Configure VAO to interpret the data of VBO (only use format where 3 GLfloats are used to represent 1 vertex and continually separated by comma for the other vertices)
        InterpetAndEnable();

        VBO::Unbind();
        VAO::Unbind();
    }

    void Delete(){
            VBO::Delete();
            VAO::Delete();
    }
};

#endif