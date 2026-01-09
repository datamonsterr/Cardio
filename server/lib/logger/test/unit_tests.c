#include "logger.h"
#include "testing.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Test logger function writes to file
TEST(test_logger_creates_file)
{
    const char* log_file = "/tmp/test_log_1.log";

    // Remove file if it exists
    remove(log_file);

    logger(log_file, "INFO", "Test message");

    // Check file exists
    FILE* fp = fopen(log_file, "r");
    ASSERT(fp != NULL);
    fclose(fp);

    // Cleanup
    remove(log_file);
}

TEST(test_logger_writes_correct_tag)
{
    const char* log_file = "/tmp/test_log_2.log";

    remove(log_file);

    logger(log_file, "ERROR", "An error occurred");

    // Read file content
    FILE* fp = fopen(log_file, "r");
    char buffer[256];
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    // Check that tag is present
    ASSERT(strstr(buffer, "[ERROR]") != NULL);

    // Cleanup
    remove(log_file);
}

TEST(test_logger_writes_correct_message)
{
    const char* log_file = "/tmp/test_log_3.log";

    remove(log_file);

    logger(log_file, "DEBUG", "Debug information");

    // Read file content
    FILE* fp = fopen(log_file, "r");
    char buffer[256];
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    // Check that message is present
    ASSERT(strstr(buffer, "Debug information") != NULL);

    // Cleanup
    remove(log_file);
}

TEST(test_logger_appends_to_file)
{
    const char* log_file = "/tmp/test_log_4.log";

    remove(log_file);

    logger(log_file, "INFO", "First message");
    logger(log_file, "INFO", "Second message");

    // Read file content
    FILE* fp = fopen(log_file, "r");
    char buffer1[256], buffer2[256];
    fgets(buffer1, sizeof(buffer1), fp);
    fgets(buffer2, sizeof(buffer2), fp);
    fclose(fp);

    // Check both messages are present
    ASSERT(strstr(buffer1, "First message") != NULL);
    ASSERT(strstr(buffer2, "Second message") != NULL);

    // Cleanup
    remove(log_file);
}

TEST(test_logger_multiple_tags)
{
    const char* log_file = "/tmp/test_log_5.log";

    remove(log_file);

    logger(log_file, "INFO", "Info message");
    logger(log_file, "WARN", "Warning message");
    logger(log_file, "ERROR", "Error message");

    // Read file content
    FILE* fp = fopen(log_file, "r");
    char buffer1[256], buffer2[256], buffer3[256];
    fgets(buffer1, sizeof(buffer1), fp);
    fgets(buffer2, sizeof(buffer2), fp);
    fgets(buffer3, sizeof(buffer3), fp);
    fclose(fp);

    // Check all tags are present
    ASSERT(strstr(buffer1, "[INFO]") != NULL);
    ASSERT(strstr(buffer2, "[WARN]") != NULL);
    ASSERT(strstr(buffer3, "[ERROR]") != NULL);

    // Cleanup
    remove(log_file);
}

TEST(test_logger_includes_timestamp)
{
    const char* log_file = "/tmp/test_log_6.log";

    remove(log_file);

    logger(log_file, "INFO", "Message with timestamp");

    // Read file content
    FILE* fp = fopen(log_file, "r");
    char buffer[256];
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    // Check timestamp format (YYYY-MM-DD HH:MM:SS)
    // The timestamp should contain a date pattern
    ASSERT(strstr(buffer, "-") != NULL); // Date separator
    ASSERT(strstr(buffer, ":") != NULL); // Time separator

    // Cleanup
    remove(log_file);
}

TEST(test_logger_empty_message)
{
    const char* log_file = "/tmp/test_log_7.log";

    remove(log_file);

    logger(log_file, "INFO", "");

    // Read file content
    FILE* fp = fopen(log_file, "r");
    char buffer[256];
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    // Check that tag is still present even with empty message
    ASSERT(strstr(buffer, "[INFO]") != NULL);

    // Cleanup
    remove(log_file);
}

TEST(test_logger_long_message)
{
    const char* log_file = "/tmp/test_log_8.log";
    const char* long_msg = "This is a very long message that contains a lot of text to test if the logger can handle "
                           "longer strings without any issues or truncation problems.";

    remove(log_file);

    logger(log_file, "INFO", long_msg);

    // Read file content
    FILE* fp = fopen(log_file, "r");
    char buffer[512];
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    // Check that the long message is present
    ASSERT(strstr(buffer, long_msg) != NULL);

    // Cleanup
    remove(log_file);
}

TEST(test_logger_special_characters)
{
    const char* log_file = "/tmp/test_log_9.log";

    remove(log_file);

    logger(log_file, "INFO", "Message with special chars: @#$%^&*()");

    // Read file content
    FILE* fp = fopen(log_file, "r");
    char buffer[256];
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    // Check that special characters are preserved
    ASSERT(strstr(buffer, "@#$%^&*()") != NULL);

    // Cleanup
    remove(log_file);
}

int main()
{
    printf("Running Logger Library Unit Tests\n");
    printf("==================================\n");

    RUN_TEST(test_logger_creates_file);
    RUN_TEST(test_logger_writes_correct_tag);
    RUN_TEST(test_logger_writes_correct_message);
    RUN_TEST(test_logger_appends_to_file);
    RUN_TEST(test_logger_multiple_tags);
    RUN_TEST(test_logger_includes_timestamp);
    RUN_TEST(test_logger_empty_message);
    RUN_TEST(test_logger_long_message);
    RUN_TEST(test_logger_special_characters);

    printf("\n==================================\n");
    if (failed)
    {
        printf("Some tests FAILED\n");
        return 1;
    }
    else
    {
        printf("All tests PASSED\n");
        return 0;
    }
}
