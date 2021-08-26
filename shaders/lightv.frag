layout (location = 0) in vec3 vertCoord;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normalCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(vertCoord, 1.0));
    Normal = mat3(transpose(inverse(model))) * normalCoord;
    TexCoord = texCoord;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
