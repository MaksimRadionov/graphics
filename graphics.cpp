#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./stb/stb_image_write.h"

struct Light 
{
    Light(const Vec3f &p, const float &i) : position(p), intensity(i) {}
    Vec3f position;
    float intensity;
};

struct Material 
{
    Material(const Vec2f &a, const Vec3f &color, const float &spec) : albedo(a), diffuse_color(color), specular_exponent(spec) {}
    Material() : albedo(1,0), diffuse_color(), specular_exponent() {}
    Vec2f albedo;
    Vec3f diffuse_color;
    float specular_exponent;
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

Vec3f reflect(const Vec3f &I, const Vec3f &N) 
{
    return I - N*2.f*(I*N);
}

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

Vec3f cast_ray(const Vec3f &orig, const Vec3f &dir, const std::vector<Sphere> &spheres, const std::vector<Light> &lights) 
{
    Vec3f point, N;//точка пересечения с ближайшей сферой и нормаль к этой точке
    Material material;
    if (!scene_intersect(orig, dir, spheres, point, N, material))
        return Vec3f(0.2, 0.7, 0.8); // background color
    
    float diffuse_light_intensity = 0, specular_light_intensity = 0;
    for (size_t i=0; i<lights.size(); i++) 
    {
        Vec3f light_dir = (lights[i].position - point).normalize();//L - направление на источник от поверхоолоности
        diffuse_light_intensity  += lights[i].intensity * std::max(0.f, light_dir*N);
        specular_light_intensity += powf(std::max(0.f, -reflect(-light_dir, N)*dir), material.specular_exponent)*lights[i].intensity;//reflect - отражение dir - направлению в камеру
    }
    return material.diffuse_color * diffuse_light_intensity * material.albedo[0] + Vec3f(1., 1., 1.)*specular_light_intensity * material.albedo[1];//Vec3f(1., 1., 1.) - типа цвет specular 

}

void render(const std::vector<Sphere> &spheres, const std::vector<Light> &lights,std::string file_name) 
{
    const int width    = 1024;
    const int height   = 768;
    const float fov      = M_PI/2;//исправил на float
    std::vector<Vec3f> framebuffer(width*height);

    #pragma omp parallel for
    for (size_t j = 0; j<height; j++) 
    {
        for (size_t i = 0; i<width; i++) 
        {
            float x =  (2*(i + 0.5)/(float)width  - 1)*tan(fov/2.)*width/(float)height;
            float y = -(2*(j + 0.5)/(float)height - 1)*tan(fov/2.);//экран помещен на расстояние 1 от начала! типа там фокус что ли. смотри wiki в REadme.
            Vec3f dir = Vec3f(x, y, -1).normalize();
            framebuffer[i+j*width] = cast_ray(Vec3f(0,0,0), dir, spheres, lights);
        }
    }


    std::ofstream ofs; // save the framebuffer to file
    ofs.open(file_name.c_str());
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height*width; ++i) 
    {
        Vec3f &c = framebuffer[i];                       //так понимаю что это нормировка на единицу
        float max = std::max(c[0], std::max(c[1], c[2]));//каждого цвета. Если какой то цвет превышает 1(интенсивность) то все 3 
        if (max>1)                                       //цвета перенормировывают
            c = c*(1./max);
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
    Material      ivory(Vec2f(0.6,  0.3), Vec3f(0.4, 0.4, 0.3),   50.);
    Material red_rubber(Vec2f(0.9,  0.1), Vec3f(0.3, 0.1, 0.1),   10.);

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(Vec3f(-3,    0,   -16), 2,      ivory));
    spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, red_rubber));
    spheres.push_back(Sphere(Vec3f( 1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(Vec3f( 7,    5,   -18), 4,      ivory));

    std::vector<Light>  lights;
    lights.push_back(Light(Vec3f(-20, 20,  20), 1.5));
    lights.push_back(Light(Vec3f( 30, 50, -25), 1.8));
    lights.push_back(Light(Vec3f( 30, 20,  30), 1.7));

    render(spheres, lights,"2");

    return 0;
  
}

