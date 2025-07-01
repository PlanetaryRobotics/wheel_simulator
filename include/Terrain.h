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

    // Constructor to initialize the wheel properties
    Terrain(const std::filesystem::path& terrain_path, double w_x, double w_y, double w_z, 
            float w_b, float t_dens, float v_1, float v_2)
        :  terrain_filepath(terrain_path), world_size_x(w_x), world_size_y(w_y), world_size_z(w_z), world_bottom(w_b), 
           terrain_density(t_dens), volume1(v_1), volume2(v_2) {
    }
};

#endif // TERRAIN_H