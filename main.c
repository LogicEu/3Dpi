#include <mass.h>
#include <gleex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

static const float half_pi = M_PI * 0.5f;

static unsigned int global_seed;
static unsigned int lightshader;
static unsigned int texshader;
static unsigned int skyboxshader;

static inline uint8_t lerpu8(uint8_t a, uint8_t b, float f)
{
    return (uint8_t)lerpf((float)a, (float)b, f);
}

static inline void pixel_lerp(uint8_t* dst, uint8_t* a, uint8_t* b, const uint8_t channels)
{
    for (uint8_t i = 0; i < channels; i++) {
        dst[i] = lerpu8(a[i], b[i], 0.5f);
    }
}

static unsigned int mesh_bind(const mesh_t* restrict mesh)
{
    if (!mesh || !mesh->vertices.size) {
        return 0;
    }

    unsigned int id = glee_buffer_id();

    glee_buffer_create(id, mesh->vertices.data, mesh->vertices.size * mesh->vertices.bytes);
    glee_buffer_attribute_set(0, 3, 0, 0);
    
    if (mesh->uvs.size) {
        glee_buffer_create(id, mesh->uvs.data, mesh->uvs.size * mesh->uvs.bytes);
        glee_buffer_attribute_set(1, 2, 0, 0);
    }

    if (mesh->normals.size) {
        glee_buffer_create(id, mesh->normals.data, mesh->normals.size * mesh->normals.bytes);
        glee_buffer_attribute_set(2, 3, 0, 0);
    }

    return id;
}

/* pass mesh with existing normal array */
static void mesh_smooth_optim(mesh_t* restrict mesh)
{
    table_t table = table_compress(&mesh->vertices);
    const size_t size = table.size;

    vec3* nn = mesh->normals.data;    
    vec3 normals[size];

    const vec3* v = table.data;
    for (size_t i = 0; i < size; ++i, ++v) {
        size_t* search = array_search_all(&mesh->vertices, v);
        vec3 n = {0.0, 0.0, 0.0};
    
        size_t j;
        for (j = 0; search[j]; ++j) {
            n = vec3_add(n, nn[search[j] - 1]);
        }

        normals[i] = vec3_normal(vec3_div(n, (float)j));
        free(search);
    }

    array_clear(&mesh->normals);
    for (size_t i = 0; table.indices[i]; ++i) {
        array_push(&mesh->normals, &normals[table.indices[i] - 1]);
    }

    table_free(&table);
}

static void mesh_smooth_sphere(mesh_t* restrict mesh)
{
    array_free(&mesh->normals);
    mesh->normals = array_copy(&mesh->vertices);
}

static void mesh_perlinize(mesh_t* restrict mesh, const float mult)
{   
    vec3* v = mesh->vertices.data;
    for (size_t i = 0; i < mesh->vertices.size; ++i, ++v) {
        v->y = perlin2d(v->x, v->z, 0.01f, 4, global_seed) * mult;
    }
    mesh_normals_get_face(mesh);
}

static void mesh_min_max_height(const mesh_t* restrict mesh, float* restrict min, float* restrict max)
{
    float max_ret = -1000.0f;
    float min_ret = 1000.0f;
    vec3* v = mesh->vertices.data;
    for (vec3* const end = v + mesh->vertices.size; v != end; ++v) {
        if (v->y > max_ret) max_ret = v->y;
        if (v->y < min_ret) min_ret = v->y;
    }
    *max = max_ret;
    *min = min_ret;
}

static void mesh_height_color_gradient(mesh_t* restrict mesh)
{
    float max, min;
    mesh_min_max_height(mesh, &min, &max);
    vec3* v = mesh->vertices.data;
    for (vec3* const end = v + mesh->vertices.size; v != end; ++v) {
        vec2 uv = vec2_new(inverse_lerpf(min, max, v->y), 0.0f);
        array_push(&mesh->uvs, &uv);
    }
}

static void scale_gradient_bitmap(bmp_t* bitmap)
{
    bmp_t b = bmp_new(bitmap->width * 2 - 1, 1, bitmap->channels);
    for (int i = 0; i < (int)b.width; i++) {
        if (i % 2 == 0) {
            memcpy(px_at(&b, i, 0), px_at(bitmap, i / 2, 0), b.channels);
        } else {
            int min = (i - 1) / 2;
            int max = (i + 1) / 2;
            min *= (min > 0);
            if (max > (int)bitmap->width) max = bitmap->width;
            pixel_lerp(px_at(&b, i, 0), px_at(bitmap, min, 0), px_at(bitmap, max, 0), b.channels);
        }
    }
    bmp_free(bitmap);
    *bitmap = b;
}

static void scale_gradient_bmp_irregular(bmp_t* bitmap)
{
    bmp_t b = bmp_new(bitmap->width * 2 - 1, 1, bitmap->channels);
    for (int i = 0; i < (int)b.width; i++) {
        if (i % 2 == 0) {
            memcpy(px_at(&b, i, 0), px_at(bitmap, i / 2, 0), b.channels);
        } else {
            int min = i / 2 - 1;
            int max = i / 2 + 1;
            min *= (min > 0);
            if (max > (int)bitmap->width) max = bitmap->width;
            pixel_lerp(px_at(&b, i, 0), px_at(bitmap, min, 0), px_at(bitmap, max, 0), b.channels);
        }
    }
    bmp_free(bitmap);
    *bitmap = b;
}

static bmp_t gradient_planet()
{
    bmp_t bitmap = bmp_new(6, 1, 4);
    uint8_t* buff = bitmap.pixels;
    buff[0] = 255, buff[1] = 255, buff[2] = 255, buff[3] = 255; //White
    buff[4] = 250, buff[5] = 248,  buff[6] = 220, buff[7] = 255; //Clear Brown
    buff[8] = 185, buff[9] = 145, buff[10] = 125, buff[11] = 255; //Brown
    buff[12] = 85, buff[13] = 45, buff[14] = 25, buff[15] = 255; //Brown
    buff[16] = 105, buff[17] = 85, buff[18] = 65, buff[19] = 255; //Grey
    buff[20] = 255, buff[21] = 255, buff[22] = 255, buff[23] = 255; //White
    return bitmap;
}

static bmp_t gradient_bitmap()
{
    bmp_t bitmap = bmp_new(8, 1, 4);
    uint8_t* buff = bitmap.pixels;
    buff[0] = 100, buff[1] = 100, buff[2] = 255, buff[3] = 255; //Blue
    buff[4] = 250, buff[5] = 248, buff[6] = 220, buff[7] = 255; //Clear Brown
    buff[8] = 105, buff[9] = 255, buff[10] = 105, buff[11] = 255; //Lighter Green
    buff[12] = 55, buff[13] = 255, buff[14] = 55, buff[15] = 255; //Green
    buff[16] = 0, buff[17] = 225, buff[18] = 0, buff[19] = 255; //Dark Green
    buff[20] = 185,buff[21] = 145,buff[22] = 125,buff[23] = 255; //Brown
    buff[24] = 105, buff[25] = 85, buff[26] = 65, buff[27] = 255; //Grey
    buff[28] = 255, buff[29] = 255, buff[30] = 255, buff[31] = 255; //White
    return bitmap;
}

static void bmp_smooth(bmp_t* restrict bitmap, const unsigned int smooth)
{
    for (int y = 0; y < (int)bitmap->height; y++) {
        for (int x = 0; x < (int)bitmap->width; x++) {
            if (*px_at(bitmap, x, y) > 105 && rand() % smooth != 0) {
                if (x + 1 < (int)bitmap->width) pixel_lerp(px_at(bitmap, x + 1, y), px_at(bitmap, x + 1, y), px_at(bitmap, x, y), bitmap->channels);
                if (x - 1 >= 0) pixel_lerp(px_at(bitmap, x - 1, y), px_at(bitmap, x - 1, y), px_at(bitmap, x, y), bitmap->channels);
                if (y + 1 < (int)bitmap->height) pixel_lerp(px_at(bitmap, x, y + 1), px_at(bitmap, x, y + 1), px_at(bitmap, x, y), bitmap->channels);
                if (y - 1 >= 0) pixel_lerp(px_at(bitmap, x, y - 1), px_at(bitmap, x, y - 1), px_at(bitmap, x, y), bitmap->channels);
            }
        }
    }
}

static bmp_t space_bitmap(const unsigned int w, const unsigned int h, const unsigned int n, const unsigned in, const unsigned int smooth)
{
    uint8_t color[] = {0, 0, 0, 0};
    bmp_t bitmap = bmp_color(w, h, 4, &color[0]);
    for (int y = 0; y < (int)bitmap.height; y++) {
        for (int x = 0; x < (int)bitmap.width; x++) {
            if (rand() % in > n) memset(px_at(&bitmap, x, y), 255, bitmap.channels);
        }
    }

    for (unsigned int i = 0; i < smooth; i++) {
        bmp_smooth(&bitmap, smooth);
    }

    return bitmap;
}

static texture_t space_cubemap(const unsigned int w, const unsigned int h, const unsigned int n, const unsigned int in, const unsigned int smooth)
{
    texture_t texture;
    texture.width = w;
    texture.height = h;
    glGenTextures(1, &texture.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture.id);

    for (int i = 0; i < 6; i++) {
        bmp_t bitmap = space_bitmap(w, h, n, in, smooth);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.pixels);
        bmp_free(&bitmap);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

static skybox_t space_skybox(const unsigned int w, const unsigned int h, const unsigned int n, const unsigned int in, const unsigned int smooth)
{
    skybox_t skybox;
    skybox.cubemap = space_cubemap(w, h, n, in, smooth);
    skybox.VAO = glee_buffer_skybox_create();
    return skybox;
}

void shaders_init()
{
    int w, h;
    glee_window_get_size(&w, &h);
    float width = (float)w, height = (float)h, scale = 1.0;

    texshader = glee_shader_load("shaders/texvert.frag", "shaders/texfrag.frag");
    glUniform3f(glGetUniformLocation(texshader, "resolution"), width, height, scale);
    glUniform4f(glGetUniformLocation(texshader, "camera"), 0.0f, 0.0f, 1.0f, 0.0f); 

    lightshader = glee_shader_load("shaders/lightv.frag", "shaders/light.frag");
    glUseProgram(lightshader);
    glUniform3f(glGetUniformLocation(lightshader, "global_light.direction"), -0.5f, -1.0f, -0.5f);
    glUniform3f(glGetUniformLocation(lightshader, "global_light.ambient"), 0.0f, 0.0f, 0.0f);
    glUniform3f(glGetUniformLocation(lightshader, "global_light.diffuse"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(lightshader, "global_light.specular"), 0.7f, 0.7f, 0.7f);

    glUniform3f(glGetUniformLocation(lightshader, "point_light.position"), 4.0f, 8.0f, 4.0f); 
    glUniform3f(glGetUniformLocation(lightshader, "point_light.ambient"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(lightshader, "point_light.diffuse"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(lightshader, "point_light.specular"), 1.0f, 1.0f, 1.0f);

    glUniform1f(glGetUniformLocation(lightshader, "point_light.constant"), .01f);
    glUniform1f(glGetUniformLocation(lightshader, "point_light.linear"), 0.01f);
    glUniform1f(glGetUniformLocation(lightshader, "point_light.quadratic"), 0.01f);
    glUniform1f(glGetUniformLocation(lightshader, "shininess"), 32.0f);

    skyboxshader = glee_shader_load("shaders/skyboxvert.frag", "shaders/skyboxfrag.frag");
}

int main()
{
    glee_init();
    glee_window_create("Space", 800, 600, 0, 0);
    glee_mode_set(GLEE_MODE_3D);
    shaders_init();

    //unsigned int quad = glee_buffer_quad_create();
    global_seed = rand_uint(rand_uint(time(NULL)));
    printf("Seed: %u\n", global_seed);

    unsigned int draw_kind = 4;
    vec3 position = {0., 3., -5.};

    float v = 0.0f, h = 0.0f;
    const float speed = 15.0f;

    float div = 32.0f;
    float last_time = glee_time_get();

    bmp_t bitmap = gradient_planet();
    for (int i = 0; i < 8; i++) {
        scale_gradient_bmp_irregular(&bitmap);
    }

    texture_t planet = texture_from_bmp(&bitmap);
    mesh_t mesh2 = mesh_shape_sphere(8);
    mesh_t mesh = mesh_shape_sphere(4);
    mesh_scale(&mesh, 12.0f);
    mesh_scale(&mesh2, 6.0f);
    mesh_move(&mesh2, vec3_new(34.0f, 0.0f, 0.0f));
    mesh_merge(&mesh, &mesh2);
    mesh_height_color_gradient(&mesh);
    
    unsigned int id_planet = mesh_bind(&mesh);
    printf("Planet mesh: %zu, %u\n", mesh.vertices.size, id_planet);
    printf("Time: %f\n", glee_time_get() - last_time);

    last_time = glee_time_get();
    bitmap = gradient_bitmap();
    for (int i = 0; i < 4; i++) {
        scale_gradient_bitmap(&bitmap);
    }  

    texture_t gradient = texture_from_bmp(&bitmap);
    mesh_t plane = mesh_shape_plane(100, 100);
    mesh_perlinize(&plane, div);
    //mesh_smooth_optim(&plane);
    mesh_height_color_gradient(&plane);
    
    unsigned int id_floor = mesh_bind(&plane);
    
    printf("Plane mesh: %zu, %u\n", plane.vertices.size, id_floor);
    printf("Time: %f\n", glee_time_get() - last_time);

    bool flag = true;
    skybox_t skybox = space_skybox(1080, 1080, 998, 1000, 2);
    mat4 mat_id = mat4_id();

    int _w, _h;
    glee_window_get_size(&_w, &_h);
    float width = (float)_w, height = (float)_h;
    float aspect = width / height;
    float planet_rot = 0.0f;
    char delta_string[20] = "0.0000";
    int alarm = 20;

    mat4 projection = mat4_perspective_RH(deg_to_rad(45.0f), aspect, 0.1f, 1000.0f);

    glee_screen_color(0.0f, 0.0f, 0.2f, 1.0f);
    while(glee_window_is_open()) {
        glee_screen_clear(GLEE_MODE_3D);

        float delta_time = glee_time_delta(&last_time);
        float delta_speed = delta_time * speed;
        alarm --;
        if (alarm < 0) {
            sprintf(delta_string, "%f", delta_time);
            alarm = 30;
        }
        printf("%f\r", delta_time);

        glee_mouse_pos_3d(&h, &v);
        h *= 0.01f;
        v *= 0.01f;

        vec3 direction = {cos(v) * sin(h), sin(v), cos(v) * cos(h)};
        vec3 right = {sin(h - half_pi), 0.0f, cos(h - half_pi)};
        vec3 up = vec3_cross(right, direction);
 
        mat4 view = mat4_look_at_RH(position, vec3_add(position, direction), up);

        if (glee_key_pressed(GLFW_KEY_ESCAPE)) break;
        if (glee_key_down(GLFW_KEY_D)) 
            position = vec3_add(position, vec3_mult(right, delta_speed));
        if (glee_key_down(GLFW_KEY_A)) 
            position = vec3_sub(position, vec3_mult(right, delta_speed));
        if (glee_key_down(GLFW_KEY_W)) 
            position = vec3_add(position, vec3_mult(direction, delta_speed));
        if (glee_key_down(GLFW_KEY_S)) 
            position = vec3_sub(position, vec3_mult(direction, delta_speed));
        if (glee_key_down(GLFW_KEY_Z)) 
            position = vec3_add(position, vec3_mult(up, delta_speed));
        if (glee_key_down(GLFW_KEY_X)) 
            position = vec3_sub(position, vec3_mult(up, delta_speed));
        if (glee_key_down(GLFW_KEY_UP)) position.z += delta_speed;
        if (glee_key_down(GLFW_KEY_DOWN)) position.z -= delta_speed;
        if (glee_key_down(GLFW_KEY_RIGHT)) position.x += delta_speed;
        if (glee_key_down(GLFW_KEY_LEFT)) position.x -= delta_speed;
        if (glee_key_pressed(GLFW_KEY_M)) {
            mesh_perlinize(&plane, ++div);
        }
        if (glee_key_pressed(GLFW_KEY_N)) {
            mesh_perlinize(&plane, --div);
        }
        if (glee_key_pressed(GLFW_KEY_SPACE)) {
            if (flag) flag = false;
            else flag = true;
        }
        if (glee_key_pressed(GLFW_KEY_V)) {
            if (draw_kind == 0x0004) draw_kind = 0x0001;
            else draw_kind = 0x0004;
        }
        if (glee_key_pressed(GLFW_KEY_R)) {
            global_seed = rand_uint(rand());
            mesh_perlinize(&plane, div);
        }
        if (glee_key_pressed(GLFW_KEY_P)) {
            mesh_save(&plane, "mesh.obj");
        }
        if (glee_key_pressed(GLFW_KEY_O)) {
            mesh_save_quick(&plane, "meshq.obj");
        }
        if (glee_key_pressed(GLFW_KEY_B)) {
            mesh_smooth_sphere(&mesh);
        }

        if (flag) {
            mat4 mat = mat4_mult(projection, mat4_mult(mat4_look_at(vec3_uni(0.0f), direction, up), mat4_id()));
            glDepthMask(GL_FALSE);
            glUseProgram(skyboxshader);
            glBindVertexArray(skybox.VAO);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.cubemap.id);
            glUniformMatrix4fv(glGetUniformLocation(skyboxshader, "MVP"), 1, GL_FALSE, &mat.data[0][0]);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glDepthMask(GL_TRUE);
        }

        planet_rot += delta_time;
        mat4 planet_model = mat4_model(vec3_new(20.0f, 0.0f, 20.0f), vec3_uni(1.0f), vec3_new(0.0f, 1.0f, 0.0f ), planet_rot);
        
        glUseProgram(lightshader);
        glBindVertexArray(id_planet);
        glBindTexture(GL_TEXTURE_2D, planet.id);
        glUniform3f(glGetUniformLocation(lightshader, "point_light.position"), position.x, position.y, position.z);
        glUniform3f(glGetUniformLocation(lightshader, "viewPos"), position.x, position.y, position.z);
        glUniformMatrix4fv(glGetUniformLocation(lightshader, "view"), 1, GL_FALSE, &view.data[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(lightshader, "projection"), 1, GL_FALSE, &projection.data[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(lightshader, "model"), 1, GL_FALSE, &planet_model.data[0][0]);
        glDrawArrays(draw_kind, 0, mesh.vertices.size);

        glBindVertexArray(id_floor);
        glBindTexture(GL_TEXTURE_2D, gradient.id);
        glUniformMatrix4fv(glGetUniformLocation(lightshader, "model"), 1, GL_FALSE, &mat_id.data[0][0]);
        glDrawArrays(draw_kind, 0, plane.vertices.size); 
        
        glee_screen_refresh();
    } 
    
    mesh_free(&mesh);
    mesh_free(&plane);
    
    glee_deinit();
    return EXIT_SUCCESS;;
}
