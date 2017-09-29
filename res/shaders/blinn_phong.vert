#version 320 es
 
layout(location = 0) in vec4 vPos;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in mat4 iOffset;

uniform mat4 projection, modelview, normalMat;

out vec3 vertPos;
out vec3 normalInterp;
out vec2 texCoord;

void main(){
    gl_Position = projection * iOffset * modelview * vec4(vPos.xyz, 1.f);
    vec4 vertPos4 = modelview * vec4(vPos.xyz, 1.f);
    vertPos = vec3(vertPos4) / 1.f;//vertPos4.w;
    normalInterp = vec3(normalMat * vNormal);
    texCoord = vTexCoord;
}
