#version 430 core

in vec4 pos;

out vec4 vPos;

void main() {
    vPos = pos;

    gl_Position = pos;
}