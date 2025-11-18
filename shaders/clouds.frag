#version 440 core

in vec2 uv; // ACTUALLY NDC COORDINATES

out vec4 FragColor;

uniform mat4 invProj, invView;
uniform vec3 eyePos;
uniform float asp;
uniform float FOV;
uniform sampler3D noiseTex;
uniform float time;


uniform sampler2D sceneTex;
uniform sampler2D sceneDepthTex;
uniform float near;
uniform float far;

uniform float cloudHeight;
uniform int FBM;

uniform vec3 sunVector;

const float absorption = 0.1;
const float insideStep = 0.01;

#define MAX_ITERATIONS 1000
#define EPSILON 0.001
#define DEG_TO_RAD 0.017453292519943295769236907684886f
#define MARCH_STEP 0.01


// https://iquilezles.org/articles/distfunctions/
float sdEllipsoid( vec3 p, vec3 r )
{
    float k0 = length(p/r);
    float k1 = length(p/(r*r));
    return k0*(k0-1.0)/k1;
}


float distanceToScene(vec3 p) {
    return sdEllipsoid(p - vec3(0.0, cloudHeight, 0.0), vec3(3.0, 0.5, 3.0));
}

float noise3D(vec3 p) {
    return (2.0 * texture(noiseTex, (p + 0.5) * 0.2 + vec3(time, sin(0.2 * time), time * 0.75) * 0.01).r - 1.0);
}



float fbm(vec3 p) {
    float sum = 0.0, amp = 0.5;
    for(int i = 0; i < 5; i++){
        sum += amp * noise3D(p);
        p *= 2.0;
        amp *= 0.5;
    }
    return sum;
}

float distanceToSceneNoise(vec3 p) {
    if(FBM == 1) {
        return distanceToScene(p) + fbm(p);
    } else {
        return distanceToScene(p) + noise3D(p);
    }
}

vec3 CalcNormal(in vec3 pos){
    const vec3 SMALL_STEP = vec3(0.0001, 0.0, 0.0);

    // calculates the gradient - muy expensive
    float _dX = distanceToScene(pos + SMALL_STEP.xyy) - distanceToScene(pos - SMALL_STEP.xyy);
    float _dY = distanceToScene(pos + SMALL_STEP.yxy) - distanceToScene(pos - SMALL_STEP.yxy);
    float _dZ = distanceToScene(pos + SMALL_STEP.yyx) - distanceToScene(pos - SMALL_STEP.yyx);

    return normalize(vec3(_dX, _dY, _dZ));
}

void main() {

    vec4 sceneColor = texture(sceneTex, uv * 0.5 + 0.5);
    vec4 oceanColor = vec4(0.2, 0.2, 0.4, 1.0);
    vec4 skyColor = vec4(0.79, 0.807, 0.98, 1.0);
    vec4 sunColor = vec4(1.0, 1.0, 0.95, 1.0);



    float depth = texture(sceneDepthTex, uv * 0.5 + 0.5).r;

    // linarize depth
    // https://www.geeks3d.com/20091216/geexlab-how-to-visualize-the-depth-buffer-in-glsl/
    //    depth = depth * 2.0 - 1.0;
    //    depth = 2.0 * near * far/ (far + near - depth * (far - near));
    //    depth *= -1;

    depth = depth * 2.0 - 1.0;

    vec4 clipSpaceDepth = vec4(uv, depth, 1.0);
    vec4 viewSpaceDepth = invProj * clipSpaceDepth;
    viewSpaceDepth /= viewSpaceDepth.w;

    vec3 worldSpaceDepth = (invView * viewSpaceDepth).xyz;

    float maxT = length(worldSpaceDepth - eyePos);

    //    depth = depth * (far - near) + near;
    //    FragColor = vec4(vec3(depth), 1.0);
    //    return;

    vec4 clipSpacePos = vec4(uv, 0.0, 1.0); // uv, -1.0, 1.0 ?
    vec4 eye = invProj * clipSpacePos; // clip space -> view space
    eye /= eye.w;

    float tanF = tan(FOV * 0.5 * DEG_TO_RAD);
    vec3 rayDirView = normalize(vec3(
        uv.x * asp     * tanF,
        uv.y           * tanF,
        -1.0
    ));

    vec3 dir = normalize((invView * vec4(rayDirView, 0.0)).xyz); // view space -> world space
    vec3 sunDir = -normalize(sunVector);

    // if going down, water
    skyColor = mix(skyColor, sunColor, smoothstep(0.999, 0.9999619, dot(dir, -sunDir)));
    skyColor = mix(oceanColor, skyColor, smoothstep(0.0, 0.0, dir.y));
    sceneColor = mix(skyColor, sceneColor, sceneColor.a);


    vec4 result = vec4(0.0);


    float t = 0.0001;
    vec3 p;
    bool hit = false;
    for(int i = 0; i < MAX_ITERATIONS; i++) {
        p = eyePos + dir * t;
//        if(t > far) {
//            break;
//        }
        if(t > maxT) {
            break;
        }
        float d = -distanceToSceneNoise(p);
        if(d > 0.0) {
            d *= 0.2;
            float diffuse = clamp((d + distanceToSceneNoise(p - 0.3 * sunDir)) / 0.3, 0.0, 1.0 );

            vec3 baseCloudColor = vec3(0.6, 0.6, 0.75);
            vec3 litColor = vec3(0.9, 0.8, 0.6) * diffuse;

            vec4 color = vec4(mix(vec3(1.0), vec3(0.0), d), d);
            color.rgb *= baseCloudColor + litColor;
            color.rgb *= color.a; // square it essentially
            result += color * (1.0 - result.a); // only 1-a light left to absorb, so absorb that
        }
        t += MARCH_STEP;
    }


    FragColor = vec4(mix(sceneColor.xyz, result.xyz, result.a), 1.0);
}
