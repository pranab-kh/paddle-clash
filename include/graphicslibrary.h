#ifndef GRAPHICS_LIB_H
#define GRAPHICS_LIB_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vaovbo.h>

struct point{
    int x, y, z;
    point(int x, int y, int z) : x(x), y(y), z(z){}
};

#endif