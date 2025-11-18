#include <vector>
//#include <GL/glew.h>
#include <iostream>
#include <cmath>
#include "PerlinNoise.hpp"

#include "CSCIx239.h"

int axes=1;       //  Display axes
int move=0;       //  Move light
int rotate=0;
float th=30;         //  Azimuth of view angle
float ph=20;         //  Elevation of view angle
int fov=55;       //  Field of view (for perspective)
double asp=1;     //  Aspect ratio
double dim=3.0;   //  Size of world
int zh=90;        //  Light azimuth
float Ylight=1;   //  Light elevation
int shader[]  = {0,0,0}; //  Shader programs

float eyePos[] = {0,0,0}; //  Eye position

bool lmbDown = false;
double prevX, prevY;
float sensitivity = 0.1f;

int windowWidth, windowHeight;

float cloudHeight = 2.3;
int FBM = 0;
int cloudShader;
int cloudNoiseTexture;

bool newIsland = true;
int triplanar = 1;
int islandShader;
int grassTexture, stoneTexture, sandTexture;

int erosionShader;
int erosionShader_tex;
int erode = 32;

int oceanShader;

GLuint frameBuffer;
GLuint sceneTex;
GLuint sceneDepthTex;

float zNear, zFar;

//  Light colors
const float Emission[]  = {0.0,0.0,0.0,1.0};
const float Ambient[]   = {0.5,0.5,0.5,1.0};
const float Diffuse[]   = {0.9,0.9,0.9,1.0};
const float Specular[]  = {1.0,1.0,1.0,1.0};
const float Shinyness[] = {1.5};
//  Transformation matrixes
float ProjectionMatrix[16];
float ViewMatrix[16];
//  Set lighting parameters using uniforms
float Position[4];


int CreateShaderProgCompute(const char* file) {
   //  Create program
   int prog = glCreateProgram();
   //  Create and compile compute shader
   CreateShader(prog,GL_COMPUTE_SHADER,file);
   //  Link program
   glLinkProgram(prog);
   //  Check for errors
   PrintProgramLog(prog);
   //  Return name
   return prog;
}

// https://github.com/Reputeless/PerlinNoise
std::vector<std::vector<float>> generateNoise(int xLen, int zLen, double factor) {
   std::vector<std::vector<float>> heights;

   const siv::PerlinNoise::seed_type seed = time(nullptr);
   const siv::PerlinNoise perlin{ seed };

   for(int z = 0; z < zLen; z += 1) {
      std::vector<float> row;
      for(int x = 0; x < xLen; x += 1) {
         const double noise = perlin.octave2D_01(
            (double)x / (double)xLen * factor,
            (double)z / (double)zLen * factor,
            4
         );
         row.push_back(static_cast<float>(noise));
      }
      heights.push_back(row);
   }

   return heights;
}




struct MeshVertex {
   float pos[4];
   float normal[3];
   float tex[2];
   float color[4];
};

void normalizeVector(std::vector<float> &v) {
   double sum = 0;
   for(size_t i = 0; i < v.size(); i++) {
      sum += v[i] * v[i];
   }

   double length = sqrt(sum);

   for(size_t i = 0; i < v.size(); i++) {
      v[i] /= length;
   }
}

std::vector<float> cross(std::vector<float> v1, std::vector<float> v2) {
   std::vector<float> res;

   if(v2.size() != 3 || v1.size() != 3) return res;

   res.push_back(v1[1]*v2[2] - v1[2]*v2[1]);
   res.push_back(v1[2]*v2[0] - v1[0]*v2[2]);
   res.push_back(v1[0]*v2[1] - v1[1]*v2[0]);

   return res;
}




// create the mesh vertices for the island (with normals and such)
std::vector<MeshVertex> generateIslandMesh(std::vector<std::vector<float>> heights, float xSize, float zSize) {
   std::vector<MeshVertex> islandVertices;
   size_t xLen = heights.size();
   size_t zLen = heights[0].size();

   float dx = xSize / (float)xLen;
   float dz = zSize / (float)zLen;

   for(size_t z = 0; z < zLen; z += 1) {
      for(size_t x = 0; x < xLen; x += 1) {
         MeshVertex v{};

         v.pos[0] = (float)x / (float)xLen * xSize - xSize / 2;
         v.pos[1] = heights[x][z];
         v.pos[2] = (float)z / (float)zLen * zSize - zSize / 2;
         v.pos[3] = 1.0;

         // 0,0 is bottom left here
         float l = x > 0        ? heights[x-1][z]  : heights[x][z];
         float r = x < xLen - 1 ? heights[x+1][z]  : heights[x][z];
         float u = z > 0        ? heights[x][z-1]  : heights[x][z];
         float d = z < zLen - 1 ? heights[x][z+1]  : heights[x][z];

         std::vector<float> tanX = { dx, r - l, 0 };
         std::vector<float> tanZ = { 0, u - d, dz };

         std::vector<float> normal = cross(tanZ, tanX);
         normalizeVector(normal);

         v.normal[0] = normal[0];
         v.normal[1] = normal[1];
         v.normal[2] = -normal[2];

         v.tex[0] = (float)x / (float)xLen * 5.0;
         v.tex[1] = (float)z / (float)zLen * 5.0;

         v.color[0] = normal[0];
         v.color[1] = normal[1];
         v.color[2] = normal[2];
         v.color[3] = 1.0;

         islandVertices.push_back(v);
      }
   }

   return islandVertices;
}

// get the list of indices of the vertices
std::vector<GLuint> generateIslandIndexList(int xLen, int zLen) {
   std::vector<GLuint> islandIndexList;

   for(int z = 0; z < zLen-1; z += 1) {
      for(int x = 0; x < xLen-1; x += 1) {
         GLuint ul = z * xLen + x;
         GLuint ur = z * xLen + x + 1;
         GLuint bl = (z + 1) * xLen + x;
         GLuint br = (z + 1) * xLen + x + 1;

         islandIndexList.push_back(ul);
         islandIndexList.push_back(bl);
         islandIndexList.push_back(ur);

         islandIndexList.push_back(ur);
         islandIndexList.push_back(bl);
         islandIndexList.push_back(br);
      }
   }

   return islandIndexList;
}


// use perlin noise to generate a heightmap
std::vector<std::vector<float>> generateRandomHeights() {
   std::vector<std::vector<float>> heights = generateNoise(256, 256, 2);

   // flatten to circle
   int xLen = heights.size();
   int zLen = heights[0].size();
   float maxDist = std::min(zLen / 2.0f, xLen / 2.0f);
   for(int z = 0; z < zLen; z += 1) {
      for(int x = 0; x < xLen; x += 1) {
         float xDist = (float)x - xLen / 2.0;
         float zDist = (float)z - zLen / 2.0;
         float centerDist = sqrt(xDist * xDist + zDist * zDist);

         float t = 1 - centerDist / maxDist;
         t = 3 * t * t - 2 * t * t * t; // smoothstep

         if(centerDist > maxDist) heights[x][z] = 0;
         else heights[x][z] = 2.5 * heights[x][z] * t;
      }
   }

   return heights;
}


// turn the heightmap into a texture
int generateHeightTexture(std::vector<std::vector<float>> heights) {
    int xLen = heights.size();
    int zLen = heights[0].size();

    // Flatten into a single array
    std::vector<float> heightsFlat;
    heightsFlat.reserve(xLen * zLen);
    for (int z = 0; z < zLen; z++) {
        // optional: assert(heights[y].size() == W);
        for (int x = 0; x < xLen; x++) {
            heightsFlat.push_back(heights[x][z]);
        }
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, xLen, zLen);

    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0, 0,
                    xLen, zLen,
                    GL_RED, // only need 1 channel!
                    GL_FLOAT,
                    heightsFlat.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}


// read a texture to a height map
void readHeightTexture(int tex, std::vector<std::vector<float>> &heights) {
   int xLen = heights.size();
   int zLen = heights[0].size();

   std::vector<float> heightsFlat;
   heightsFlat.resize(xLen * zLen);

   // 2) Bind & pack‐alignment
   glBindTexture(GL_TEXTURE_2D, tex);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);

   // 3) Read back the red channel floats
   glGetTexImage(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 GL_FLOAT,
                 heightsFlat.data());

   glBindTexture(GL_TEXTURE_2D, 0);

   heights.assign(xLen, std::vector<float>(zLen));
   for(int z = 0; z < zLen; ++z){
      for(int x = 0; x < xLen; ++x){
         heights[x][z] = heightsFlat[z * xLen + x];
      }
   }

}


// erode the heightmap
void erodeHeights(std::vector<std::vector<float>> &heights) {
   std::vector<float> heightsFlat(heights.size() * heights[0].size());
      for(int x = 0; x < (int)heights.size(); x++) {
         for(int y = 0; y < (int)heights[0].size(); y++) {
            int index = (y * (int)heights.size() + x);
            heightsFlat[index] = heights[x][y];
         }
      }



      std::vector<int> rands(256);
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dist(0, heightsFlat.size() - 1);
      for(int i = 0; i < 256; ++i){
         rands[i] = dist(gen);
      }

      GLuint randSSBO;
      glGenBuffers(1, &randSSBO);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, randSSBO);
      glBufferData(GL_SHADER_STORAGE_BUFFER,
                   rands.size()*sizeof(int),
                   rands.data(),
                   GL_STATIC_DRAW);
      // bind it to binding point 1 (for example)
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, randSSBO);

      GLuint ssbo;
      glGenBuffers(1, &ssbo);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
      glBufferData(GL_SHADER_STORAGE_BUFFER, heightsFlat.size() * sizeof(float), heightsFlat.data(), GL_DYNAMIC_DRAW);

      glUseProgram(erosionShader);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

      int loc = glGetUniformLocation(erosionShader, "mapSize");
      glUniform2i(loc, 256, 256);

      glDispatchCompute(1, 1, 1);
      glMemoryBarrier(GL_ALL_BARRIER_BITS);

      glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
      glGetBufferSubData(GL_SHADER_STORAGE_BUFFER,
                         0,
                         heightsFlat.size()*sizeof(float),
                         heightsFlat.data());
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


      for(int x = 0; x < (int)heights.size(); x++) {
         for(int y = 0; y < (int)heights[0].size(); y++) {
            int index = (y * (int)heights.size() + x);
            heights[x][y] = heightsFlat[index];
         }
      }
}



// erode the height map texture
void erodeHeights_tex(std::vector<std::vector<float>> &heights) {

   // random rain locations
   std::vector<int> rands(256);
   std::random_device rd;
   std::mt19937 gen(rd());
   std::uniform_real_distribution<> dist(0, heights.size() * heights[0].size() - 1);
   for(int i = 0; i < 256; ++i){
      rands[i] = dist(gen);
   }

   int heightTex = generateHeightTexture(heights);


   GLuint randSSBO;
   glGenBuffers(1, &randSSBO);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, randSSBO);
   glBufferData(GL_SHADER_STORAGE_BUFFER,
                rands.size()*sizeof(int),
                rands.data(),
                GL_STATIC_DRAW);
   // bind it to binding point 1 (for example)
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, randSSBO);

   glUseProgram(erosionShader_tex);
   glBindImageTexture(0, heightTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

   int loc = glGetUniformLocation(erosionShader_tex, "texSize");
   glUniform2i(loc, 256, 256);

   glDispatchCompute(1, 1, 1);
   glMemoryBarrier(GL_ALL_BARRIER_BITS);


   readHeightTexture(heightTex, heights);
}


// draw the island
void island(float xSize, float zSize) {
   static std::vector<std::vector<float>> heights;
   static unsigned int vao = 0;
   static unsigned int vbo = 0;
   static unsigned int ebo = 0;
   static size_t indexCount = 0;

   if(newIsland) {
      // cleanup old resources
      if (vao) glDeleteVertexArrays(1, &vao);
      if (vbo) glDeleteBuffers(1, &vbo);
      if (ebo) glDeleteBuffers(1, &ebo);

      heights = generateRandomHeights();
      
      // make the mesh
      std::vector<MeshVertex> islandVertices = generateIslandMesh(heights, xSize, zSize);
      std::vector<GLuint> islandIndices = generateIslandIndexList(heights[0].size(), heights.size());
      indexCount = islandIndices.size();

      // make buffers
      glGenVertexArrays(1, &vao);
      glGenBuffers(1, &vbo);
      glGenBuffers(1, &ebo);

      glBindVertexArray(vao);

      // vbo
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, islandVertices.size() * sizeof(MeshVertex), islandVertices.data(), GL_DYNAMIC_DRAW);

      // ebo
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), islandIndices.data(), GL_STATIC_DRAW);

      // vertex attributes
      glUseProgram(islandShader);
      
      int loc = glGetAttribLocation(islandShader,"Vertex");
      glVertexAttribPointer(loc,4,GL_FLOAT,0,sizeof(MeshVertex),(void*) 0);
      glEnableVertexAttribArray(loc);

      loc = glGetAttribLocation(islandShader,"Normal");
      glVertexAttribPointer(loc,3,GL_FLOAT,0,sizeof(MeshVertex),(void*)16);
      glEnableVertexAttribArray(loc);

      loc = glGetAttribLocation(islandShader,"Texture");
      glVertexAttribPointer(loc,2,GL_FLOAT,0,sizeof(MeshVertex),(void*)28);
      glEnableVertexAttribArray(loc);
      
      // unbind VAO
      glBindVertexArray(0);
      newIsland = false;
   }

   if(erode > 0) {
      erodeHeights_tex(heights);
      erode--;
      
      // height change -> regen mesh and vbo
      std::vector<MeshVertex> islandVertices = generateIslandMesh(heights, xSize, zSize);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, islandVertices.size() * sizeof(MeshVertex), islandVertices.data());
   }

   glUseProgram(islandShader);

   int id = glGetUniformLocation(islandShader,"ProjectionMatrix");
   glUniformMatrix4fv(id,1,0,ProjectionMatrix);
   id = glGetUniformLocation(islandShader,"ViewMatrix");
   glUniformMatrix4fv(id,1,0,ViewMatrix);

   float ModelViewMatrix[16];
   mat4copy(ModelViewMatrix , ViewMatrix);

   id = glGetUniformLocation(islandShader,"ModelViewMatrix");
   glUniformMatrix4fv(id,1,0,ModelViewMatrix);

   float NormalMatrix[9];
   mat3normalMatrix(ModelViewMatrix , NormalMatrix);
   id = glGetUniformLocation(islandShader,"NormalMatrix");
   glUniformMatrix3fv(id,1,0,NormalMatrix);

   id = glGetUniformLocation(islandShader,"SunVector");
   glUniform3fv(id,1,Position);
   id = glGetUniformLocation(islandShader,"Ambient");
   glUniform4fv(id,1,Ambient);
   id = glGetUniformLocation(islandShader,"Diffuse");
   glUniform4fv(id,1,Diffuse);
   id = glGetUniformLocation(islandShader,"Specular");
   glUniform4fv(id,1,Specular);

   id = glGetUniformLocation(islandShader,"Ks");
   glUniform4fv(id,1,Specular);
   id = glGetUniformLocation(islandShader,"Ke");
   glUniform4fv(id,1,Emission);
   id = glGetUniformLocation(islandShader,"Shinyness");
   glUniform1f(id,Shinyness[0]);

   id = glGetUniformLocation(islandShader,"Triplanar");
   glUniform1i(id,triplanar);


   // Bind VAO and Textures
   glBindVertexArray(vao);

   int loc = glGetUniformLocation(islandShader, "texGrass");
   glUniform1i(loc, 0);
   loc = glGetUniformLocation(islandShader, "texStone");
   glUniform1i(loc, 1);
   loc = glGetUniformLocation(islandShader, "texSand");
   glUniform1i(loc, 2);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, grassTexture);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, stoneTexture);
   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, sandTexture);

   // Draw
   glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, NULL);

   // Unbind
   glBindVertexArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   // Cleanup GL_ACTIVE_TEXTURE
   glActiveTexture(GL_TEXTURE0);
}


// generate a 3D noise texture for the clouds
int generate3DNoiseTexture(int size) {
   const siv::PerlinNoise::seed_type seed = time(nullptr);
   const siv::PerlinNoise perlin{ seed };

   std::vector<float> noise(size*size*size);
   for(int z = 0; z < size; z++) {
      for(int y = 0; y < size; y++) {
         for(int x = 0; x < size; x++) {
            float fx = float(x) / size;
            float fy = float(y) / size;
            float fz = float(z) / size;
            noise[x + y*size + z*size*size] = perlin.normalizedOctave3D_01(fx*8,fy*8,fz*8,8);
         }
      }
   }

   GLuint texture;
   glGenTextures(1, &texture);
   glBindTexture(GL_TEXTURE_3D, texture);
   glTexImage3D(GL_TEXTURE_3D,
                0,                  // mip level
                GL_R16F,            // internal format (one float channel)
                size, size, size,   // width, height, depth
                0,                  // border
                GL_RED,             // format of your data
                GL_FLOAT,           // type of your data
                noise.data());
   // Filtering & wrapping
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,     GL_MIRRORED_REPEAT);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,     GL_MIRRORED_REPEAT);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,     GL_MIRRORED_REPEAT);

   return texture;
}

void clouds() {
   static unsigned int quadVAO = 0;
   const float quadData[] = {
      -1,  1, -1, 1,    -1,  1, // top left
      -1, -1, -1, 1,    -1, -1, // bottom left
       1,  1, -1, 1,     1,  1, // top right

       1,  1, -1, 1,     1,  1, // top right
      -1, -1, -1, 1,    -1, -1, // bottom left
       1, -1, -1, 1,     1, -1 // bottom right
   };

   float invProj[16], invView[16];
   mat4invertMatrix(ProjectionMatrix, invProj);
   mat4invertMatrix(ViewMatrix, invView);

   glUseProgram(cloudShader);

   //  Once initialized, just bind VAO
   if (quadVAO)
      glBindVertexArray(quadVAO);
   else {
      glGenVertexArrays(1,&quadVAO);
      glBindVertexArray(quadVAO);


      unsigned int vbo=0;
      glGenBuffers(1,&vbo);

      glBindBuffer(GL_ARRAY_BUFFER,vbo);

      glBufferData(GL_ARRAY_BUFFER,sizeof(quadData),quadData,GL_STATIC_DRAW);

      //  Bind arrays
      int loc = glGetAttribLocation(cloudShader,"Vertex");
      glVertexAttribPointer(loc,4,GL_FLOAT,0,sizeof(float) * 6,(void*) 0);
      glEnableVertexAttribArray(loc);

      for(int i = 0; i < 10; i++) ErrCheck("quad - vert");


      loc = glGetAttribLocation(cloudShader,"UV");
      glVertexAttribPointer(loc,2,GL_FLOAT,0,sizeof(float) * 6,(void*) (4 * sizeof(float)));
      glEnableVertexAttribArray(loc);
      ErrCheck("quad - uv");

   }


   mat4identity(ProjectionMatrix);
   mat4ortho(ProjectionMatrix, -1, 1, -1, 1, -1, 1);

   mat4identity(ViewMatrix);

   int id = glGetUniformLocation(cloudShader,"ProjectionMatrix");
   glUniformMatrix4fv(id,1,0,ProjectionMatrix);
   //  View matrix is the modelview matrix since model matrix is I
   id = glGetUniformLocation(cloudShader,"ModelViewMatrix");
   glUniformMatrix4fv(id,1,0,ViewMatrix);

   id = glGetUniformLocation(cloudShader,"invView");
   glUniformMatrix4fv(id,1,0,invView);
   //  View matrix is the modelview matrix since model matrix is I
   id = glGetUniformLocation(cloudShader,"invProj");
   glUniformMatrix4fv(id,1,0,invProj);

   id = glGetUniformLocation(cloudShader,"invView");
   glUniformMatrix4fv(id,1,0,invView);
   //  View matrix is the modelview matrix since model matrix is I
   id = glGetUniformLocation(cloudShader,"invProj");
   glUniformMatrix4fv(id,1,0,invProj);

   id = glGetUniformLocation(cloudShader,"eyePos");
   glUniform3fv(id, 1, eyePos);

   id = glGetUniformLocation(cloudShader,"FOV");
   glUniform1f(id, fov);

   id = glGetUniformLocation(cloudShader,"asp");
   glUniform1f(id, (float)asp);

   id = glGetUniformLocation(cloudShader,"time");
   glUniform1f(id, (float)glfwGetTime());

   id = glGetUniformLocation(cloudShader,"near");
   glUniform1f(id, zNear);

   id = glGetUniformLocation(cloudShader,"far");
   glUniform1f(id, zFar);

   id = glGetUniformLocation(cloudShader,"FBM");
   glUniform1i(id, FBM);

   id = glGetUniformLocation(cloudShader,"cloudHeight");
   glUniform1f(id, cloudHeight);

   id = glGetUniformLocation(cloudShader,"sunVector");
   glUniform3fv(id, 1, Position);


   int loc = glGetUniformLocation(cloudShader, "noiseTex");
   glUniform1i(loc, 0);

   loc = glGetUniformLocation(cloudShader, "sceneTex");
   glUniform1i(loc, 1);

   loc = glGetUniformLocation(cloudShader, "sceneDepthTex");
   glUniform1i(loc, 2);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_3D, cloudNoiseTexture);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, sceneTex);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, sceneDepthTex);


   glDisable(GL_DEPTH_TEST);
   glDrawArrays(GL_TRIANGLES,0,6);
   glEnable(GL_DEPTH_TEST);

   glBindVertexArray(0);
   glBindBuffer(GL_ARRAY_BUFFER,0);
}

void ocean() {
   int quads = 4;
   float quadSize = 5.0;
   float halfQuad = quadSize / 2.0;
   std::vector<float> verts;
   for(int qz = -quads; qz <= quads; qz++) {
      for(int qx = -quads; qx <= quads; qx++) {
         for(int corner = 0; corner < 4; corner++) {
            // 3 2
            // 0 1

            float x = float(qx) * quadSize;
            float z = float(qz) * quadSize;

            if(corner % 3 == 0) {
               x -= halfQuad;
            } else {
               x += halfQuad;
            }

            if(corner <= 1) {
               z -= halfQuad;
            } else {
               z += halfQuad;
            }

            verts.push_back(x);
            verts.push_back(0.0);
            verts.push_back(z);
            verts.push_back(1.0);
         }
      }
   }


   static GLuint oceanVAO = -1;
   GLuint oceanVBO = -1;
   if(oceanVAO == (unsigned int) -1) {;
      glPatchParameteri(GL_PATCH_VERTICES, 4);

      glGenVertexArrays(1, &oceanVAO);
      glGenBuffers(1, &oceanVBO);

      glBindVertexArray(oceanVAO);
      glBindBuffer(GL_ARRAY_BUFFER, oceanVBO);
      glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float) * 4, verts.data(), GL_STATIC_DRAW);

      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
                            4 * sizeof(float), (void*)0);
      glBindVertexArray(0);
   }


   glUseProgram(oceanShader);

   int loc = glGetUniformLocation(oceanShader, "view");
   glUniformMatrix4fv(loc, 1, 0, ViewMatrix);

   loc = glGetUniformLocation(oceanShader, "proj");
   glUniformMatrix4fv(loc, 1, 0, ProjectionMatrix);

   float NormalMatrix[9];
   mat3normalMatrix(ViewMatrix , NormalMatrix);
   loc = glGetUniformLocation(oceanShader,"norm");
   glUniformMatrix3fv(loc,1,0,NormalMatrix);

   loc = glGetUniformLocation(oceanShader, "eyePos");
   glUniform3fv(loc, 1, eyePos);

   loc = glGetUniformLocation(oceanShader, "sunDir");
   glUniform3fv(loc, 1,Position);

   loc = glGetUniformLocation(oceanShader, "time");
   glUniform1f(loc, glfwGetTime());

   glBindVertexArray(oceanVAO);
   glDrawArrays(GL_PATCHES, 0, verts.size()/4);
   glBindVertexArray(0);

}

//
//  Refresh display
//
void display(GLFWwindow* window){

   mat4identity(ProjectionMatrix);

   zNear = dim / 8;
   zFar = dim * 8;

   if (rotate) {
      th += 0.5f;
      th = fmod(th, 360.0f);
   }

   mat4perspective(ProjectionMatrix , fov,asp,zNear,zFar);

   mat4identity(ViewMatrix);

   eyePos[0] = -2*dim*Sin(th)*Cos(ph);
   eyePos[1] = +2*dim        *Sin(ph);
   eyePos[2] = +2*dim*Cos(th)*Cos(ph);
   mat4lookAt(ViewMatrix , eyePos[0],eyePos[1],eyePos[2] , 0,0,0 , 0,Cos(ph),0);

   //  Light position
   Position[0] = 4*Cos(zh);
   Position[1] = Ylight;
   Position[2] = 4*Sin(zh);
   Position[3] = 1;






   // RENDER SCENE TO FIRST PASS BUFFER;
   glBindFramebuffer(GL_FRAMEBUFFER,frameBuffer);
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   glEnable(GL_DEPTH_TEST);


   island(5.0, 5.0);
   ocean();


   glBindFramebuffer(GL_FRAMEBUFFER,0);
   glDisable(GL_DEPTH_TEST);
   glClear(GL_COLOR_BUFFER_BIT);

   clouds();

   glUseProgram(0);

   //  Render the scene and make it visible
   ErrCheck("display");
   glFlush();
   glfwSwapBuffers(window);
}


void generateSceneBuffers() {
   // GENERATE FRAME BUFFER AND TEXTURES FOR THE MAINS SCENE PASS
   glGenFramebuffers(1, &frameBuffer);
   glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);


   glGenTextures(1, &sceneTex);
   glBindTexture(GL_TEXTURE_2D, sceneTex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, windowWidth, windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTex, 0);


   glGenTextures(1, &sceneDepthTex);
   glBindTexture(GL_TEXTURE_2D, sceneDepthTex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
             windowWidth, windowHeight, 0,
             GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneDepthTex, 0);

   if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      printf("we got a frame buffer problem...\n");
      exit(1);
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void mouse_button(GLFWwindow* window, int button, int action, int mods) {
   if (button == GLFW_MOUSE_BUTTON_LEFT) {
      if (action == GLFW_PRESS) {
         lmbDown = true;
         glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
         glfwGetCursorPos(window, &prevX, &prevY);
      } else if (action == GLFW_RELEASE) {
         lmbDown = false;
         glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      }
   }
}



void cursor_position(GLFWwindow* window, double xpos, double ypos){
   if (lmbDown) {
      double dx = xpos - prevX;
      double dy = ypos - prevY;

      th += dx * sensitivity;
      ph += dy * sensitivity;

      if (ph > 89.0) ph = 89.0;
      if (ph < 5.0)  ph = 5.0;

      th = fmod(th, 360.0f);
      ph = fmod(ph, 360.0f);

      prevX = xpos;
      prevY = ypos;
   }
}


//
//  Key pressed callback
//
void key(GLFWwindow* window,int key,int scancode,int action,int mods)
{
   //  Discard key releases (keeps PRESS and REPEAT)
   if (action==GLFW_RELEASE) return;

   int shift = (mods & GLFW_MOD_SHIFT);

   //  Exit on ESC
   if (key == GLFW_KEY_ESCAPE)
      glfwSetWindowShouldClose(window,1);
   //  Toggle light movement
   else if (key == GLFW_KEY_S)
      move = 1 - move;
   else if (key == GLFW_KEY_M)
      rotate = 1 - rotate;
   //  Light elevation
   else if (key==GLFW_KEY_KP_ADD || key == GLFW_KEY_EQUAL)
      Ylight += 0.1;
   else if (key==GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS)
      Ylight -= 0.1;
   //  Right arrow key - increase angle by 5 degrees
   else if (key == GLFW_KEY_RIGHT)
      th += 5.0f;
   //  Left arrow key - decrease angle by 5 degrees
   else if (key == GLFW_KEY_LEFT)
      th -= 5.0f;
   //  Up arrow key - increase elevation by 5 degrees
   else if (key == GLFW_KEY_UP && !shift) {
      ph += 5.0f;
      if(ph > 90.0f) ph = 90.0f;
   }
   //  Down arrow key - decrease elevation by 5 degrees
   else if (key == GLFW_KEY_DOWN && !shift) {
      ph -= 5.0f;
      if(ph < 5.0f) ph = 5.0f;
   }
   //  PageUp key - increase dim
   else if (key == GLFW_KEY_PAGE_DOWN || (key == GLFW_KEY_DOWN && shift))
      dim += 0.1;
   //  PageDown key - decrease dim
   else if ((key == GLFW_KEY_PAGE_UP || (key == GLFW_KEY_UP && shift)) && dim>1)
      dim -= 0.1;
   else if (key == GLFW_KEY_E && erode <= 0)
      erode = 1;
   else if (key == GLFW_KEY_SPACE) {
      newIsland = true;
      erode = 32;
   } else if (key == GLFW_KEY_T)
      triplanar = 1 - triplanar;
   else if (key == GLFW_KEY_C && shift)
      cloudHeight += 0.05;
   else if (key == GLFW_KEY_C && !shift)
      cloudHeight -= 0.05;
   else if (key == GLFW_KEY_F)
      FBM = 1 - FBM;
   //  Keep angles to +/-360 degrees
   th = fmod(th, 360.0);
   ph = fmod(ph, 360.0);
}

//
//  Window resized callback
//
void reshape(GLFWwindow* window,int width,int height)
{
   //  Get framebuffer dimensions (makes Apple work right)
   glfwGetFramebufferSize(window,&width,&height);
   //  Ratio of the width to the height of the window
   asp = (height>0) ? (double)width/height : 1;
   //  Set the viewport to the entire window
   glViewport(0,0, width,height);

   windowWidth = width;
   windowHeight = height;


   glDeleteTextures(1, &sceneTex);
   glDeleteTextures(1, &sceneDepthTex);
   glDeleteFramebuffers(1, &frameBuffer);

   generateSceneBuffers();
}

//
//  Main program with GLFW event loop
//
int main(int argc,char* argv[]) {
   //  Initialize GLFW
   windowWidth = 1920;
   windowHeight = 1080;
   asp = (double)windowWidth/windowHeight;


   GLFWwindow* window = InitWindow("Island Generator by Ari Goldman",1,windowWidth,windowHeight,&reshape,&key);
   glfwSetMouseButtonCallback(window, mouse_button);
   glfwSetCursorPosCallback(window, cursor_position);

   //  Load textures
   const GLubyte* version    = glGetString(GL_VERSION);
   const GLubyte* glslVer    = glGetString(GL_SHADING_LANGUAGE_VERSION);
   const GLubyte* renderer   = glGetString(GL_RENDERER);
   const GLubyte* vendor     = glGetString(GL_VENDOR);

   printf("OpenGL Version: %s\n", version);
   printf("GLSL Version:   %s\n", glslVer);
   printf("Renderer:       %s\n", renderer);
   printf("Vendor:         %s\n", vendor);

   sandTexture = LoadTexBMP("textures/sand.bmp");
   stoneTexture = LoadTexBMP("textures/light-bumped-rock.bmp");
   grassTexture = LoadTexBMP("textures/patchy-meadow.bmp");

   // cloudNoiseTexture = LoadTexBMP("textures/cloudNoise.bmp");

   cloudNoiseTexture = generate3DNoiseTexture(128);

   //  Switch font to nearest
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
   //  Create Shader Programs
   islandShader = CreateShaderProg("shaders/island.vert", "shaders/island.frag");
   cloudShader = CreateShaderProg("shaders/clouds.vert", "shaders/clouds.frag");

   // erosionShader = CreateShaderProgCompute("shaders/erosion.compute");
   erosionShader_tex = CreateShaderProgCompute("shaders/erosion_tex.compute");

   shader[0] = CreateShaderProg("gl4pix.vert","gl4pix.frag");
   shader[1] = CreateShaderProg("gl4fix.vert","gl4fix.frag");
   shader[2] = CreateShaderGeom("gl4tex.vert","gl4tex.geom","gl4tex.frag");


   oceanShader = glCreateProgram();
   CreateShader(oceanShader, GL_VERTEX_SHADER, "shaders/ocean.vert");
   CreateShader(oceanShader, GL_TESS_CONTROL_SHADER, "shaders/ocean.tcs");
   CreateShader(oceanShader, GL_TESS_EVALUATION_SHADER, "shaders/ocean.tes");
   CreateShader(oceanShader, GL_FRAGMENT_SHADER, "shaders/ocean.frag");

   glLinkProgram(oceanShader);

   PrintProgramLog(oceanShader);



   generateSceneBuffers();


   //  Event loop
   ErrCheck("init");
   while(!glfwWindowShouldClose(window))
   {
      //  Light animation
      if (move) zh = fmod(90*glfwGetTime(),360);
      //  Display
      display(window);
      //  Process any events
      glfwPollEvents();
   }
   //  Shut down GLFW
   glfwDestroyWindow(window);
   glfwTerminate();
   return 0;
}
