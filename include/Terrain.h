#ifndef TERRAIN_H
#define TERRAIN_H

#include <filesystem>
#include <unordered_map>
#include <string>

struct Terrain {
    std::filesystem::path terrain_filepath;
    double world_size_x;
    double world_size_y;
    double world_size_z;
    float world_bottom;
    float terrain_density;
    float volume1;
    float volume2;
    std::vector<float> scales;
    float3 MOI1;
    float3 MOI2;

    std::unordered_map<std::string, float> material_properties;

    // Constructor to initialize the wheel properties
    Terrain(const std::filesystem::path& terrain_path, double w_x, double w_y, double w_z, 
            float w_b, float t_dens, float v_1, float v_2, std::vector<float> terrain_scales, float scale_factor, float3 MOI_1, float3 MOI_2, 
            std::unordered_map<std::string, float> mat_prop)
        :  terrain_filepath(terrain_path), 
           world_size_x(w_x), world_size_y(w_y), world_size_z(w_z), world_bottom(w_b), 
           terrain_density(t_dens), volume1(v_1), volume2(v_2), scales(std::move(terrain_scales)), 
           MOI1(MOI_1), MOI2(MOI_2),
           material_properties(mat_prop)
    {
        for (auto& s : scales) {
            s *= scale_factor;
        }
    }
};

#endif // TERRAIN_H