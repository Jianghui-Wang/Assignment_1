#version 410 core

in vec3 texCoord;

uniform sampler3D volume;

out vec4 color;

void main() {
    float v = texture(volume, texCoord).r;
    color   = vec4(v, v, v, 1.0);
}
