#version 330 core
in vec2 TexCoord;
in vec4 Color;

out vec4 FragColor;

uniform sampler2D spriteTexture;
uniform vec4 tintColor;

void main()
{
    vec4 texColor = texture(spriteTexture, TexCoord);
    FragColor = texColor * Color * tintColor;
}
