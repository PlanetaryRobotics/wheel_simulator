// main.cpp
#include "include/WheelSimulator.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include "include/json.hpp"
using json = nlohmann::json;


int main(int argc, char* argv[]) {
    // Process input data
    if (argc != 3) {
        // std::cerr << "Usage: ./WheelSimulator <slip> <sim_endtime> <batch_dir_name> <wheel_path> <terrain_path> <data_path>" << std::endl;
        std::cerr << "Usage: ./WheelSimulator <input_path>" << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Could not open " << argv[1] << "\n";
        return 1;
    }

    json job_json;
    file >> job_json;

    // Import slip and batch directory from CLI arguments
    double slip = job_json.value("slip", 0.2);
    double sim_endtime = job_json.value("sim_endtime", 5);
    std::string batch_dir = argv[2];
    std::string output_dir = job_json.value("output_dir", "");
    float rotational_velocity = job_json.value("rotational_velocity", 0.2);
    float step_size = job_json.value("step_size", 1e-6);
    float scale_factor = job_json.value("scale_factor", 10);
    std::filesystem::path data_drivepath =  job_json.value("data_drivepath", "/ocean/projects/mch240013p/matthies/");


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

    
    std::filesystem::path wheel_json_path = wheel_directory / "wheel_parameters.json";
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
    float mass = 0.238; // TODO: read from file
    float total_mass = wheel_json.value("total_mass", 4.5);

    std::filesystem::path terrain_json_path = terrain_directory / "terrain_parameters.json";
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

    Wheel wheel(outer_radius, rim_radius, width, mass, wheel_filepath);
    Terrain terrain(terrain_filepath, world_size_x, world_size_y, world_size_z, world_bottom,
                    terrain_density, volume1, volume2);

    try {
        WheelSimulator simulator(wheel, terrain, slip, sim_endtime, 
                    batch_dir, output_dir, data_drivepath, 
                    job_json, rotational_velocity, step_size, scale_factor, total_mass);
        simulator.PrepareSimulation();
        simulator.RunSimulation();
    } catch (const std::exception& e) {
        std::cerr << "Simulation failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Simulation completed successfully." << std::endl;
    return EXIT_SUCCESS;
}
