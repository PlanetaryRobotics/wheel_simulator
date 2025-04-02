// main.cpp
#include "WheelSimulator.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    // Process input data
    if (argc != 7) {
        std::cerr << "Usage: ./WheelSimulator <slip> <sim_endtime> <batch_dir_name> <wheel_path> <terrain_path> <data_path>" << std::endl;
        return EXIT_FAILURE;
    }

    // Import slip and batch directory from CLI arguments
    double slip = std::atof(argv[1]);
    double sim_endtime = std::atof(argv[2]);
    std::string batch_dir = argv[3];

    std::filesystem::path wheel_filepath = argv[4];
    std::filesystem::path terrain_filepath = argv[5];
    std::filesystem::path data_drivepath = argv[6];

    try {
        WheelSimulator simulator(slip, sim_endtime, batch_dir, wheel_filepath, terrain_filepath, data_drivepath);
        simulator.PrepareSimulation();
        simulator.RunSimulation();
    } catch (const std::exception& e) {
        std::cerr << "Simulation failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Simulation completed successfully." << std::endl;
    return EXIT_SUCCESS;
}
