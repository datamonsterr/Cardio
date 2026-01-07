#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Original logger (backward compatible)
void logger(const char* log_file, const char* tag, const char* message)
{
    FILE* stream = fopen(log_file, "a");
    if (!stream) {
        fprintf(stderr, "Failed to open log file: %s\n", log_file);
        return;
    }
    time_t now;
    time(&now);
    struct tm* local = localtime(&now);
    fprintf(stream, "[%s] %d-%02d-%02d %02d:%02d:%02d %s\n", tag, local->tm_year + 1900, local->tm_mon + 1,
            local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec, message);
    fclose(stream);
}

// Enhanced logger with function name and terminal output
void logger_ex(const char* log_file, const char* tag, const char* function, const char* message, int to_terminal)
{
    time_t now;
    time(&now);
    struct tm* local = localtime(&now);
    
    // Format: [TAG] YYYY-MM-DD HH:MM:SS [function] message
    char formatted_msg[2048];
    snprintf(formatted_msg, sizeof(formatted_msg), 
             "[%s] %d-%02d-%02d %02d:%02d:%02d [%s] %s",
             tag, local->tm_year + 1900, local->tm_mon + 1,
             local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec,
             function, message);
    
    // Write to file
    FILE* stream = fopen(log_file, "a");
    if (stream) {
        fprintf(stream, "%s\n", formatted_msg);
        fclose(stream);
    }
    
    // Write to terminal if requested
    if (to_terminal) {
        // Use color codes for different log levels
        const char* color_code = "\033[0m";  // Default (white)
        if (strcmp(tag, "ERROR") == 0) {
            color_code = "\033[1;31m";  // Red
        } else if (strcmp(tag, "WARN") == 0) {
            color_code = "\033[1;33m";  // Yellow
        } else if (strcmp(tag, "INFO") == 0) {
            color_code = "\033[1;32m";  // Green
        } else if (strcmp(tag, "DEBUG") == 0) {
            color_code = "\033[1;36m";  // Cyan
        }
        fprintf(stdout, "%s%s\033[0m\n", color_code, formatted_msg);
        fflush(stdout);
    }
}