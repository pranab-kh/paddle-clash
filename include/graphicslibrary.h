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

    void changeCenterCoords(float x, float y, float z){
        pos = Vec3(x, y, z);
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
            mousePrevPos = Vec3(0, 0.01, 3.5);
            mouseCurrentPos = Vec3(0, 0.01, 3.5);
            radius = (a > b)? a : b;
            updateHandleCoords();
            updateHitboxes();
    }

    void updateHandleCoords()
    {
             int idx = 0;
            // Left top edge
            for(int i = 0; i < 3; i++)
                handleVertices[idx++] = points[handleStartIdx + i];

            // Right top edge
            for(int i = 0; i < 3; i++)
                handleVertices[idx++] = points[handleEndIdx + i];

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
                handleVertices[idx++] = points[handleEndIdx + i];
    }

    void updateAllPoints(){
        Ellipsoid::updateAllPoints();
        updateHandleCoords();
        updateHitboxes();
    }

    void movePaddle(float winWidth, float winHeight){
        Vec3 displacement = (mouseCurrentPos - mousePrevPos);
        if(magnitude(displacement) == 0) return;

        Vec3 normalizedDisplacement = normalize(displacement);
        float xScale = abs(displacement.x * 6.5)/(winWidth);
        float yScale = abs(displacement.y * 6.5)/(winHeight);
        normalizedDisplacement.x *= xScale;
        normalizedDisplacement.y *= yScale;
        
        incrementCenterCoords(normalizedDisplacement.x, normalizedDisplacement.y);

        // Clamping within table boundaries (Merged from AI branch)
        if(pos.x < -2.5f) pos.x = -2.5f;  // left wall
        if(pos.x >  2.5f) pos.x =  2.5f;  // right wall
        if(pos.z < 0.1f)  pos.z = 0.1f;   // can't cross net
        if(pos.z >  4.8f) pos.z =  4.8f;  // can't go past near edge

        updateHitboxes(); // Crucial for collision detection!
    }

    void updateHitboxes(){
        // Hitbox logic from working branch
        hitBoxMin = Vec3(pos.x - radius, pos.y-radius, pos.z - 0.05f);
        hitBoxMax = Vec3(pos.x + radius, pos.y+radius, pos.z + 0.05f);
    }
};

struct Ball : public Ellipsoid{
    float radius;
    const float coeffOfRestitutionForTable = 0.3;
    const float coeffOfRestitutionForPaddle = 1.2;
    bool outOfBounds = false; 
    
    Ball(float rad, float h = 0, float k = 0, float l = 0) : Ellipsoid(rad, rad, rad, h, k, l){
        radius = rad;
        acc = Vec3(0, -9.8, 5);
    }

    void updateKinematics(float deltaTime, Paddle& playerPaddle, Paddle& opponentPaddle)
    {
        acc.x *= 0.9;
        acc.z *= 0.9;
        vel = vel + acc * deltaTime;
        pos = pos + vel * deltaTime;
        
        // Bounce off table
        if(pos.y <= 0.01)
        {
            pos.y = 0.01;
            vel.y *= -1 * coeffOfRestitutionForTable;
        }

        // Collision with player paddle
        Vec3 closestPoint = getClosestPoint(playerPaddle.hitBoxMin, playerPaddle.hitBoxMax);
        float dist = distance(pos, closestPoint);
        if(dist <= radius)
        {
            vel.z = -4.0f;              // always shoot toward opponent on hit
            vel.y = 3.0f;               // consistent upward arc
            vel.x *= 0.5f;              // dampen sideways drift
        }
                

         // Collision with opponent paddle
         closestPoint = getClosestPoint(opponentPaddle.hitBoxMin, opponentPaddle.hitBoxMax);
         dist = distance(pos, closestPoint);
         if(dist <= radius)
         {
            vel.z *= -1 * coeffOfRestitutionForPaddle;
            vel.y += -acc.y/10;
         }
    }

    Vec3 getClosestPoint(Vec3 hitBoxMin, Vec3 hitBoxMax)
    {
        Vec3 closest;

        if(pos.x < hitBoxMin.x) closest.x = hitBoxMin.x;
        else if(pos.x > hitBoxMax.x) closest.x = hitBoxMax.x;
        else closest.x = pos.x;

        if(pos.y < hitBoxMin.y) closest.y = hitBoxMin.y;
        else if(pos.y > hitBoxMax.y) closest.y = hitBoxMax.y;
        else closest.y = pos.y;

        if(pos.z < hitBoxMin.z) closest.z = hitBoxMin.z;
        else if(pos.z > hitBoxMax.z) closest.z = hitBoxMax.z;
        else closest.z = pos.z;

        return closest;
    }

    void resetToServe(const Paddle& paddle) {
        pos = Vec3(paddle.pos.x, paddle.pos.y + radius + 0.3f, paddle.pos.z);
        vel = Vec3(0, -1.0f, -0.5f);  // slight drop + slight push toward opponent
        acc = Vec3(0, -9.8f, 0);
    }

    void followPaddle(const Paddle& paddle) {
        pos.x = paddle.pos.x;
        pos.z = paddle.pos.z;
        // Y stays fixed above paddle
        pos.y = paddle.pos.y + radius + 0.3f;
    }
};

// AI Logic (Merged from broken branch, updated for GPU workflow)
void updateAI(Paddle& aiPaddle, const Ball& ball, float deltaTime){
    float aiSpeed = 2.5f;
    float deadzone = 0.05f;

    if(ball.pos.x > aiPaddle.pos.x + deadzone)
        aiPaddle.pos.x += aiSpeed * deltaTime;
    else if(ball.pos.x < aiPaddle.pos.x - deadzone)
        aiPaddle.pos.x -= aiSpeed * deltaTime;

    // AI boundaries
    if(aiPaddle.pos.x < -2.5f) aiPaddle.pos.x = -2.5f;
    if(aiPaddle.pos.x >  2.5f) aiPaddle.pos.x =  2.5f;

    // Update the hitboxes so the ball can actually collide with the AI paddle!
    aiPaddle.updateHitboxes();
}

#endif