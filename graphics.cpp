#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./stb/stb_image_write.h"

struct Sphere 
{
    Vec3f center;
    float radius;

    Sphere(const Vec3f &c, const float &r) : center(c), radius(r) {}

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

Vec3f cast_ray(const Vec3f &orig, const Vec3f &dir, const Sphere &sphere) 
{
    float sphere_dist = std::numeric_limits<float>::max();
    if (!sphere.ray_intersect(orig, dir, sphere_dist)) 
        return Vec3f(0.2, 0.7, 0.8); // background color

    return Vec3f(0.4, 0.4, 0.3);
}

void render(const Sphere &sphere) {
    const int width    = 1024;
    const int height   = 768;
    const float fov      = M_PI/2.;//исправил на float
    std::vector<Vec3f> framebuffer(width*height);

    #pragma omp parallel for
    for (size_t j = 0; j<height; j++) {
        for (size_t i = 0; i<width; i++) {
            float x =  (2*(i + 0.5)/(float)width  - 1)*tan(fov/2.)*width/(float)height;
            float y = -(2*(j + 0.5)/(float)height - 1)*tan(fov/2.);//экран помещен на расстояние 1 от начала! типа там фокус что ли. смотри wiki в REadme.
            Vec3f dir = Vec3f(x, y, -1).normalize();
            std::cout<<Vec3f(x, y, -1)<< fov/2.<<std::endl ;
            framebuffer[i+j*width] = cast_ray(Vec3f(0,0,0), dir, sphere);
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
    Sphere sphere(Vec3f(0, 1, 3), 3.17);
    render(sphere);

    return 0;
}

