#version 410 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 tCoord;

out vec3 texCoord;

uniform mat4 model;        // scales the quad so the slice keeps its aspect
uniform mat4 texTransform; // maps (u, v, 0, 1) to a 3D sample location

void main() {
    gl_Position = model * vec4(pos, 1.0);
    texCoord    = (texTransform * vec4(tCoord, 1.0)).xyz;
}
