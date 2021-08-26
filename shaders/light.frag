out vec4 FragColor;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {    
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform DirLight global_light;
uniform PointLight point_light;
uniform vec3 viewPos;
uniform sampler2D tex;
uniform float shininess;

vec3 calc_dir_light(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    vec3 ambient  = light.ambient  * vec3(texture(tex, TexCoord));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(tex, TexCoord));
    vec3 specular = light.specular * spec * vec3(texture(tex, TexCoord));
    return (ambient + diffuse + specular);
}

vec3 calc_point_light(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			    light.quadratic * (distance * distance));

    vec3 ambient  = light.ambient  * vec3(texture(tex, TexCoord));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(tex, TexCoord));
    vec3 specular = light.specular * spec * vec3(texture(tex, TexCoord));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
} 

void main()
{
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 norm = normalize(Normal);
    vec3 result = calc_dir_light(global_light, norm, viewDir);
    result += calc_point_light(point_light, norm, FragPos, viewDir);
    FragColor = vec4(result, 1.0f);
}
