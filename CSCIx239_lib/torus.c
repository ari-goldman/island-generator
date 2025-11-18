//  CSCIx239 library
//  Willem A. (Vlakkies) Schreuder
#include "CSCIx239.h"

//
//  Solid unit torus
//
void SolidTorus(float r,int n)
{
   //  Loop along ring
   for (int i=0;i<n;i++)
   {
      float ph0 =  i   *360.0/n;
      float ph1 = (i+1)*360.0/n;
      //  Loop around ring
      glBegin(GL_QUAD_STRIP);
      for (int j=0;j<=n;j++)
      {
         float th = j*360.0/n;
         glNormal3d(Cos(ph1)*Cos(th),-Sin(ph1)*Cos(th),Sin(th)); glTexCoord2d(ph1/30.0,th/180.0); glVertex3d(Cos(ph1)*(1+r*Cos(th)),-Sin(ph1)*(1+r*Cos(th)),r*Sin(th));
         glNormal3d(Cos(ph0)*Cos(th),-Sin(ph0)*Cos(th),Sin(th)); glTexCoord2d(ph0/30.0,th/180.0); glVertex3d(Cos(ph0)*(1+r*Cos(th)),-Sin(ph0)*(1+r*Cos(th)),r*Sin(th));
      }
      glEnd();
   }
}

//
//  Textured unit torus
//
void TexturedTorus(float r,int n,int tex)
{
   //  Draw with texture
   if (tex)
   {
      glBindTexture(GL_TEXTURE_2D,tex);
      glEnable(GL_TEXTURE_2D);
      SolidTorus(r,n);
      glDisable(GL_TEXTURE_2D);
   }
   //  Draw without texture
   else
      SolidTorus(r,n);
}

//
// General torus
//
void Torus(float x,float y,float z , float R,float r, float th,float ph , int n,int tex)
{

   //  Transform
   glPushMatrix();
   glTranslated(x,y,z);
   glRotated(ph,1,0,0);
   glRotated(th,0,1,0);
   glScaled(R,R,R);
   //  Draw textured cube
   TexturedTorus(r/R,n,tex);
   // Restore
   glPopMatrix();
}
