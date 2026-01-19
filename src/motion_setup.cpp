/*
 *    motion_setup.cpp
 *
 *    CLI tool for Motion authentication setup
 *    Prompts for admin and viewer passwords, hashes with bcrypt, saves to config
 *
 *    Copyright 2026 Motion Project
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 */

#include "webu_auth.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <termios.h>
#include <unistd.h>

const char* DEFAULT_CONFIG_PATH = "/usr/local/etc/motion/motion.conf";

/**
 * Get password from user without echoing to terminal
 */
std::string get_password(const char *prompt) {
    struct termios old_term, new_term;

    /* Turn off echo */
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    std::cout << prompt << ": " << std::flush;
    std::string password;
    std::getline(std::cin, password);
    std::cout << std::endl;

    /* Restore echo */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);

    return password;
}

/**
 * Update a parameter in the config file
 * Returns true on success, false on failure
 */
bool update_config_parameter(const std::string &config_path,
                             const std::string &param_name,
                             const std::string &param_value) {
    /* Read entire file */
    std::ifstream infile(config_path);
    if (!infile) {
        std::cerr << "Error: Cannot read config file: " << config_path << std::endl;
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    bool found = false;

    while (std::getline(infile, line)) {
        /* Check if this line sets the parameter (handle comments and whitespace) */
        std::string trimmed = line;
        /* Remove leading whitespace */
        size_t start = trimmed.find_first_not_of(" \t");
        if (start != std::string::npos) {
            trimmed = trimmed.substr(start);
        }

        /* Skip commented lines */
        if (!trimmed.empty() && trimmed[0] != ';' && trimmed[0] != '#') {
            /* Check if line starts with parameter name */
            if (trimmed.find(param_name) == 0) {
                /* Check if followed by whitespace or end of string */
                size_t param_len = param_name.length();
                if (trimmed.length() == param_len ||
                    trimmed[param_len] == ' ' ||
                    trimmed[param_len] == '\t') {
                    /* Replace this line */
                    lines.push_back(param_name + " " + param_value);
                    found = true;
                    continue;
                }
            }
        }

        /* Keep original line */
        lines.push_back(line);
    }
    infile.close();

    /* If parameter not found, append it */
    if (!found) {
        lines.push_back(param_name + " " + param_value);
    }

    /* Write back to file */
    std::ofstream outfile(config_path);
    if (!outfile) {
        std::cerr << "Error: Cannot write config file: " << config_path << std::endl;
        return false;
    }

    for (const auto &l : lines) {
        outfile << l << "\n";
    }

    return true;
}

int main(int argc, char **argv) {
    bool reset_mode = false;
    std::string config_path = DEFAULT_CONFIG_PATH;

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--reset") == 0) {
            reset_mode = true;
        } else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_path = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            std::cout << "Motion Authentication Setup\n";
            std::cout << "============================\n\n";
            std::cout << "Usage: motion-setup [OPTIONS]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --reset              Reset forgotten passwords\n";
            std::cout << "  --config PATH        Use alternate config file\n";
            std::cout << "  --help, -h           Show this help message\n\n";
            std::cout << "This tool configures Motion authentication by:\n";
            std::cout << "  1. Prompting for admin password (username: admin)\n";
            std::cout << "  2. Prompting for viewer username and password\n";
            std::cout << "  3. Hashing passwords with bcrypt (work factor 12)\n";
            std::cout << "  4. Updating config file\n\n";
            std::cout << "Note: Must be run as root to write to config file\n";
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            std::cerr << "Use --help for usage information\n";
            return 1;
        }
    }

    /* Check root privileges */
    if (geteuid() != 0) {
        std::cerr << "Error: motion-setup must be run as root\n";
        std::cerr << "Use: sudo motion-setup\n";
        return 1;
    }

    std::cout << "Motion Authentication Setup\n";
    std::cout << "============================\n\n";

    if (reset_mode) {
        std::cout << "Password Reset Mode\n\n";
    } else {
        std::cout << "This wizard will configure authentication for Motion.\n";
        std::cout << "You'll create two accounts:\n";
        std::cout << "  - admin: Full access (view, configure, control)\n";
        std::cout << "  - viewer: Read-only access (view only)\n\n";
    }

    /* Admin password */
    std::cout << "Admin Account (username: admin)\n";
    std::cout << "--------------------------------\n";

    std::string admin_pass = get_password("Admin password");
    std::string admin_pass_confirm = get_password("Confirm password");

    if (admin_pass != admin_pass_confirm) {
        std::cerr << "Error: Passwords don't match\n";
        return 1;
    }

    if (admin_pass.empty()) {
        std::cerr << "Error: Password cannot be empty\n";
        return 1;
    }

    /* Viewer credentials */
    std::cout << "\nViewer Account\n";
    std::cout << "---------------\n";
    std::cout << "Username [viewer]: " << std::flush;

    std::string viewer_user;
    std::getline(std::cin, viewer_user);
    if (viewer_user.empty()) {
        viewer_user = "viewer";
    }

    std::string viewer_pass = get_password("Viewer password");
    std::string viewer_pass_confirm = get_password("Confirm password");

    if (viewer_pass != viewer_pass_confirm) {
        std::cerr << "Error: Passwords don't match\n";
        return 1;
    }

    if (viewer_pass.empty()) {
        std::cerr << "Error: Password cannot be empty\n";
        return 1;
    }

    /* Hash passwords */
    std::cout << "\nHashing passwords with bcrypt (this may take a few seconds)...\n";

    std::string admin_hash = cls_webu_auth::hash_password(admin_pass);
    if (admin_hash.empty()) {
        std::cerr << "Error: Failed to hash admin password\n";
        return 1;
    }

    std::string viewer_hash = cls_webu_auth::hash_password(viewer_pass);
    if (viewer_hash.empty()) {
        std::cerr << "Error: Failed to hash viewer password\n";
        return 1;
    }

    /* Update config file */
    std::cout << "Updating config file: " << config_path << "\n";

    std::string admin_value = "admin:" + admin_hash;
    if (!update_config_parameter(config_path, "webcontrol_authentication", admin_value)) {
        return 1;
    }

    std::string viewer_value = viewer_user + ":" + viewer_hash;
    if (!update_config_parameter(config_path, "webcontrol_user_authentication", viewer_value)) {
        return 1;
    }

    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  Configuration Updated Successfully\n";
    std::cout << "========================================\n\n";
    std::cout << "Admin username:  admin\n";
    std::cout << "Admin password:  " << admin_pass << "\n\n";
    std::cout << "Viewer username: " << viewer_user << "\n";
    std::cout << "Viewer password: " << viewer_pass << "\n\n";
    std::cout << "Passwords have been hashed with bcrypt (work factor 12)\n";
    std::cout << "Config file updated: " << config_path << "\n\n";
    std::cout << "Restart Motion to apply changes:\n";
    std::cout << "  sudo systemctl restart motion\n\n";

    return 0;
}
