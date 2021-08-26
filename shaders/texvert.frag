layout (location = 0) in vec2 texCoord;

uniform vec3 resolution;
uniform vec4 camera;
uniform vec4 trans;
uniform vec2 scale_rot;

out vec2 TexCoord;

vec2 pixel_to_screen(vec2 pos, vec3 res)
{
   float x = ((2.*pos.x/(res.y/res.z)) - (res.x/res.y))*(res.y/res.x);
   float y = ((2.*pos.y/(res.y/res.z)) - 1.);
   return vec2(x, y);
}

vec2 rot_and_scale_around(vec2 vec, float scale, float rot)
{
   float x = scale*cos(rot)*(vec.x) - scale*sin(rot)*(vec.y);
   float y = scale*sin(rot)*(vec.x) + scale*cos(rot)*(vec.y);
   return vec2(x, y);
}

vec2 rot_and_scale(vec2 vec, vec2 wh, float scale, float rot)
{
   float x = scale*cos(rot)*(vec.x)*wh.x - scale*sin(rot)*(vec.y)*wh.y;
   float y = scale*sin(rot)*(vec.x)*wh.x + scale*cos(rot)*(vec.y)*wh.y;
   return vec2(x, y);
}

vec2 camera_transform(vec2 vec, vec2 origin, float scale, float rot)
{
   float x = scale*cos(rot)*(vec.x - origin.x) - scale*sin(rot)*(vec.y - origin.y) + origin.x;
   float y = scale*sin(rot)*(vec.x - origin.x) + scale*cos(rot)*(vec.y - origin.y) + origin.y;
   return vec2(x, y);
}

void main()
{
   vec2 position = rot_and_scale(texCoord, trans.zw, scale_rot.x, scale_rot.y);
   position += rot_and_scale_around(vec2(-trans.z/2., trans.w/2.), scale_rot.x, scale_rot.y) + trans.xy - camera.xy;
   position = camera_transform(position, vec2(resolution.x/resolution.z, resolution.y/resolution.z)/2., camera.z, camera.w);

   position = pixel_to_screen(position, resolution);
   TexCoord = vec2(texCoord.x, -texCoord.y);
   gl_Position = vec4(position, 0., 1.);
}
