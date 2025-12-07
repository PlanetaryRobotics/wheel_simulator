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
#include "Terrain.h"
#include "SimParams.h"
#include "json.hpp"
using json = nlohmann::json;

class WheelSimulator {
public:
    /**
     * @brief Constructs the WheelSimulator with the given simulation parameters.
     * 
     * @param wheel      Wheel geometry, mass/inertia, mesh path, and material properties.
     * @param terrain    Terrain mesh path and bulk terrain properties (world size, density, volumes, and moments of inertia).
     * @param simparams  High-level simulation parameters (slip, end time, batch/output directories, data path, step size, scale factor, angle, etc.).
     * @param param      JSON configuration object containing input/output paths, simulation end time, and other run-time settings.
     */
    WheelSimulator( Wheel wheel, Terrain terrain, 
                    SimParams simparams, const json param
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
    json param_;
    
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
    // float step_size_;
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
    //Sim Parameters
    SimParams simparams_; //Uses Simparams structure form SimParams.h
    
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