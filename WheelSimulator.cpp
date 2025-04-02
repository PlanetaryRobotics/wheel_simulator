// WheelSimulator.cpp
#include "WheelSimulator.h"
#include "Utils.h"
#include "Constants.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace deme;

WheelSimulator::WheelSimulator(double slip, double sim_endtime, const std::string& batch_dir,
                    const std::filesystem::path& wheel_filepath,
                    const std::filesystem::path& terrain_filepath,
                    const std::filesystem::path& data_drivepath,
                    double scale_factor)
    : slip_(slip),
      sim_endtime_(sim_endtime),
      terrain_filepath_(terrain_filepath),
      batch_dir_(batch_dir),
      data_dir_(data_drivepath),
      step_size_(Constants::INITIAL_STEP_SIZE),
      fps_(Constants::FPS),
      out_steps_(static_cast<unsigned int>(1.0 / (Constants::FPS * Constants::INITIAL_STEP_SIZE))),
      report_steps_(static_cast<unsigned int>(1.0 / (Constants::REPORT_PERTIMESTEP * Constants::INITIAL_STEP_SIZE))),
      curr_step_(0),
      currframe_(0),
      frame_time_(1.0 / Constants::FPS),
      total_pressure_(0.0f),
      added_pressure_(0.0f),
      scale_factor_(scale_factor),
      mat_type_terrain_(DEMSim_.LoadMaterial({
                        {"E", 1e9},
                        {"nu", 0.3},
                        {"CoR", 0.3},
                        {"mu", 0.5},
                        {"Crr", 0.00}
                    })),
      wheel_(0.085f, 0.06f, 0.238f, wheel_filepath) // initializes wheel radius, width, and mass. TODO: load in from a wheel file, instead of hardcoded
{
    // Constructor body. Can remain empty or initialize additional members if necessary
}

void WheelSimulator::PrepareSimulation() {
    std::cout << "Intializing Output Directories" <<std::endl;
    InitializeOutputDirectories();
    std::cout << "Writing Sim Parameters" <<std::endl;
    WriteSimulationParameters();
    std::cout << "Intializing Output Files" <<std::endl;
    InitializeOutputFiles();
    std::cout << "Configuring DEM Solver" <<std::endl;
    ConfigureDEMSolver();
    std::cout << "Preparing Particles" <<std::endl;
    PrepareParticles();
    std::cout << "Configuring Wheel" <<std::endl;
    ConfigureWheel();
    std::cout << "Setting up prescribed motions" <<std::endl;
    SetupPrescribedMotions();
    std::cout << "Setting up inspectors" <<std::endl;
    SetupInspectors();
    std::cout << "Initializing DEMsim..." << std::endl;
    DEMSim_.Initialize();
}

void WheelSimulator::RunSimulation() {
    std::cout << "Letting wheel sink..." << std::endl;
    PerformInitialSink();
    std::cout << "Applying Wheel Forward Motion..." << std::endl;
    ApplyWheelForwardMotion();
    std::cout << "Running main sim loop..." << std::endl;
    RunSimulationLoop();
}

// Create the output folder structure
void WheelSimulator::InitializeOutputDirectories() {
    out_dir_ = data_dir_ / batch_dir_ / (Utils::getCurrentTimeStamp() + "_SkidSteerSim_" + std::to_string(slip_));
    rover_dir_ = out_dir_ / "rover";
    particles_dir_ = out_dir_ / "particles";

    std::error_code ec;
    if (!std::filesystem::create_directories(rover_dir_, ec) ||
        !std::filesystem::create_directories(particles_dir_, ec)) {
        throw std::runtime_error("Failed to create output directories.");
    }

    std::cout << "Output directory: " << out_dir_ << std::endl;
}

// Write params.json
void WheelSimulator::WriteSimulationParameters() {
    std::filesystem::path output_params_path = out_dir_ / "params.json";
    output_params_.open(output_params_path);
    if (!output_params_) {
        throw std::runtime_error("Failed to open params.json for writing.");
    }
    // TODO: Add the rest of the output params. Right now we are just recording the slip.
    output_params_ << "{\n"
                   << "\t\"slip\": " 
                   << slip_ 
                   << "\n}";
    output_params_.close();
}

// Initialize output.csv
void WheelSimulator::InitializeOutputFiles() {
    std::filesystem::path output_datafile_path = out_dir_ / "output.csv";
    output_datafile_.open(output_datafile_path);
    if (!output_datafile_) {
        throw std::runtime_error("Failed to open output.csv for writing.");
    }
    output_datafile_ << "t,f_x,f_y,f_z,d_c,v_max,pos_x,pos_y,pos_z,oriq_x,oriq_y,oriq_z,oriq_w,vel_x,vel_y,vel_z" << std::endl;
}

// Configure DEM Solver settings
void WheelSimulator::ConfigureDEMSolver() {
    DEMSim_.SetVerbosity(INFO);
    DEMSim_.SetOutputFormat(OUTPUT_FORMAT::CSV);
    DEMSim_.SetOutputContent(OUTPUT_CONTENT::ABSV);
    DEMSim_.SetOutputContent(OUTPUT_CONTENT::VEL);
    DEMSim_.SetOutputContent(OUTPUT_CONTENT::ABS_ACC);
    DEMSim_.SetOutputContent(OUTPUT_CONTENT::FAMILY);
    DEMSim_.SetMeshOutputFormat(MESH_FORMAT::VTK);
    DEMSim_.SetContactOutputContent({"OWNER", "FORCE", "POINT"});

    // Family Settings
    DEMSim_.SetFamilyFixed(Family::FIXED);
    DEMSim_.DisableContactBetweenFamilies(Family::FIXED, Family::FIXED);

    // Force Recording
    DEMSim_.SetNoForceRecord();

    // World Settings
    DEMSim_.SetInitTimeStep(step_size_);
    DEMSim_.SetGravitationalAcceleration(make_float3(0.0f, 0.0f, -Constants::GRAVITY_MAGNITUDE));
    DEMSim_.SetMaxVelocity(Constants::MAX_VELOCITY);
    DEMSim_.SetErrorOutVelocity(Constants::ERROR_OUT_VELOCITY);
    DEMSim_.SetExpandSafetyMultiplier(Constants::EXPAND_SAFETY_MULTIPLIER);
    DEMSim_.SetCDUpdateFreq(Constants::CD_UPDATE_FREQ);
}

// Load in the particles
void WheelSimulator::PrepareParticles() {

    // Prepare Terrain Particles
    // Load clump types and properties
    DEMSim_.SetMaterialPropertyPair("mu", DEMSim_.LoadMaterial(wheel_.material_properties), mat_type_terrain_, 0.8);

    std::cout << "Defining World..." << std::endl;

    // Define world dimensions
    double world_size_x = 1.0;
    double world_size_y = 0.3;
    double world_size_z = 2.0;
    DEMSim_.InstructBoxDomainDimension(world_size_x, world_size_y, world_size_z);
    DEMSim_.InstructBoxDomainBoundingBC("top_open", mat_type_terrain_);

    // Add boundary planes
    float bottom = -0.5f;
    DEMSim_.AddBCPlane(make_float3(0.0f, 0.0f, bottom), make_float3(0.0f, 0.0f, 1.0f), mat_type_terrain_);
    DEMSim_.AddBCPlane(make_float3(0.0f, static_cast<float>(world_size_y) / 2.0f, 0.0f), make_float3(0.0f, -1.0f, 0.0f), mat_type_terrain_);
    DEMSim_.AddBCPlane(make_float3(0.0f, -static_cast<float>(world_size_y) / 2.0f, 0.0f), make_float3(0.0f, 1.0f, 0.0f), mat_type_terrain_);
    DEMSim_.AddBCPlane(make_float3(-static_cast<float>(world_size_x) / 2.0f, 0.0f, 0.0f), make_float3(1.0f, 0.0f, 0.0f), mat_type_terrain_);
    DEMSim_.AddBCPlane(make_float3(static_cast<float>(world_size_x) / 2.0f, 0.0f, 0.0f), make_float3(-1.0f, 0.0f, 0.0f), mat_type_terrain_);

    // Define terrain particle templates
    float terrain_density = 2.6e3f;
    float volume1 = 4.2520508f;
    float mass1 = terrain_density * volume1;
    float3 MOI1 = make_float3(1.6850426f, 1.6375114f, 2.1187753f) * terrain_density;
    float volume2 = 2.1670011f;
    float mass2 = terrain_density * volume2;
    float3 MOI2 = make_float3(0.57402126f, 0.60616378f, 0.92890173f) * terrain_density;


    // Scale factors
    std::vector<double> scales = {0.0014, 0.00075833, 0.00044, 0.0003, 0.0002, 0.00018333, 0.00017};
    for (auto& scale : scales) {
        scale *= scale_factor_*10.0;
    }

    std::cout << "Loading clump templates..." << std::endl;


    // Load clump templates
    std::shared_ptr<DEMClumpTemplate> my_template2 = DEMSim_.LoadClumpType(mass2, MOI2, GetDEMEDataFile("clumps/triangular_flat_6comp.csv"), mat_type_terrain_);
    std::shared_ptr<DEMClumpTemplate> my_template1 = DEMSim_.LoadClumpType(mass1, MOI1, GetDEMEDataFile("clumps/triangular_flat.csv"), mat_type_terrain_);
    std::vector<std::shared_ptr<DEMClumpTemplate>> ground_particle_templates = {
        my_template2,
        DEMSim_.Duplicate(my_template2),
        my_template1,
        DEMSim_.Duplicate(my_template1),
        DEMSim_.Duplicate(my_template1),
        DEMSim_.Duplicate(my_template1),
        DEMSim_.Duplicate(my_template1)
    };

    // Scale and name templates
    for (size_t i = 0; i < scales.size(); ++i) {
        auto& tmpl = ground_particle_templates.at(i);
        tmpl->Scale(scales.at(i));

        char t_name[20];
        std::sprintf(t_name, "%04zu", i);
        tmpl->AssignName(std::string(t_name));
    }

    // Load clump locations from file
    std::cout << "Making terrain..." << std::endl;
    std::unordered_map<std::string, std::vector<float3>> clump_xyz;
    std::unordered_map<std::string, std::vector<float4>> clump_quaternion;

    try {
        clump_xyz = DEMSim_.ReadClumpXyzFromCsv(terrain_filepath_);
        clump_quaternion = DEMSim_.ReadClumpQuatFromCsv(terrain_filepath_);
    } catch (...) {
        throw std::runtime_error("Failed to read clump checkpoint file. Ensure the file exists and is correctly formatted.");
    }

    std::vector<float3> in_xyz;
    std::vector<float4> in_quat;
    std::vector<std::shared_ptr<DEMClumpTemplate>> in_types;
    unsigned int t_num = 0;

    for (const auto& scale : scales) {
        char t_name[20];
        std::sprintf(t_name, "%04u", t_num);

        auto this_type_xyz = clump_xyz[std::string(t_name)];
        auto this_type_quat = clump_quaternion[std::string(t_name)];

        size_t n_clump_this_type = this_type_xyz.size();
        std::cout << "Loading clump " << std::string(t_name) << " with " << n_clump_this_type << " particles." << std::endl;

        std::vector<std::shared_ptr<DEMClumpTemplate>> this_type(n_clump_this_type, ground_particle_templates.at(t_num));

        in_xyz.insert(in_xyz.end(), this_type_xyz.begin(), this_type_xyz.end());
        in_quat.insert(in_quat.end(), this_type_quat.begin(), this_type_quat.end());
        in_types.insert(in_types.end(), this_type.begin(), this_type.end());

        std::cout << "Added clump type " << t_num << std::endl;
        t_num++;
    }

    // Create and add clump batch
    DEMClumpBatch base_batch(in_xyz.size());
    base_batch.SetTypes(in_types);
    base_batch.SetPos(in_xyz);
    base_batch.SetOriQ(in_quat);

    DEMSim_.AddClumps(base_batch);
}

// Load in the wheel
void WheelSimulator::ConfigureWheel() {
    // Define simulation parameters
    // TODO: Change this so it isn't hardcoded
    float total_mass = 4.5f; // kg
    total_pressure_ = total_mass * Constants::GRAVITY_MAGNITUDE; // N
    added_pressure_ = (total_mass - wheel_.mass) * Constants::GRAVITY_MAGNITUDE; // N

    std::cout << "Total Pressure: " << total_pressure_ << "N" << std::endl;
    std::cout << "Added Pressure: " << added_pressure_ << "N" << std::endl;

    // Add wheel object
    auto wheel = DEMSim_.AddWavefrontMeshObject(wheel_.mesh_file_path, DEMSim_.LoadMaterial(wheel_.material_properties));
    wheel->SetMass(wheel_.mass);
    wheel->SetMOI(make_float3(wheel_.IXX, wheel_.IYY, wheel_.IZZ));
    wheel->SetFamily(Family::ROTATING);
    wheel_tracker_ = DEMSim_.Track(wheel);
}

void WheelSimulator::SetupPrescribedMotions() {
    // Families' prescribed motions
    float w_r = 0.2f;  // TODO: Change this so it isn't hardcoded
    float v_ref = w_r * wheel_.radius;

    //TODO: Turn family numbers into enums with descriptive names

    DEMSim_.SetFamilyPrescribedAngVel(Family::ROTATING, "0", Utils::toStringWithPrecision(w_r), "0", false);
    DEMSim_.AddFamilyPrescribedAcc(Family::ROTATING, "none", "none", Utils::toStringWithPrecision(-added_pressure_ / 0.238f)); // TODO: What does this number mean?

    DEMSim_.SetFamilyPrescribedAngVel(Family::ROTATING_AND_TRANSLATING, "0", Utils::toStringWithPrecision(w_r), "0", false);
    DEMSim_.SetFamilyPrescribedLinVel(Family::ROTATING_AND_TRANSLATING, Utils::toStringWithPrecision(v_ref * (1.0 - slip_)), "0", "none", false);
    DEMSim_.AddFamilyPrescribedAcc(Family::ROTATING_AND_TRANSLATING, "none", "none", Utils::toStringWithPrecision(-added_pressure_ / 0.238f)); // TODO: What does this number mean?
}

void WheelSimulator::SetupInspectors() {
    // Setup inspectors. These let us track and query data of objects.
    max_z_finder_ = DEMSim_.CreateInspector("clump_max_z");
    min_z_finder_ = DEMSim_.CreateInspector("clump_min_z");
    total_mass_finder_ = DEMSim_.CreateInspector("clump_mass");
    max_v_finder_ = DEMSim_.CreateInspector("clump_max_absv");
}

void WheelSimulator::WriteParticleCSV() {
    char filename[200];
    sprintf(filename, "%s/DEMdemo_output_%04d.csv", particles_dir_.c_str(), currframe_);
    DEMSim_.WriteSphereFile(std::string(filename));
}

void WheelSimulator::WriteWheelMesh() {
    char meshname[200];
    sprintf(meshname, "%s/DEMdemo_mesh_%04d.vtk", rover_dir_.c_str(), currframe_);
    DEMSim_.WriteMeshFile(meshname);
}

void WheelSimulator::PerformInitialSink() {
    // Put the wheel in place, then let the wheel sink in initially
    std::cout << "Setting max z value" << std::endl;
    float max_z = max_z_finder_->GetValue();
    std::cout << "Setting wheel position" << std::endl;
    if (wheel_tracker_) {
        wheel_tracker_->SetPos(make_float3(-0.25, 0, max_z + 0.01 + wheel_.radius));
    } else {
        std::cerr << "Error: wheel_tracker_ is null!" << std::endl;
    }
    std::cout << "Starting wheel settling loop" << std::endl;
    for (double t = 0; t < 0.5; t += frame_time_) {
        std::cout << "Outputting frame: " << currframe_ << std::endl;
        std::cout << "Writing sphere" << std::endl;
        WriteParticleCSV();
        std::cout << "Writing mesh" << std::endl;
        WriteWheelMesh();
        std::cout << "Doing dynamics" << std::endl;
        DEMSim_.DoDynamicsThenSync(frame_time_);
        currframe_++;
    }
}

void WheelSimulator::ApplyWheelForwardMotion() {
    // Switch wheel from free fall into DP test. Tell it to start driving forward.
    DEMSim_.DoDynamicsThenSync(0);
    DEMSim_.ChangeFamily(Family::ROTATING, Family::ROTATING_AND_TRANSLATING);
}

void WheelSimulator::UpdateActiveBoxDomain(float box_halfsize_x, float box_halfsize_y) {
    // The active box domain is a trick to improve performance. 
    // Essentially, we only update the position of particles near the wheel, and freeze all particles outside the box
    // This likely causes some simualtion inaccuracies. We should perform a sensitivity study to see if this is the case.
    DEMSim_.DoDynamicsThenSync(0.0f);
    DEMSim_.ChangeClumpFamily(Family::FIXED);

    size_t num_changed = 0;
    // Retrieve wheel position
    float x = wheel_tracker_->Pos().x;
    float y = wheel_tracker_->Pos().y;

    std::pair<float, float> Xrange = {x - box_halfsize_x, x + box_halfsize_x};
    std::pair<float, float> Yrange = {y - box_halfsize_y, y + box_halfsize_y};
    num_changed += DEMSim_.ChangeClumpFamily(Family::FREE, Xrange, Yrange);

    std::cout << num_changed << " particles changed family number." << std::endl;
}

void WheelSimulator::WriteFrameData(double t, float3 forces) {
    // Write a new row of summary data to output.csv.
    try {
        output_datafile_<< t << "," 
                        << forces.x << "," 
                        << forces.y << "," 
                        << forces.z << ","
                        << forces.x/total_pressure_ << "," 
                        << max_v_finder_->GetValue() << ","
                        << wheel_tracker_->Pos().x << ","
                        << wheel_tracker_->Pos().y << ","
                        << wheel_tracker_->Pos().z << ","
                        << wheel_tracker_->OriQ().x << ","
                        << wheel_tracker_->OriQ().y << ","
                        << wheel_tracker_->OriQ().z << ","
                        << wheel_tracker_->OriQ().w << ","
                        << wheel_tracker_->Vel().x << ","
                        << wheel_tracker_->Vel().y << ","
                        << wheel_tracker_->Vel().z
                        << std::endl;
        output_datafile_.flush();
    } catch (...) {
        throw std::runtime_error("Unable to write content to output file.");
    }
}

void WheelSimulator::RunSimulationLoop() {
    std::cout << "Output at " << Constants::FPS << " FPS" << std::endl;
    // The main sim loop currently runs at twice the step size as the settling phase
    // TODO: This is likely confusing; we should split these into two parameters, e.g. step_size_settle, step_size_drive
    step_size_ *= 2.;
    DEMSim_.UpdateStepSize(step_size_);

    // Start simulation timer
    auto start = std::chrono::high_resolution_clock::now();

    // Active box domain parameters
    // TODO: This should be based on the wheel size, not hardcoded
    float box_halfsize_x = 0.085f * 1.25f;
    float box_halfsize_y = 0.06f * 2.0f;

    for (double t = 0.0; t < sim_endtime_; t += step_size_, curr_step_++) {
        if (curr_step_ % out_steps_ == 0) {
            UpdateActiveBoxDomain(box_halfsize_x, box_halfsize_y);

            // Write output files
            WriteWheelMesh();
            WriteParticleCSV();

            std::cout << "Outputting frame: " << currframe_ << std::endl;
            currframe_++;
            DEMSim_.ShowThreadCollaborationStats();
        }

        if (curr_step_ % report_steps_ == 0) {
            // Retrieve forces on the wheel
            float3 forces = wheel_tracker_->ContactAcc() * wheel_.mass;

            // Output to terminal
            std::cout << "Time: " << t << std::endl;
            std::cout << "Force on wheel: " << forces.x << ", " << forces.y << ", " << forces.z << std::endl;
            std::cout << "Drawbar pull coeff: " << (forces.x / total_pressure_) << std::endl;

            WriteFrameData(t, forces);

            // Termination condition
            // if (wheel_tracker_->Pos().x > (world_size_x / 2.0f - wheel_radius * 1.2f)) {
            //     std::cout << "This is far enough, stopping the simulation..." << std::endl;
            //     DEMSim_.DoDynamicsThenSync(0.0f);
            //     break;
            // }
        }

        DEMSim_.DoDynamics(step_size_);
    }

    // End simulation timer
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_sec = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    std::cout << time_sec.count() << " seconds (wall time) to finish the simulation." << std::endl;

    // Show stats
    DEMSim_.ShowTimingStats();
    DEMSim_.ShowAnomalies();

    std::cout << "WheelSimulator demo exiting..." << std::endl;
}
