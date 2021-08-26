layout(location = 0) in vec3 vertCoord;

uniform mat4 MVP;

out vec3 TexCoords;

void main()
{
    TexCoords = vertCoord;
    gl_Position = MVP * vec4(vertCoord, 1.0);
}
