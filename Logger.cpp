#include "Logger.h"

// Constructor: Initializes the logger and opens the specified file.
Logger::Logger(const std::string& log_file) {
    eventLog.open(log_file, std::ios::app);
    if (!eventLog.is_open()) {
        throw std::runtime_error("Failed to open log file: " + log_file);
    }
    log("System", "Logger initialized.");
}

// Destructor: Logs a shutdown message and closes the file.
Logger::~Logger() {
    log("System", "Logger shutting down.");
    if (eventLog.is_open()) {
        eventLog.close();
    }
}

// Logs a formatted message to the file.
void Logger::log(const std::string& category, const std::string& message) {
    time_t now = getCurrentTimestamp();
    // Using std::put_time for safe, formatted time output.
    eventLog << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S")
             << " [" << category << "] " << message << "\n";
    eventLog.flush(); // Ensure the message is written immediately.
}

// Helper function to get the current time.
time_t Logger::getCurrentTimestamp() const {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}
