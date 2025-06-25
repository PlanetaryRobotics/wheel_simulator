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

    std::filesystem::path wheel_directory = job_json.value("wheel_folder_path");
    std::filesystem::path wheel_file = wheel_directory / "wheel.obj";
    std::filesystem::path terrain_filepath = job_json.value("terrain_filepath");
    std::filesystem::path data_drivepath =  job_json.value("data_drivepath", "/ocean/projects/mch240013p/matthies/");

    
    std::filesystem::path wheel_json_path = wheel_directory / "wheel_parameters.json";
    std::ifstream file(wheel_json_path);
    if (!file) {
        std::cerr << "Could not open " << wheel_json_path << "\n";
        return 1;
    }

    json wheel_json;
    file >> wheel_json;

    // read wheel json parameters
    float width = wheel_json.value("width");
    float rim_radius = wheel_json.value("rim_radius"); //rim_radius is effective radius
    float outer_radius = wheel_json.value("outer_radius");

    try {
        WheelSimulator simulator(outer_radius, rim_radius, width, grouser_num, slip, sim_endtime, 
                    batch_dir, output_dir, wheel_filepath, terrain_filepath, data_drivepath, job_json);
        simulator.PrepareSimulation();
        simulator.RunSimulation();
    } catch (const std::exception& e) {
        std::cerr << "Simulation failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Simulation completed successfully." << std::endl;
    return EXIT_SUCCESS;
}
