#version 400 core

in      vec4  Kd;  // Diffuse
uniform vec4  Ks;  // Specular
uniform vec4  Ke;  // Emissions
uniform float Shinyness;

//  Light vectors
in vec3 View;
in vec3 Light;
in vec3 Norm;
in vec3 WorldNorm;
in vec3 WorldPos;

//  Light colors
uniform vec4 Ambient;
uniform vec4 Diffuse;
uniform vec4 Specular;


in vec2 TexCoord;

uniform sampler2D texGrass;
uniform sampler2D texStone;
uniform sampler2D texSand;

uniform int Triplanar;

const float sandHeightTop = 0.14;
const float sandHeightBottom = 0.04;

//  Fragment color
out vec4 FragColor;

// triplanar mapping
vec4 triplanarMap(sampler2D tex) {
    vec3 blend = abs(WorldNorm);
    blend += 0.00001;
    blend /= dot(blend, vec3(1.0)); // average

    vec2 uvX = WorldPos.yz;
    vec2 uvY = WorldPos.xz;
    vec2 uvZ = WorldPos.xy;

    vec4 texX = texture(tex, uvX);
    vec4 texY = texture(tex, uvY);
    vec4 texZ = texture(tex, uvZ);

    return blend.x * texX + blend.y * texY + blend.z * texZ;
}


void main() {
    if(length(WorldPos.xz) > 2.5) discard;

    //  N is the object normal
    vec3 N = normalize(Norm);
    //  L is the light vector
    vec3 L = normalize(Light);
    //  R is the reflected light vector R = 2(L.N)N - L
    vec3 R = reflect(-L,N);
    //  V is the view vector (eye vector)
    vec3 V = normalize(View);

    //  Diffuse 
    float Id = max(dot(L,N) , 0.0);
    //  Specular
    float Is = (Id>0.0) ? pow(max(dot(R,V),0.0) , Shinyness) : 0.0;

    vec3 WN = normalize(WorldNorm);

    vec4 TextureColor;

    vec4 grassColor;
    vec4 rockColor;
    vec4 sandColor;

    if(Triplanar == 1) {
        grassColor = triplanarMap(texGrass);
        rockColor = triplanarMap(texStone);
        sandColor = triplanarMap(texSand);
    } else {
        grassColor = texture(texGrass, TexCoord);
        rockColor = texture(texStone, TexCoord);
        sandColor = texture(texSand, TexCoord);
    }

    float t = smoothstep(0.3, 0.5, WN.y);
    TextureColor = mix(rockColor, grassColor, t);

    t = smoothstep(sandHeightTop, sandHeightBottom, WorldPos.y);
    TextureColor = mix(TextureColor, sandColor, t);


    //  Sum color types
    vec4 color = Ke + TextureColor*Ambient + Id*TextureColor*Diffuse + Is*TextureColor*Specular;

    //  Apply texture
    FragColor = color;
}
