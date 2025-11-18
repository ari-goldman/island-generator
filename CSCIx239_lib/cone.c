//  CSCIx239 library
//  Willem A. (Vlakkies) Schreuder
#include "CSCIx239.h"

//
//  Solid unit cone
//
void SolidCone(int n)
{
   //  Bottom (fan of triangles)
   glNormal3d(0,0,-1); 
   glBegin(GL_TRIANGLE_FAN);
   glTexCoord2d(0.5,0.5); glVertex3d(0,0,0);
   for (int i=0;i<=n;i++)
   {
      float th = i*360.0/n;
      glTexCoord2d(0.5*(Cos(th)+1),0.5*(Sin(th)+1)); glVertex3d(Cos(th),Sin(th),0);
   }
   glEnd();

   //  Cone Body (strip of quads)
   glBegin(GL_QUAD_STRIP);
   for (int i=0;i<=n;i++)
   {
      float th =  i*360.0/n;
      glNormal3d(Cos(th),Sin(th),0);
      glTexCoord2d(0,th/90.0); glVertex3d(Cos(th),Sin(th),0);
      glTexCoord2d(2,th/90.0); glVertex3d(0,0,1);
   }
   glEnd();
}

//
//  Textured unit cone
//
void TexturedCone(int n,int tex)
{
   //  Draw with texture
   if (tex)
   {
      glBindTexture(GL_TEXTURE_2D,tex);
      glEnable(GL_TEXTURE_2D);
      SolidCone(n);
      glDisable(GL_TEXTURE_2D);
   }
   //  Draw without texture
   else
      SolidCone(n);
}

//
// General cone
//
void Cone(float x,float y,float z , float r,float h, float th,float ph , int n,int tex)
{
   //  Transform
   glPushMatrix();
   glTranslated(x,y,z);
   glRotated(ph,1,0,0);
   glRotated(th,0,1,0);
   glScaled(r,r,h);
   //  Draw textured cube
   TexturedCone(n,tex);
   // Restore
   glPopMatrix();
}
