#include "logger.hpp"

std::mutex Logger::print_mutex_;
LogLevel Logger::min_level_ = LogLevel::INFO;
