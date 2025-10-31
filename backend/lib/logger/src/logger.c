#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void logger(const char* log_file, const char* tag, const char* message)
{
    FILE* stream = fopen(log_file, "a");
    if (stream == NULL)
    {
        // If we can't open the log file, write to stderr instead
        fprintf(stderr, "Warning: Could not open log file %s\n", log_file);
        return;
    }
    time_t now;
    time(&now);
    struct tm* local = localtime(&now);
    fprintf(stream, "[%s] %d-%02d-%02d %02d:%02d:%02d %s\n", tag, local->tm_year + 1900, local->tm_mon + 1,
            local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec, message);
    fclose(stream);
}