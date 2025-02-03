// Utils.h
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace Utils {
    /**
     * @brief Generates the current timestamp in the format YYYY-MM-DD_HH-MM_.
     * 
     * @return std::string The formatted timestamp.
     */
    inline std::string getCurrentTimeStamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d_%H-%M_");
        return ss.str();
    }

    /**
     * @brief Converts a floating-point number to a string with precision.
     * 
     * @param value The floating-point number.
     * @param precision Number of decimal places.
     * @return std::string The formatted string.
     */
    inline std::string toStringWithPrecision(double value, int precision = 6) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(precision) << value;
        return out.str();
    }
}

#endif // UTILS_H
