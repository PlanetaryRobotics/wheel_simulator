// main.cpp
#include "include/WheelSimulator.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include "include/json.hpp"
using json = nlohmann::json;


int main(int argc, char* argv[]) {
    // Process input data
    if (argc != 4) {
        // std::cerr << "Usage: ./WheelSimulator <slip> <sim_endtime> <batch_dir_name> <wheel_path> <terrain_path> <data_path>" << std::endl;
        std::cerr << "Usage: ./WheelSimulator <input_json_folder_path> <batch_name> <slip>" << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream file((argv[1] + "/job_parameters.json"));
    if (!file) {
        std::cerr << "Could not open " << argv[1] << "\n";
        return 1;
    }

    json job_json;
    file >> job_json;

    // Import slip and batch directory from CLI arguments
    double slip = std::atof(argv[3]);
    double sim_endtime = job_json.value("sim_endtime", 0.1);
    std::string batch_dir = argv[2];
    std::string output_dir = job_json.value("output_dir", "");
    float rotational_velocity = job_json.value("rotational_velocity", 0.2);
    float step_size = job_json.value("step_size", 1e-6);
    float scale_factor = job_json.value("scale_factor", 10);
    std::filesystem::path data_drivepath =  job_json.value("data_drivepath", "/ocean/projects/mch240013p/matthies/");
    float angle_deg = job_json.value("wheel_angle", 0.0);


    if(!job_json.contains("wheel_folder_path")){
        std::cerr << "Error: 'wheel_folder_path' is missing from the job_json\n";
        return 1;
    }
    std::filesystem::path wheel_directory = job_json["wheel_folder_path"];
    std::filesystem::path wheel_filepath = wheel_directory / "wheel.obj";

    if(!job_json.contains("terrain_filepath")){
        std::cerr << "Error: 'terrain_filepath' is missing from the job_json\n";
        return 1;
    }
    std::filesystem::path terrain_directory = job_json["terrain_filepath"];
    std::filesystem::path terrain_filepath = terrain_directory / "GRC_3e5_Reduced_Footprint.csv";

    
    std::filesystem::path wheel_json_path = argv[1] / "/wheel_parameters.json";
    std::ifstream file2(wheel_json_path);
    if (!file2) {
        std::cerr << "Could not open " << wheel_json_path << "\n";
        return 1;
    }

    json wheel_json;
    file2 >> wheel_json;

    // read wheel json parameters
    float width = wheel_json["width"];
    float rim_radius = wheel_json["rim_radius"]; //rim_radius is effective radius
    float outer_radius = wheel_json["outer_radius"];
    float mass = wheel_json.value("mass", 0.238);
    float total_mass = wheel_json.value("total_mass", 4.5);

    std::filesystem::path terrain_json_path = argv[1] / "terrain_parameters.json";
    std::ifstream file3(terrain_json_path);
    if (!file3) {
        std::cerr << "Could not open " << terrain_json_path << "\n";
        return 1;
    }
    json terrain_json;
    file3 >> terrain_json;

    //read terrain json parameters
    double world_size_x = terrain_json.value("world_size_x", 1);
    double world_size_y = terrain_json.value("world_size_y", 0.3);
    double world_size_z = terrain_json.value("world_size_z", 2);
    float world_bottom = terrain_json.value("world_bottom", -0.5);
    float terrain_density = terrain_json.value("terrain_density", 2.6e3);
    float volume1 = terrain_json.value("volume1", 4.2520508);
    float volume2 = terrain_json.value("volume2", 2.1670011);
    float3 MOI1;
    if(terrain_json.contains("MOI1") && terrain_json["MOI1"].is_array() && terrain_json["MOI1"].size() == 3){
        MOI1.x = terrain_json["MOI1"][0].get<float>();
        MOI1.y = terrain_json["MOI1"][1].get<float>();
        MOI1.z = terrain_json["MOI1"][2].get<float>();
    }
    else{
        MOI1 = make_float3(1.6850426f, 1.6375114f, 2.1187753f);
    }
    float3 MOI2;
    if(terrain_json.contains("MOI2") && terrain_json["MOI2"].is_array() && terrain_json["MOI2"].size() == 3){
        MOI2.x = terrain_json["MOI2"][0].get<float>();
        MOI2.y = terrain_json["MOI2"][1].get<float>();
        MOI2.z = terrain_json["MOI2"][2].get<float>();
    }
    else{
        MOI2 = make_float3(0.57402126f, 0.60616378f, 0.92890173f);
    }

    Wheel wheel(outer_radius, rim_radius, width, mass, wheel_filepath, total_mass);
    Terrain terrain(terrain_filepath, world_size_x, world_size_y, world_size_z, world_bottom,
                    terrain_density, volume1, volume2, MOI1, MOI2);
    SimParams simparams(slip, sim_endtime, batch_dir, output_dir, data_drivepath, 
                        rotational_velocity, step_size, scale_factor, angle_deg);

    try {
        WheelSimulator simulator(wheel, terrain, simparams, job_json);
        simulator.PrepareSimulation();
        simulator.RunSimulation();
    } catch (const std::exception& e) {
        std::cerr << "Simulation failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Simulation completed successfully." << std::endl;
    return EXIT_SUCCESS;
}
