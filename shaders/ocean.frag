// ocean.frag
#version 430 core

in vec3 WorldNormal;
in vec3 Normal;
in vec3 WorldPos;
in vec3 Light;


out vec4 FragColor;

uniform vec3 eyePos;


void main(){
    vec3 View = -eyePos;

    //  N is the object normal
    vec3 N = normalize(Normal);
    //  L is the light vector
    vec3 L = normalize(Light);
    //  R is the reflected light vector R = 2(L.N)N - L
    vec3 R = reflect(-L,N);
    //  V is the view vector (eye vector)
    vec3 V = normalize(eyePos - WorldPos);

    //  Diffuse light is cosine of light and normal vectors
    float Id = max(dot(L,N) , 0.0);
    //  Specular is cosine of reflected and view vectors
    float Shinyness = 15.0;
    float Is = (Id>0.0) ? pow(max(dot(R,V),0.0) , Shinyness) : 0.0;

    vec3 WN = normalize(WorldNormal);



    //  Sum color types
    //           emission     ambient                      diffuse                       specular
    vec4 color = vec4(0.0) + vec4(0.2, 0.2, 0.4, 1.0) + Id* vec4(0.3, 0.4, 0.5, 1.0) + Is*vec4(1.0);
    color.g = color.g * (1.0 + WorldPos.y * 2.0);
    color.a = 1.0;


    float foamStart = 1.5;
    float foamEnd   = 2.5;


    float d = length(WorldPos.xz);
    float f = clamp((d - foamStart)/(foamEnd - foamStart), 0.0, 1.0);

    color = mix(vec4(1.0), color, f);

    //  Apply texture
    FragColor = color;
}
