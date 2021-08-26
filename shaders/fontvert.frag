layout (location = 0) in vec2 texCoord;

uniform vec3 resolution;
uniform vec4 camera;
uniform vec4 trans;
uniform vec4 scale_rot;

out vec2 TexCoord;

vec2 pixel_to_screen(vec2 pos, vec3 res)
{
   float x = ((2.*pos.x/(res.y/res.z)) - (res.x/res.y))*(res.y/res.x);
   float y = ((2.*pos.y/(res.y/res.z)) - 1.);
   return vec2(x, y);
}

vec2 rot_and_scale_around(vec2 vec, vec2 origin, float scale, float rot)
{
   float x = scale*cos(rot)*(vec.x - origin.x) - scale*sin(rot)*(vec.y - origin.y) + origin.x;
   float y = scale*sin(rot)*(vec.x - origin.x) + scale*cos(rot)*(vec.y - origin.y) + origin.y;
   return vec2(x, y);
}

vec2 rot_and_scale(vec2 vec, vec2 origin, vec2 wh, float scale, float rot)
{
   float x = scale*cos(rot)*(vec.x - origin.x)*wh.x - scale*sin(rot)*(vec.y - origin.y)*wh.y + origin.x;
   float y = scale*sin(rot)*(vec.x - origin.x)*wh.x + scale*cos(rot)*(vec.y - origin.y)*wh.y + origin.y;
   return vec2(x, y);
}

void main()
{
   vec2 origin = vec2(scale_rot.z / resolution.x, scale_rot.w / resolution.y);
   vec2 position = rot_and_scale(texCoord, origin, trans.zw, scale_rot.x, scale_rot.y);
   position += rot_and_scale_around(trans.xy, scale_rot.zw, scale_rot.x, scale_rot.y);

   position = pixel_to_screen(position, resolution);
   TexCoord = vec2(texCoord.x, -texCoord.y);
   gl_Position = vec4(position, 0., 1.);
}
