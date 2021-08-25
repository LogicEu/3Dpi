#include <mass.h>
#include <gleex.h>
#include <stdbool.h>
#include <string.h>

//#define BUG printf("File: %s, Line: %u, Func: %s\n", __FILE__, __LINE__, __func__)

unsigned int global_seed, lightshader, texshader, framebuffershader, skyboxshader, fontshader;
const float half_pi = 3.14f / 2.0f;

void vertex_element_buffer_obj(unsigned int id, array_t* vertices, array_t* indices, obj_flag type)
{
    if (!vertices || !indices) return;
    unsigned int VBO, EBO;
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices->used * vertices->bytes, vertices->data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->used * indices->bytes, indices->data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertices->bytes, (void*)0);
    if (type == OBJ_VTN) {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertices->bytes, (void*)offsetof(vertex_t, uv));
    }
    if (type != OBJ_V) {
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, vertices->bytes, (void*)offsetof(vertex_t, normal));
    }
}

void vmesh_bind(unsigned int id, vmesh_t* mesh)
{
    if (!mesh) return;
    vertex_element_buffer_obj(id, mesh->vertices, mesh->indices, mesh->type);
}

void perlin_mesh(mesh_t* mesh, float mult)
{   
    for (int i = 0; i < mesh->vertices->used; i++) {
        vec3* v = (vec3*)array_index(mesh->vertices, i);
        v->y = perlin2d(v->x, v->z, 0.01, 4, global_seed) * mult;
    }
    mesh->normals = vec3_face_normal_array(mesh->vertices);
}

void perlin_vmesh(vmesh_t* mesh, float mult)
{
    for (int i = 0; i < mesh->vertices->used; i++) {
        vertex_t* v = (vertex_t*)array_index(mesh->vertices, i);
        vec3 pos = v->position;
        v->position.y = perlin2d(pos.x, pos.z, 0.01, 4, global_seed) * mult;
    }
    mesh->type = OBJ_VN;
    vertex_array_set_face_normal(mesh->vertices, mesh->indices);
}

uint8_t lerpu8(uint8_t a, uint8_t b, float f)
{
    return (uint8_t)lerpf((float)a, (float)b, f);
}

void pixel_lerp(uint8_t* dst, uint8_t* a, uint8_t* b, uint8_t channels)
{
    for (int i = 0; i < channels; i++) {
        dst[i] = lerpu8(a[i], b[i], 0.5f);
    }
}

void scale_gradient_bitmap(bmp_t* bitmap)
{
    bmp_t b = bmp_new(bitmap->width * 2 - 1, 1, bitmap->channels);
    for (int i = 0; i < b.width; i++) {
        if (i % 2 == 0) {
            memcpy(px_at(&b, i, 0), px_at(bitmap, i / 2, 0), b.channels);
        } else {
            int min = (i - 1) / 2;
            int max = (i + 1) / 2;
            min *= (min > 0);
            if (max > bitmap->width) max = bitmap->width;
            pixel_lerp(px_at(&b, i, 0), px_at(bitmap, min, 0), px_at(bitmap, max, 0), b.channels);
        }
    }
    bmp_free(bitmap);
    *bitmap = b;
}

void scale_gradient_bmp_irregular(bmp_t* bitmap)
{
    bmp_t b = bmp_new(bitmap->width * 2 - 1, 1, bitmap->channels);
    for (int i = 0; i < b.width; i++) {
        if (i % 2 == 0) {
            memcpy(px_at(&b, i, 0), px_at(bitmap, i / 2, 0), b.channels);
        } else {
            int min = i / 2 - 1;
            int max = i / 2 + 1;
            min *= (min > 0);
            if (max > bitmap->width) max = bitmap->width;
            pixel_lerp(px_at(&b, i, 0), px_at(bitmap, min, 0), px_at(bitmap, max, 0), b.channels);
        }
    }
    bmp_free(bitmap);
    *bitmap = b;
}

bmp_t gradient_planet()
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

bmp_t gradient_bitmap()
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

void bmp_smooth(bmp_t* bitmap, const unsigned int smooth)
{
    for (int y = 0; y < bitmap->height; y++) {
        for (int x = 0; x < bitmap->width; x++) {
            if (*px_at(bitmap, x, y) > 105 && rand() % smooth != 0) {
                if (x + 1 < bitmap->width) pixel_lerp(px_at(bitmap, x + 1, y), px_at(bitmap, x + 1, y), px_at(bitmap, x, y), bitmap->channels);
                if (x - 1 >= 0) pixel_lerp(px_at(bitmap, x - 1, y), px_at(bitmap, x - 1, y), px_at(bitmap, x, y), bitmap->channels);
                if (y + 1 < bitmap->height) pixel_lerp(px_at(bitmap, x, y + 1), px_at(bitmap, x, y + 1), px_at(bitmap, x, y), bitmap->channels);
                if (y - 1 >= 0) pixel_lerp(px_at(bitmap, x, y - 1), px_at(bitmap, x, y - 1), px_at(bitmap, x, y), bitmap->channels);
            }
        }
    }
}

bmp_t space_bitmap(const unsigned int w, const unsigned int h, const unsigned int n, const unsigned in, const unsigned int smooth)
{
    uint8_t color[] = {0, 0, 0, 0};
    bmp_t bitmap = bmp_color(w, h, 4, &color[0]);
    for (int y = 0; y < bitmap.height; y++) {
        for (int x = 0; x < bitmap.width; x++) {
            if (rand() % in > n) memset(px_at(&bitmap, x, y), 255, bitmap.channels);
        }
    }
    for (int i = 0; i < smooth; i++) {
        bmp_smooth(&bitmap, smooth);
    }
    return bitmap;
}

texture_t space_cubemap(const unsigned int w, const unsigned int h, const unsigned int n, const unsigned int in, const unsigned int smooth)
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

skybox_t* space_skybox(const unsigned int w, const unsigned int h, const unsigned int n, const unsigned int in, const unsigned int smooth)
{
    skybox_t* skybox = (skybox_t*)malloc(sizeof(skybox_t));
    skybox->cubemap = space_cubemap(w, h, n, in, smooth);
    skybox->VAO = glee_buffer_skybox_create();
    return skybox;
}

void shaders_init_this()
{
    unsigned int w, h, s;
    glee_window_get_size(&w, &h);
    s = glee_get_2d_scale();
    float width = (float)(int)w, height = (float)(int)h, scale = (float)(int)s;

    texshader = glee_shader_load("assets/shaders/texvert.frag", "assets/shaders/texfrag.frag");
    glUniform3f(glGetUniformLocation(texshader, "resolution"), width, height, scale);
    glUniform4f(glGetUniformLocation(texshader, "camera"), 0.0f, 0.0f, 1.0f, 0.0f);

    fontshader = glee_shader_load("assets/shaders/fontvert.frag", "assets/shaders/fontfrag.frag");
    glUniform3f(glGetUniformLocation(fontshader, "resolution"), width, height, scale);
    glUniform4f(glGetUniformLocation(fontshader, "camera"), 0.0f, 0.0f, 1.0f, 0.0f);

    lightshader = glee_shader_load("assets/shaders/lightv.frag", "assets/shaders/light.frag");
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

    //framebuffershader = shader_load("shaders/framebufferv.frag", "shaders/framebufferf.frag");
    skyboxshader = glee_shader_load("assets/shaders/skyboxvert.frag", "assets/shaders/skyboxfrag.frag");
}

int main()
{
    glee_init();
    glee_window_create("Space", 800, 600, 0, 0);
    glee_set_3d_mode();
    shaders_init_this();

    unsigned int quad = glee_buffer_quad_create();
    global_seed = rand_uint(rand_uint(time(NULL)));
    printf("Seed: %u\n", global_seed);

    unsigned int draw_kind = 4;
    float scale = 1.0f, rot = 0.0f;
    vec3 position = {0., 3., -5.};

    float v = 0.0f, h = 0.0f;
    const float mouse_speed = 0.005f;
    const float speed = 15.0f;

    vec4 background = {0.0f, 0.0f, 0.2f, 1.0f};
    texture_t tile = texture_load("assets/sprites/tile.png");
    font_t* font = font_load("assets/fonts/arcadeclassic.ttf", 24);

    float div = 32.0f;
    float last_time = glee_time_get();
    bmp_t bitmap = gradient_planet();
    for (int i = 0; i < 8; i++) {
        scale_gradient_bmp_irregular(&bitmap);
    }
    texture_t planet = texture_from_bmp(&bitmap);
    vmesh_t* mesh2 = vmesh_shape_sphere(8);
    vmesh_t* mesh = vmesh_shape_sphere(4);
    vmesh_scale(mesh, 12.0f);
    vmesh_scale(mesh2, 6.0f);
    vmesh_move(mesh2, vec3_new(34.0f, 0.0f, 0.0f));
    vmesh_combine(mesh, mesh2);
    vmesh_height_color_gradient(mesh);
    unsigned int id_planet = glee_buffer_id();
    mesh->type = OBJ_VTN;
    vmesh_bind(id_planet, mesh);
    printf("Planet mesh: %d, %d, %d\n", mesh->vertices->used, mesh->indices->used, id_planet);
    printf("Time: %f\n", glee_time_get() - last_time);

    last_time = glee_time_get();
    bitmap = gradient_bitmap();
    for (int i = 0; i < 4; i++) {
        scale_gradient_bitmap(&bitmap);
    }

    texture_t gradient = texture_from_bmp(&bitmap);
    vmesh_t* plane = vmesh_shape_plane(100, 100);
    perlin_vmesh(plane, div);
    vmesh_smooth_optim(&plane);
    vmesh_height_color_gradient(plane);
    unsigned int id_floor = glee_buffer_id();
    mesh->type = OBJ_VTN;
    vmesh_bind(id_floor, plane);
    printf("Plane mesh: %d, %d, %d\n", plane->vertices->used, plane->indices->used, id_floor);
    printf("Time: %f\n", glee_time_get() - last_time);

    bool flag = true;
    skybox_t* skybox = space_skybox(1080, 1080, 998, 1000, 2);
    mat4 mat_id = mat4_id();

    //framebuffer_t* framebuffer = framebuffer_new();
    unsigned int _w, _h;
    glee_window_get_size(&_w, &_h);
    float width = (float)(int)_w, height = (float)(int)_h;
    float aspect = width / height;
    float planet_rot = 0.0f;
    char delta_string[20] = "0.0000";
    int alarm = 20;

    glee_screen_color(background.x, background.y, background.z, background.w);
    while(glee_window_is_open()) {
        glee_screen_clear();
        //set_3d_mode();
        //framebuffer_bind(framebuffer);

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

        mat4 projection = mat4_perspective_RH(deg_to_rad(45.0f), aspect, 0.1f, 100.0f);
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
        if (glee_key_down(GLFW_KEY_P)) scale += delta_time;
        if (glee_key_down(GLFW_KEY_O)) scale -= delta_time;
        if (glee_key_down(GLFW_KEY_K)) rot += delta_time;
        if (glee_key_down(GLFW_KEY_L)) rot -= delta_time;
        if (glee_key_pressed(GLFW_KEY_M)) {
            div++;
            perlin_vmesh(plane, div);
        }
        if (glee_key_pressed(GLFW_KEY_N)) {
            div--;
            perlin_vmesh(plane, div);
        }
        if (glee_key_pressed(GLFW_KEY_SPACE)) {
            if (flag) {
                background = vec4_new(0.3f, 0.3f, 1.0f, 1.0f);
                flag = false;
            }
            else {
                background = vec4_new(0.0f, 0.0f, 0.1f, 1.0f);
                flag = true;
            }
        }
        if (glee_key_pressed(GLFW_KEY_V)) {
            if (draw_kind == 0x0004) draw_kind = 0x0001;
            else draw_kind = 0x0004;
        }
        if (glee_key_pressed(GLFW_KEY_R)) {
            global_seed = rand_uint(rand());
            perlin_vmesh(plane, div);
        }
        if (glee_key_pressed(GLFW_KEY_P)) {
            vmesh_write_file(mesh, "assets/models/vmesh.obj");
        }
        if (glee_key_pressed(GLFW_KEY_O)) {
            vmesh_write_file_quick(mesh, "assets/models/vmeshq.obj");
        }
        if (glee_key_pressed(GLFW_KEY_B)) {
            vmesh_smooth_sphere(&mesh);
        }

        if (flag) {
            mat4 mat = mat4_mult(projection, mat4_mult(mat4_look_at(vec3_uni(0.0f), direction, up), mat4_id()));
            glDepthMask(GL_FALSE);
            glUseProgram(skyboxshader);
            glBindVertexArray(skybox->VAO);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->cubemap.id);
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
        glDrawElements(draw_kind, mesh->indices->used, GL_UNSIGNED_INT, 0);

        glBindVertexArray(id_floor);
        glBindTexture(GL_TEXTURE_2D, gradient.id);
        glUniformMatrix4fv(glGetUniformLocation(lightshader, "model"), 1, GL_FALSE, &mat_id.data[0][0]);
        glDrawElements(draw_kind, plane->indices->used, GL_UNSIGNED_INT, 0);

        //glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //screen_clear(&background.r);
        //set_2d_mode();
        //glBindVertexArray(quad);
        //draw_framebuffer(&framebuffer->texture);
        //draw_texture(&planet, core.width / core.scale - 10, core.height / core.scale - 10, 1.0f, 0.0f);
        //draw_string("3Dpi", font, 10, core.height / core.scale - 48, 1.0f, 0.0f, 
        //            color(0.5f, 0.5f, 1.0f, 1.0f));
        //draw_string(delta_string, font, 10, core.height / core.scale - 128, 1.0f, 0.0f, color(0.5f, 0.5f, 1.0f, 1.0f));

        glee_screen_refresh();
    } 
    free(font);
    free(skybox);
    vmesh_free(mesh);
    vmesh_free(plane);
    glee_deinit();
    return 0;
}
