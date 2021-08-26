in vec2 TexCoord;

uniform sampler2D TexSampler;
uniform vec4 color;

out vec4 FragColor;

void main()
{   
    vec4 texture = vec4(1., 1., 1., texture(TexSampler, TexCoord + vec2(.5)).r);
    FragColor = texture * color;
} 
