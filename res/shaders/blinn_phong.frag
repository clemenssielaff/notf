#version 320 es

precision mediump float;

in vec3 v_position;
in vec3 v_normal;
in vec2 v_texcoord;

const vec3 lightPos = vec3(1000.0,1000.0,1000.0);
const vec3 ambientColor = vec3(0.1, 0.0, 0.0);
const vec3 diffuseColor = vec3(0.5, 0.0, 0.0);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 16.0;

uniform sampler2D s_texture;

layout(location=0) out vec4 f_color;

void main() {

  vec3 normal = normalize(v_normal);
  vec3 lightDir = normalize(lightPos - v_position);

  float lambertian = max(dot(lightDir,normal), 0.0);
  float specular = 0.0;

  //vec3 diffuseColor = texture(s_texture, v_texcoord).xyz;
  
  if(lambertian > 0.0) {

    vec3 viewDir = normalize(-v_position);

    vec3 halfDir = normalize(lightDir + viewDir);
    float specAngle = max(dot(halfDir, normal), 0.0);
    specular = pow(specAngle, shininess);
  }
  
  vec3 colorLinear = ambientColor +
                     lambertian * diffuseColor +
                     specular * specColor;
  //f_color = vec4(colorLinear, 1.0) + texture(s_texture, v_texcoord);
  f_color = texture(s_texture, v_texcoord);
}
