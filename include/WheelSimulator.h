// WheelSimulator.h
#ifndef WHEELSIMULATOR_H
#define WHEELSIMULATOR_H

#include <DEM/API.h>
#include <DEM/HostSideHelpers.hpp>

#include <memory>
#include <string>
#include <filesystem>
#include <vector>
#include "Wheel.h"
#include "json.hpp"
using json = nlohmann::json;

class WheelSimulator {
public:
    /**
     * @brief Constructs the WheelSimulator with the given simulation parameters.
     * 
     * @param slip Slip ratio for the simulation.
     * @param sim_endtime Simulation end time.
     * @param batch_dir Batch directory name for outputs.
     * @param wheel_filepath Path to the .obj file for the wheel
     * @param terrain_filepath Path to the .csv file containing the pre-settled terrain data.
     * @param data_drivepath Path to the data drive
     */
    WheelSimulator( Wheel wheel, Terrain terrain,
                    double slip, 
                    double sim_endtime, 
                    const std::string& batch_dir, 
                    const std::string& output_dir,
                    const std::filesystem::path& data_drivepath,
                    const json param, float rotational_velocity,
                    float step_size, float scale_factor
                );
    


    /**
     * @brief Prepares the simulation by initializing directories, writing parameters, and setting up the simulation environment.
     */
    void PrepareSimulation();

    /**
     * @brief Executes the simulation loop.
     */
    void RunSimulation();

private:
    // Simulation Parameters
    double slip_;
    double sim_endtime_;
    std::string batch_dir_;
    std::string output_dir_;
    json param_;
    float rot_velocity_;
    scale_factor_;
    
    
    std::filesystem::path terrain_filepath_;

    // File System Paths
    std::filesystem::path data_dir_;
    std::filesystem::path out_dir_;
    std::filesystem::path rover_dir_;
    std::filesystem::path particles_dir_;

    // DEM Simulation Components
    deme::DEMSolver DEMSim_;
    std::shared_ptr<deme::DEMMaterial> mat_type_terrain_;

    // Output Files
    std::ofstream output_params_;
    std::ofstream output_datafile_;

    // Simulation State
    float step_size_;
    unsigned int fps_;
    unsigned int out_steps_;
    unsigned int report_steps_;
    unsigned int curr_step_;
    unsigned int currframe_;
    double frame_time_;
    float total_pressure_;
    float added_pressure_;

    // Trackers
    std::shared_ptr<deme::DEMTracker> wheel_tracker_;
    
    // Terrain Inspectors
    std::shared_ptr<deme::DEMInspector> max_z_finder_;
    std::shared_ptr<deme::DEMInspector> min_z_finder_;
    std::shared_ptr<deme::DEMInspector> total_mass_finder_;
    std::shared_ptr<deme::DEMInspector> max_v_finder_;

    // Wheel
    Wheel wheel_;  // Uses wheel structure from Wheel.h
    // Terrain
    Terrain terrain_; //Uses terrain structure from Terrain.h
    // Different families. These are used by the DEM engine to group elements
    // Elements within a family can have their motion properties all set at once.
    // By default, all simulation elements have a family of 0.
    enum Family : int {
        FREE = 0,
        FIXED,
        ROTATING,
        ROTATING_AND_TRANSLATING
    };

    // Private Methods
    void InitializeOutputDirectories();
    void WriteSimulationParameters();
    void InitializeOutputFiles();
    void ConfigureDEMSolver();
    void ConfigureWheel();
    void PrepareParticles();
    void SetupPrescribedMotions();
    void SetupInspectors();

    void PerformInitialSink();
    void ApplyWheelForwardMotion();
    void UpdateActiveBoxDomain(float box_halfsize_x, float box_halfsize_y);
    void WriteWheelMesh();
    void WriteParticleCSV();
    void WriteFrameData(double t, float3 forces);
    void RunSimulationLoop();
};

#endif // WHEELSIMULATOR_H