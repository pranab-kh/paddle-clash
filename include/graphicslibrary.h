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
    GLfloat a,b,c; 
    Vec3 pos;
    Vec3 vel;
    Vec3 acc;
    GLfloat *points;
    // Number of horizontal sections
    const int stacks = 400;
    // Number of partitions in each horizontal slice
    const int slices = 400;
    const int vertices = stacks * slices;
    const int size = vertices * 3;

    void updateAllPoints()
    {
        GLfloat phi = 0;
        GLfloat phistep =  M_PI/stacks;
        GLfloat thetastep = (2 * M_PI)/slices;
        
        int idx = 0;
        for(int i = 0; i < stacks; i++)
        {
            GLfloat theta = 0;
            for(int j = 0; j < slices; j++)
            {
                points[idx++] = pos.x + a * cosf(theta) * sinf(phi);
                // points[idx++] = pos.y + b * sinf(theta) * sinf(phi);
                // We are in (x,y,-z) octant so, z is subtracted
                points[idx++] = pos.y + b * cosf(phi);
                points[idx++] = pos.z - c * sinf(theta) * sinf(phi);
                theta += thetastep;
            }
            phi += phistep;
        }   
    }

    Ellipsoid(GLfloat a, GLfloat b, GLfloat c, GLfloat h = 0, GLfloat k = 0, GLfloat l = 0) : a(a), b(b), c(c){
        pos = Vec3(h, k, l);
        points = new GLfloat[size];
        updateAllPoints();
    }

    void changeCenterCoords(float x, float y){
        pos = Vec3(x, y);
    }

    void incrementCenterCoords(float x, float y){
        pos = pos + Vec3(x, y);
    }


    ~Ellipsoid(){
        delete[] points;
    }  
};




// Paddle contains an ellipse partitioned into red and skin color in 70 : 30 ratio with a handle.
struct Paddle: public Ellipsoid{
    const int triangleStartIdx = size * 0.7;
    const int handleStartIdx = triangleStartIdx + int(slices/5) * 3;
    const int handleEndIdx = triangleStartIdx + int(slices/3.5) * 3;
    const static int handleVertexCount = 18;
    GLfloat handleVertices[handleVertexCount];
    GLfloat radius;
    Vec3 mousePrevPos;
    Vec3 mouseCurrentPos;
    Vec3 hitBoxMin;
    Vec3 hitBoxMax;

    Paddle(GLfloat a, GLfloat b, GLfloat c, GLfloat h = 0, GLfloat k = 0, GLfloat l = 0) : Ellipsoid(a, b, c, h, k, l){
            // Hard coded the center position of the player racket in coordinates
            mousePrevPos = Vec3(0, 0.01, 3.5);
            mouseCurrentPos = Vec3(0, 0.01, 3.5);
            // We only need to compare between two radii because 2 out of 3 radii are same and one 1 out of 3 is 0.
            radius = (a > b)? a : b;
            updateHandleCoords();
            updateHitboxes();
    }

    void updateHandleCoords()
    {
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
            handleVertices[idx++] = handleVertices[1] - radius;
            handleVertices[idx++] = handleVertices[2];

            // Left bottom edge
            handleVertices[idx++] = handleVertices[0];
            handleVertices[idx++] = handleVertices[1] - radius;
            handleVertices[idx++] = handleVertices[2];

            // Right bottom edge
            handleVertices[idx++] = handleVertices[3];
            handleVertices[idx++] = handleVertices[4] - radius;
            handleVertices[idx++] = handleVertices[5];

            // Right top edge
            for(int i = 0; i < 3; i++)
            {
                handleVertices[idx++] = points[handleEndIdx + i];
            }
    }

    void updateAllPoints(){
        Ellipsoid::updateAllPoints();
        updateHandleCoords();
        updateHitboxes();
    }

    void movePaddle(float winWidth, float winHeight){
        // Calculate the displacement
        Vec3 displacement = (mouseCurrentPos - mousePrevPos);
        if(magnitude(displacement) == 0)
        {
            return;
        }
        // Normalize the displacement
        Vec3 normalizedDisplacement = normalize(displacement);
        // In order to change the increase the speed of racket motion when mouse travels more distance, we use two scales
        float xScale = abs(displacement.x * 6.5)/(winWidth);
        float yScale = abs(displacement.y * 6.5)/(winHeight);
        normalizedDisplacement.x *= xScale;
        normalizedDisplacement.y *= yScale;

        incrementCenterCoords(normalizedDisplacement.x, normalizedDisplacement.y);
        // All points are updated according to the center that we just changed

        // clamp within table boundaries
        if(pos.x < -2.5f) pos.x = -2.5f;  // left wall
        if(pos.x >  2.5f) pos.x =  2.5f;  // right wall
        if(pos.z < 0.1f)  pos.z = 0.1f;   // can't cross net
        if(pos.z >  4.8f) pos.z =  4.8f;  // can't go past near edge

        // updateAllPoints();
    }

    void updateHitboxes(){
    // Z axis is reversed so z has negative sign
    hitBoxMin = Vec3(pos.x - radius, pos.y - radius, pos.z + 0.05f);
    hitBoxMax = Vec3(pos.x + radius, pos.y + radius, pos.z - 0.05f);
        // updateAllPoints();
        // After using the concept of mvp matrix, we don't need to do this thing
    }

};


struct Ball : public Ellipsoid{

    const float coeffOfRestitution = 0.9;
    Ball(float rad, float h = 0, float k = 0, float l = 0) : Ellipsoid(rad, rad, rad, h, k, l){
        acc = Vec3(0, -9.8/3, 0);
    }

    // void updateKinematics(float deltaTime, Paddle& playerPaddle, Paddle& opponentPaddle)
    void updateKinematics(float deltaTime)
    {
        vel = vel + acc * deltaTime;
        pos = pos + vel * deltaTime;
        // The ball bounces back if it hits the table
        if(pos.y <= 0.01)
        {
            vel.y *= -1 * coeffOfRestitution;
        }

        // // From here

        // // Collision with the paddles

        // // Collision with the player paddle
        // // Finding the  point in the hitbox of the paddle which is closest to the ball
        //  float xClosest, yClosest, zClosest;
        //  if(pos.x < playerPaddle.hitBoxMin.x)
        //     xClosest = playerPaddle.hitBoxMin.x;
        //  if(pos.x > xClosest)


        updateAllPoints();
    }
};


void updateAI(Paddle& aiPaddle, const Ball& ball, float deltaTime){
    float aiSpeed = 2.5f;
    float deadzone = 0.05f;

    if(ball.pos.x > aiPaddle.pos.x + deadzone)
        aiPaddle.pos.x += aiSpeed * deltaTime;
    else if(ball.pos.x < aiPaddle.pos.x - deadzone)
        aiPaddle.pos.x -= aiSpeed * deltaTime;

    if(aiPaddle.pos.x < -2.5f) aiPaddle.pos.x = -2.5f;
    if(aiPaddle.pos.x >  2.5f) aiPaddle.pos.x =  2.5f;

    // aiPaddle.updateAllPoints();
}


#endif