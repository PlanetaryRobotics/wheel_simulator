// Constants.h
#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace Constants {
    constexpr double PI = 3.14159265358979323846;
    constexpr float GRAVITY_MAGNITUDE = 9.81f;
    constexpr float INITIAL_STEP_SIZE = 1e-6f;
    constexpr float MAX_VELOCITY = 50.0f;
    constexpr float ERROR_OUT_VELOCITY = 60.0f;
    constexpr float EXPAND_SAFETY_MULTIPLIER = 1.1f;
    constexpr int CD_UPDATE_FREQ = 30;
    constexpr unsigned int FPS = 10;
    constexpr unsigned int REPORT_PERTIMESTEP = 1000;
}

#endif // CONSTANTS_H
