// main.cpp
#include "WheelSimulator.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    // Process input data
    if (argc != 4) {
        std::cerr << "Usage: ./WheelSimulator <slip> <sim_endtime> <batch_dir_name>" << std::endl;
        return EXIT_FAILURE;
    }

    // Import slip and batch directory from CLI arguments
    double slip = std::atof(argv[1]);
    double sim_endtime = std::atof(argv[2]);
    std::string batch_dir = argv[3];

    std::filesystem::path wheel_filepath = "/usr/local/share/chrono/data/robot/moonranger/obj/moonranger_wheel.obj";
    std::filesystem::path terrain_filepath = "/home/moonshot-chrono/sims/DEM-Engine/build_3/DemoOutput_GRCPrep_Part2/GRC_3e5_Reduced_Footprint.csv";

    try {
        WheelSimulator simulator(slip, sim_endtime, batch_dir, wheel_filepath, terrain_filepath);
        simulator.PrepareSimulation();
        simulator.RunSimulation();
    } catch (const std::exception& e) {
        std::cerr << "Simulation failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Simulation completed successfully." << std::endl;
    return EXIT_SUCCESS;
}
