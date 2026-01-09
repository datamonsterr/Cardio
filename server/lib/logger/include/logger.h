#pragma once

// Original logger (deprecated - use logger_ex)
void logger(const char* log_file, const char* tag, const char* message);

// Enhanced logger with function name and terminal output
void logger_ex(const char* log_file, const char* tag, const char* function, const char* message, int to_terminal);

// Convenience macros
#define LOG_INFO(msg) logger_ex(MAIN_LOG, "INFO", __func__, msg, 1)
#define LOG_WARN(msg) logger_ex(MAIN_LOG, "WARN", __func__, msg, 1)
#define LOG_ERROR(msg) logger_ex(MAIN_LOG, "ERROR", __func__, msg, 1)
#define LOG_DEBUG(msg) logger_ex(MAIN_LOG, "DEBUG", __func__, msg, 1)