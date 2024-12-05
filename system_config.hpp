#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <map>
#include <vector>
#include <filesystem>
#include <atomic>

namespace fs = std::filesystem;

class SystemConfig {
public:
    // Storage management
    static constexpr uint64_t MIN_FREE_SPACE = 100 * 1024 * 1024;  // 100MB minimum free space
    static constexpr uint64_t LOG_ROTATE_SIZE = 50 * 1024 * 1024;  // 50MB log rotation
    static constexpr uint32_t MAX_CAPTURE_FILES = 1000;            // Maximum handshake files
    
    // Display configurations
    enum class DisplayMode {
        HEADLESS,
        HDMI,
        WAVESHARE_2_13,  // Common e-paper display
        AUTO
    };

    struct DisplayConfig {
        DisplayMode mode;
        uint32_t width;
        uint32_t height;
        uint32_t refresh_rate;
        bool enabled;
    };

    // System paths
    struct SystemPaths {
        fs::path root;           // Root directory
        fs::path config;         // Configuration files
        fs::path captures;       // Handshake captures
        fs::path logs;          // Log files
        fs::path models;        // AI models
        fs::path plugins;       // Plugin directory
    };

private:
    DisplayConfig display_config;
    SystemPaths paths;
    std::atomic<bool> low_power_mode{false};
    std::atomic<uint8_t> cpu_governor{0};  // 0=powersave, 1=ondemand, 2=performance
    
    // Storage monitoring
    std::atomic<uint64_t> free_space{0};
    std::atomic<bool> storage_warning{false};

    // Configuration file handling
    std::map<std::string, std::string> config_values;
    
    void initializePaths() {
        // Set up directory structure for SD card
        paths.root = "/opt/pwnagotchi";
        paths.config = paths.root / "config";
        paths.captures = paths.root / "captures";
        paths.logs = paths.root / "logs";
        paths.models = paths.root / "models";
        paths.plugins = paths.root / "plugins";

        // Create directories if they don't exist
        for (const auto& path : {paths.root, paths.config, paths.captures, 
                               paths.logs, paths.models, paths.plugins}) {
            fs::create_directories(path);
        }
    }

public:
    SystemConfig() {
        initializePaths();
        loadConfig();
    }

    bool initializeDisplay(DisplayMode mode = DisplayMode::AUTO) {
        display_config.mode = mode;
        
        if (mode == DisplayMode::AUTO) {
            // Auto-detect display
            if (checkHDMIConnection()) {
                display_config.mode = DisplayMode::HDMI;
            } else {
                display_config.mode = DisplayMode::HEADLESS;
            }
        }

        switch (display_config.mode) {
            case DisplayMode::HDMI:
                display_config.width = 800;
                display_config.height = 480;
                display_config.refresh_rate = 60;
                break;
            case DisplayMode::WAVESHARE_2_13:
                display_config.width = 250;
                display_config.height = 122;
                display_config.refresh_rate = 1;  // E-paper refresh rate
                break;
            case DisplayMode::HEADLESS:
                display_config.enabled = false;
                return true;
            default:
                return false;
        }

        display_config.enabled = true;
        return true;
    }

    // Storage management
    bool checkStorage() {
        std::error_code ec;
        auto space = fs::space(paths.root, ec);
        if (ec) return false;

        free_space = space.available;
        storage_warning = (free_space < MIN_FREE_SPACE);

        if (storage_warning) {
            cleanupOldFiles();
        }

        return !storage_warning;
    }

    void cleanupOldFiles() {
        // Rotate logs
        rotateLogFiles();
        
        // Clean old captures if needed
        cleanupCaptures();
    }

    void rotateLogFiles() {
        for (const auto& entry : fs::directory_iterator(paths.logs)) {
            if (fs::file_size(entry) > LOG_ROTATE_SIZE) {
                std::string filename = entry.path().string();
                std::string new_name = filename + ".1";
                fs::rename(filename, new_name);
            }
        }
    }

    void cleanupCaptures() {
        std::vector<fs::path> captures;
        for (const auto& entry : fs::directory_iterator(paths.captures)) {
            captures.push_back(entry.path());
        }

        if (captures.size() > MAX_CAPTURE_FILES) {
            // Sort by modification time
            std::sort(captures.begin(), captures.end(), 
                     [](const fs::path& a, const fs::path& b) {
                         return fs::last_write_time(a) < fs::last_write_time(b);
                     });

            // Remove oldest files
            size_t to_remove = captures.size() - MAX_CAPTURE_FILES;
            for (size_t i = 0; i < to_remove; ++i) {
                fs::remove(captures[i]);
            }
        }
    }

    // Power management
    void setLowPowerMode(bool enabled) {
        low_power_mode = enabled;
        if (enabled) {
            cpu_governor = 0;  // powersave
        } else {
            cpu_governor = 1;  // ondemand
        }
        updateCPUGovernor();
    }

    void updateCPUGovernor() {
        std::string governor;
        switch (cpu_governor) {
            case 0: governor = "powersave"; break;
            case 1: governor = "ondemand"; break;
            case 2: governor = "performance"; break;
            default: governor = "ondemand";
        }

        // Write to scaling_governor
        std::ofstream gov_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        if (gov_file.is_open()) {
            gov_file << governor;
        }
    }

    // Configuration management
    void loadConfig() {
        std::ifstream config_file(paths.config / "config.txt");
        std::string line;
        while (std::getline(config_file, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                config_values[key] = value;
            }
        }
    }

    void saveConfig() {
        std::ofstream config_file(paths.config / "config.txt");
        for (const auto& [key, value] : config_values) {
            config_file << key << "=" << value << "\n";
        }
    }

    // Getters
    const SystemPaths& getPaths() const { return paths; }
    const DisplayConfig& getDisplayConfig() const { return display_config; }
    bool isLowPowerMode() const { return low_power_mode; }
    uint64_t getFreeSpace() const { return free_space; }
    bool hasStorageWarning() const { return storage_warning; }

private:
    bool checkHDMIConnection() {
        // Check if HDMI is connected using tvservice
        std::string cmd = "tvservice -s";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return false;

        char buffer[128];
        std::string result;
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != nullptr)
                result += buffer;
        }
        pclose(pipe);

        return result.find("HDMI") != std::string::npos;
    }
};
