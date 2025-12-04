#ifndef SIMPARAMS_H
#define SIMPARAMS_H

#include <filesystem>
#include <unordered_map>
#include <string>

struct SimParams {
    double slip;
    double sim_endtime;
    const std::string batch_dir;
    const std::filesystem::path data_drivepath;
    float rotational_velocity;
    float step_size;
    float angle_deg;
    float offset_x;
    float offset_y;
    float offset_z;
    float settling_time;

    // Constructor to initialize the wheel properties
    SimParams(double slip_val, double sim_et, const std::string& b_dir, 
            const std::filesystem::path& d_drivepath, float rot_vel, float stp_sz, float deg, float off_x, float off_y, float off_z, float set_t)
        :  slip(slip_val), sim_endtime(sim_et), batch_dir(b_dir), data_drivepath(d_drivepath), 
           rotational_velocity(rot_vel), step_size(stp_sz), angle_deg(deg), 
           offset_x(off_x), offset_y(off_y), offset_z(off_z), settling_time(set_t) {
    }
};

#endif // SIMPARAMS_H