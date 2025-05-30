#pragma once

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    NONE
};

class Logger {
public:
    static void set_min_log_level(LogLevel level) {
        min_level_ = level;
    }

    static LogLevel level_from_string(const std::string& str_level) {
        static const std::unordered_map<std::string, LogLevel> level_map = {
            {"info", LogLevel::INFO},
            {"warning", LogLevel::WARNING},
            {"error", LogLevel::ERROR},
            {"none", LogLevel::NONE}
        };
        auto it = level_map.find(str_level);
        return it != level_map.end() ? it->second : LogLevel::INFO;
    }

    Logger(LogLevel level) : level_(level) {}

    template<typename T>
    Logger& operator<<(const T& value) {
        if (level_ >= min_level_) {
            stream_ << value;
        }
        return *this;
    }

    ~Logger() {
        if (level_ >= min_level_) {
            std::lock_guard<std::mutex> lck(print_mutex_);
            std::cout << "[" << level_to_string(level_) << "] " << stream_.str() << std::endl;
        }
    }
private:
    static std::string level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::INFO:
                return "info";
                break;
            case LogLevel::WARNING:
                return "warning";
                break;
            case LogLevel::ERROR:
                return "error";
                break;
            case LogLevel::NONE:
                return "none";
                break;
        }
        return "unknown";
    }

    static std::mutex print_mutex_;
    static LogLevel min_level_;
    LogLevel level_;
    std::ostringstream stream_;
};
