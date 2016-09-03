#extension GL_ARB_explicit_attrib_location : require
#extension GL_ARB_explicit_uniform_location : require

in vec2 tex_coords;
out vec4 color;

uniform sampler2D image;
uniform vec3 sprite_color;

void main()
{
    color = vec4(spriteColor, 1.0) * texture2D(image, tex_coords);
}
