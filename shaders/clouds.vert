#version 440 core

in vec4 Vertex;
in vec2 UV;

out vec2 uv;

uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;


void main() {
    uv = UV;
    gl_Position =  ProjectionMatrix * ModelViewMatrix * Vertex;
}
