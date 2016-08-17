#extension GL_ARB_explicit_attrib_location : require
#extension GL_ARB_explicit_uniform_location : require

in vec3 ourColor;
in vec2 TexCoord;

out vec4 color;

uniform sampler2D ourTexture1;
uniform sampler2D ourTexture2;

void main()
{
//    color = texture2D(ourTexture, TexCoord);
//    color = texture2D(ourTexture1, TexCoord) * vec4(ourColor, 1.0f);
    color = mix(texture2D(ourTexture1, TexCoord), texture2D(ourTexture2, TexCoord), 0.2);
}
