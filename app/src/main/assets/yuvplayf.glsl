#version 300 es
precision highp float;

uniform sampler2D uni_textureY;
uniform sampler2D uni_textureU;
uniform sampler2D uni_textureV;

in vec2 out_uv;
out vec4 fragColor;

void main(void)
{
    vec3 yuv;
    vec3 rgb;
    yuv.x = texture(uni_textureY, out_uv).r;
    yuv.y = texture(uni_textureU, out_uv).r - 0.5;
    yuv.z = texture(uni_textureV, out_uv).r - 0.5;
    rgb = mat3( 1,1,1, 0,-0.39465,2.03211,1.13983,-0.58060,0) * yuv;
    fragColor = vec4(rgb, 1);
}
