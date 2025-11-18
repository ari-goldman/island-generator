//  CSCIx239 library
//  Willem A. (Vlakkies) Schreuder
#ifndef CSCIx239
#define CSCIx239

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#ifdef USEGLEW
#include <GL/glew.h>
#endif
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenGL/glu.h>
#include <OpenGL/gl.h>
#else
#include <GL/glu.h>
#include <GL/gl.h>
#endif

#define Cos(th) cos(3.1415926/180*(th))
#define Sin(th) sin(3.1415926/180*(th))

#ifdef __cplusplus
extern "C" {
#endif

//  Initialize GLFW and create window
GLFWwindow* InitWindow(const char* title,int sync,int width,int height , void (*reshape)(GLFWwindow*,int,int) , void (*key)(GLFWwindow*,int,int,int,int));

//  Convenience functions
#ifdef __GNUC__
void Print(const char* format , ...) __attribute__ ((format(printf,1,2)));
void Fatal(const char* format , ...) __attribute__ ((format(printf,1,2))) __attribute__ ((noreturn));
#else
void Print(const char* format , ...);
void Fatal(const char* format , ...);
#endif
void ErrCheck(const char* where);
void Axes(float len);

//  Transformations
void Projection(float fov,float asp,float dim);
void View(float th,float ph,float fov,float dim);

//  Enable lighting
void SetColor(float R,float G,float B);
void Lighting(float x,float y,float z,float ambient,float diffuse,float specular);

//  Load textures and OBJ files
unsigned int LoadTexBMP(const char* file);
int          LoadOBJ(const char* file);

//  Timing/Performance
int    FramesPerSecond(void);
double Elapsed(void);

//  Shader functions
void CreateShader(int prog,const GLenum type,const char* file);
void PrintProgramLog(int obj);
float**  CreateNoise2D(int unit);
int  CreateShaderProg(const char* vert,const char* frag);
int  CreateShaderGeom(const char* vert,const char* geom,const char* frag);

//  Basic objects
void SolidCube();
void SolidSphere(int n);
void SolidCone(int n);
void SolidCylinder(int n);
void SolidTorus(float r,int n);
void SolidIcosahedron();
void SolidTeapot(int n);
//  Textured objects
void TexturedCube(int tex);
void TexturedSphere(int n,int tex);
void TexturedCone(int n,int tex);
void TexturedCylinder(int n,int tex);
void TexturedTorus(float r,int n,int tex);
void TexturedIcosahedron(int tex);
void TexturedTeapot(int n,int tex);
//  General objects
void Cube(float x,float y,float z , float dx,float dy,float dz, float th,float ph , int tex);
void Sphere(float x,float y,float z,float r,float th,int n,int tex);
void Cone(float x,float y,float z , float r,float h, float th,float ph , int n,int tex);
void Cylinder(float x,float y,float z , float r,float h, float th,float ph , int n,int tex);
void Torus(float x,float y,float z , float R,float r, float th,float ph , int n,int tex);
void Icosahedron(float x,float y,float z , float r, float th,float ph , int tex);
void Teapot(float x,float y,float z,float r,float th,float ph,int n,int tex);

#include "mat4.h"

#ifdef __cplusplus
}
#endif

#endif
