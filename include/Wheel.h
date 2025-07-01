#ifndef WHEEL_H
#define WHEEL_H

#include <filesystem>
#include <unordered_map>
#include <string>

struct Wheel {
    float r_effective;
    float r_outer;
    float width;
    float mass;
    float total_mass;
    float IXX;  // Moment of inertia about X-axis
    float IYY;  // Moment of inertia about Y-axis
    float IZZ;  // Moment of inertia about Z-axis

    std::filesystem::path mesh_file_path;

    std::unordered_map<std::string, float> material_properties;

    // Constructor to initialize the wheel properties
    Wheel(float r_o, float r_e, float w, float m, 
        const std::filesystem::path& mesh_path, float t_m)
        :  r_outer(r_o), r_effective(r_e), width(w), 
        mass(m), mesh_file_path(mesh_path), total_mass(t_m) {
        // Calc moments of inertia based on wheel dimensions
        IYY = mass * r_outer * r_outer / 2.0f;
        IXX = (mass / 12.0f) * (3.0f * r_outer * r_outer + width * width);
        IZZ = IXX;
        
        // Default material properties
        material_properties = {
            {"E", 1e9},       // Young's modulus
            {"nu", 0.3},      // Poisson's ratio
            {"CoR", 0.3},     // Coefficient of restitution
            {"mu", 0.5},      // Friction coefficient
            {"Crr", 0.00}     // Rolling resistance coefficient
        };
    }
};

#endif // WHEEL_H