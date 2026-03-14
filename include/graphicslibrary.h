#ifndef GRAPHICS_LIB_H
#define GRAPHICS_LIB_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "vaovbo.h"
#include "math3d.h"

// RGB struct to convert RGB colors to normalized form
struct rgb
{
    float r, g, b;
    rgb(int red, int green, int blue){
        r = (float)red / 255;
        g = (float)green / 255;
        b = (float)blue / 255;
    }
};

// Ellipsoid for ball and racket
struct Ellipsoid{
    //Length of semi axes and the center
    GLfloat a,b,c,h,k,l; 
    GLfloat *points;
    // Number of horizontal sections
    const int stacks = 400;
    // Number of partitions in each horizontal slice
    const int slices = 400;
    const int vertices = stacks * slices;
    const int size = vertices * 3;
    Ellipsoid(GLfloat a, GLfloat b, GLfloat c, GLfloat h = 0, GLfloat k = 0, GLfloat l = 0) : a(a), b(b), c(c), h(h), k(k), l(l){
        points = new GLfloat[size];
        GLfloat phi = 0;
        GLfloat phistep =  M_PI/stacks;
        GLfloat thetastep = (2 * M_PI)/slices;
        
        int idx = 0;
        for(int i = 0; i < stacks; i++)
        {
            GLfloat theta = 0;
            for(int j = 0; j < slices; j++)
            {
                points[idx++] = h + a * cosf(theta) * sinf(phi);
                points[idx++] = k + b * sinf(theta) * sinf(phi);
                // We are in (x,y,-z) octant so, z is subtracted
                points[idx++] = l - c * cosf(phi);
                theta += thetastep;
            }
            phi += phistep;
        }
    }

    ~Ellipsoid(){
        delete[] points;
    }  
};

// Paddle contains an ellipse partitioned into red and skin color in 70 : 30 ratio with a handle.
struct Paddle: public Ellipsoid{
    const int triangleStartIdx = size * 0.7;
    const int handleStartIdx = triangleStartIdx + int(slices/5) * 3;
    const int handleEndIdx = triangleStartIdx + int(slices/3) * 3;
    const static int handleVertexCount = 18;
    GLfloat handleVertices[handleVertexCount];
    GLfloat radius;
    Paddle(GLfloat a, GLfloat b, GLfloat c, GLfloat h = 0, GLfloat k = 0, GLfloat l = 0) : Ellipsoid(a, b, c, h, k, l){
            // We only need to compare between two radii because 2 out of 3 radii are same and one 1 out of 3 is 0.
            radius = (a > b)? a : b;
            int idx = 0;
            // Left top edge
            for(int i = 0; i < 3; i++)
            {
                handleVertices[idx++] = points[handleStartIdx + i];
            }

            // Right top edge
            for(int i = 0; i < 3; i++)
            {
                handleVertices[idx++] = points[handleEndIdx + i];
            }

            // Left bottom edge
            handleVertices[idx++] = handleVertices[0];
            handleVertices[idx++] = handleVertices[1];
            handleVertices[idx++] = handleVertices[2] + radius;

            // Left bottom edge
            handleVertices[idx++] = handleVertices[0];
            handleVertices[idx++] = handleVertices[1];
            handleVertices[idx++] = handleVertices[2] + radius;

            // Right bottom edge
            handleVertices[idx++] = handleVertices[3];
            handleVertices[idx++] = handleVertices[4];
            handleVertices[idx++] = handleVertices[5] + radius;

            // Right top edge
            for(int i = 0; i < 3; i++)
            {
                handleVertices[idx++] = points[handleEndIdx + i];
            }
    }
};

#endif