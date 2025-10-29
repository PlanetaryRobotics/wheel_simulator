#ifndef WHEEL_H
#define WHEEL_H

#include <filesystem>
#include <unordered_map>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

struct WheelParams {
    float outer_radius_m     = 0.0;
    float effective_radius_m = 0.0;
    float width_m            = 0.0;
    float mass_kg            = 0.0;
    float total_mass_kg          = 0.0;
    float angular_velocity_rad_s = 0.0;
};

inline WheelParams load_wheelparams (const std::filesystem::path& wheel_json) {
    //open json
    std::ifstream in(wheel_json);
    if (!in) {
        throw std::runtime_error("Cannot open wheel param file: " + wheel_json.string());
    }
    nlohmann::json j;
    in >> j;

    WheelParams wheelparams;
    auto get_param = [&](const char* k) -> float {
        if (!j.contains(k) || !j[k].is_number()) {
            throw std::runtime_error(std::string("Missing/invalid '") + k + "' in " + wheel_json.string());
        }
        return j[k].get<float>();
    };
    wheelparams.outer_radius_m = get_param("outer_radius_m");
    wheelparams.effective_radius_m = get_param("effective_radius_m");
    wheelparams.width_m = get_param("width_m");
    wheelparams.mass_kg = get_param("mass_kg");
    wheelparams.total_mass_kg = get_param("total_mass_kg");
    wheelparams.angular_velocity_rad_s = get_param("angular_velocity_rad_s");

    return wheelparams;
}

struct Wheel {
    float r_effective;
    float r_outer;
    float width;
    float mass;
    float IXX;  // Moment of inertia about X-axis
    float IYY;  // Moment of inertia about Y-axis
    float IZZ;  // Moment of inertia about Z-axis

    std::filesystem::path mesh_file_path;

    std::unordered_map<std::string, float> material_properties;

    // Constructor to initialize the wheel properties
    Wheel(const WheelParams& wp, const std::filesystem::path& mesh_path)
        :  r_outer(wp.outer_radius_m), r_effective(wp.effective_radius_m), 
           width(wp.width_m), mass(wp.mass_kg), mesh_file_path(mesh_path) {
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

inline Wheel makeWheel(const std::filesystem::path& wheel_json,
                       const std::filesystem::path& mesh_path) {
    const WheelParams wp = load_wheelparams(wheel_json);
    return Wheel(wp, mesh_path);
}

#endif // WHEEL_H