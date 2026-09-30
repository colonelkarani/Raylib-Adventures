#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <windows.h>
#include <shellapi.h>

// Set your target hour and minute (24-hour format)
int TARGET_HOUR = 7;   // e.g., 4:00 PM
int TARGET_MINUTE = 00; 

void openOperaGX(const std::string& url) {
    // Path to the executable (backslashes are escaped as \\)
    const char* exePath = "C:\\Users\\user\\AppData\\Local\\Programs\\Opera GX\\opera.exe";
    
    // ShellExecute cleanly opens the app with the URL argument without command line quoting issues
    ShellExecuteA(NULL, "open", exePath, url.c_str(), NULL, SW_SHOWNORMAL);
}

int main(int argc, char *argv[]) {
    // Clean up the URL format (No extra literal quotes needed for ShellExecute)
    std::string targetUrl ="https://us02web.zoom.us/j/88226457574?pwd=UlsgPtUaVjDLLDFwfXBk6npGctbhTH.1";

    if (sscanf(argv[1], "%d:%d", &TARGET_HOUR, &TARGET_MINUTE) == 2) {
        std::cout << "Hours: " << TARGET_HOUR << ", Minutes: " << TARGET_MINUTE << "\n";
    } else {
        std::cout << "Error: Invalid time format. Use HH:MM\n";
    }

    std::cout << "Target time set to: " 
              << std::setfill('0') << std::setw(2) << TARGET_HOUR << ":" 
              << std::setfill('0') << std::setw(2) << TARGET_MINUTE << "\n";
    std::cout << "Waiting for " << TARGET_HOUR << ":" << TARGET_MINUTE << std::endl;

    bool opened = false;

    while (!opened) {
        // Get current system time
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        
        // Convert to local time safely
        std::tm localTime;
        localtime_s(&localTime, &currentTime);

        // Check if current hour and minute match the target
        if (localTime.tm_hour == TARGET_HOUR && localTime.tm_min == TARGET_MINUTE) {
            std::cout << "Target time reached! Opening Opera GX...\n";
            openOperaGX(targetUrl);
            opened = true; // Exit loop after opening
        } else {
            // Sleep for 1 second before checking the time again
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}
