#ifndef SIMPARAMS_H
#define SIMPARAMS_H

#include <filesystem>
#include <unordered_map>
#include <string>

// double slip, double sim_endtime, 
//                     const std::string& batch_dir,
//                     const std::string& output_dir,
//                     const std::filesystem::path& data_drivepath,
//                     const json param, float rotational_velocity,
//                     float step_size, float scale_factor
struct SimParams {
    double slip;
    double sim_endtime;
    const std::string& batch_dir;
    const std::string& output_dir;
    const std::filesystem::path& data_drivepath;
    float rotational_velocity;
    float step_size;
    float scale_factor;

    // Constructor to initialize the wheel properties
    SimParams(double slip_val, double sim_et, const std::string& b_dir, const std::string& o_dir, 
            const std::filesystem::path& d_drivepath, float rot_vel, float stp_sz, float scale_fac)
        :  slip(slip_val), sim_endtime(sim_et), batch_dir(b_dir), output_dir(o_dir), data_drivepath(d_drivepath), 
           rotational_velocity(rot_vel), step_size(stp_sz), scale_factor(scale_fac) {
    }
};

#endif // SIMPARAMS_H