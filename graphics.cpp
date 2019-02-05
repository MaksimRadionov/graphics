#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./stb/stb_image_write.h"

struct Material 
{
    Material(const Vec3f &color) : diffuse_color(color) {}
    Material() : diffuse_color() {}
    Vec3f diffuse_color;
};

struct Sphere 
{
    Vec3f center;
    float radius;
    Material material;
    Sphere(const Vec3f &c, const float &r, const Material &m) : center(c), radius(r), material(m) {}

    bool ray_intersect(const Vec3f &orig, const Vec3f &dir, float &t0) const 
    {
        Vec3f or_centr_v = center - orig;// направление на центр сферы
        float tca = or_centr_v*dir;// проекция на dir
        float b = or_centr_v*or_centr_v - tca*tca;// от центра сферы то луча (прицельный параметр) в квадрате
        if (b > radius*radius) 
            return false;
        float thc = sqrtf(radius*radius - b);//половина хорды 
        t0       = tca - thc;//расстояние до окружности
        float t1 = tca + thc;//если внутри типа
        if (t0 < 0) 
            t0 = t1;
        if (t0 < 0) 
            return false;
        return true;
    }
};

bool scene_intersect(const Vec3f &orig, const Vec3f &dir, const std::vector<Sphere> &spheres, 
                     Vec3f &hit, Vec3f &N, Material &material) 
{
    float spheres_dist = std::numeric_limits<float>::max();
    for (size_t i=0; i < spheres.size(); i++) 
    {
        float dist_i{0};
        if (spheres[i].ray_intersect(orig, dir, dist_i) && dist_i < spheres_dist) 
        {
            spheres_dist = dist_i;
            hit = orig + dir*dist_i;
            N = (hit - spheres[i].center).normalize();
            material = spheres[i].material;
        }
    }
    return spheres_dist<1000;
}

Vec3f cast_ray(const Vec3f &orig, const Vec3f &dir, const std::vector<Sphere> &spheres) 
{
    Vec3f point, N;//точка пересечения с ближайшей сферой и нормаль к этой точке
    Material material;
    if (!scene_intersect(orig, dir, spheres, point, N, material))
        return Vec3f(0, 0, 0); // background color
    return material.diffuse_color;
}

void render(const std::vector<Sphere> &spheres) {
    const int width    = 1024;
    const int height   = 768;
    const float fov      = M_PI/2;//исправил на float
    std::vector<Vec3f> framebuffer(width*height);

    #pragma omp parallel for
    for (size_t j = 0; j<height; j++) {
        for (size_t i = 0; i<width; i++) {
            float x =  (2*(i + 0.5)/(float)width  - 1)*tan(fov/2.)*width/(float)height;
            float y = -(2*(j + 0.5)/(float)height - 1)*tan(fov/2.);//экран помещен на расстояние 1 от начала! типа там фокус что ли. смотри wiki в REadme.
            Vec3f dir = Vec3f(x, y, -1).normalize();
            framebuffer[i+j*width] = cast_ray(Vec3f(0,0,0), dir, spheres);
        }
    }


    std::ofstream ofs; // save the framebuffer to file
    ofs.open("./out.ppm");
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height*width; ++i) 
    {
        for (size_t j = 0; j<3; j++) 
        {
            ofs << (char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][j])));
        }
    }
    ofs.close();
//    stbi_write_jpg("xxx.jpg",width,height, 3, &framebuffer[0], 50);


}
int main() 
{
    Material      ivory(Vec3f(0., 1., 0.));
    Material red_rubber(Vec3f(0.3, 0.1, 0.1));

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(Vec3f(-3,    0,   -16), 2,      ivory));
    spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, red_rubber));
    spheres.push_back(Sphere(Vec3f( 1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(Vec3f( 7,    5,   -18), 4,      ivory));

    render(spheres);

    return 0;
}

