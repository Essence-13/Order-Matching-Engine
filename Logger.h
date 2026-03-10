#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>

// A simple file logger class.
class Logger {
public:
    // Constructor opens the log file.
    Logger(const std::string& log_file);
    // Destructor closes the log file.
    ~Logger();

    // Logs a message with a given category (e.g., "System", "Error").
    void log(const std::string& category, const std::string& message);

private:
    std::ofstream eventLog; // The file stream for logging.

    // Gets the current system time as a timestamp.
    time_t getCurrentTimestamp() const;
};

#endif // LOGGER_H
