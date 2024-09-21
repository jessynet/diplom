/*
 * Copyright (c) YYYY YOUR NAME HERE <user@your.dom.ain>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef WINDOWS_H
#define WINDOWS_H

#include "os.h"

class Windows
{
protected:
    std::filesystem::path curr_path;
    std::filesystem::path unpack_dir;
    std::filesystem::path build_dir;
    std::string archive_name;

public:
    int run_command(const char* cmd, bool redirection_stdout = false, bool trace = false);
    int run_command_pacman(const char* cmd);
    int run_command_pacman_check(const char* cmd, std::string full_path);
    std::list<std::vector<std::string>> name_variants(std::string name);
    int find_install_package_for_lib(std::filesystem::path pt, std::string dll, std::string a, std::string dlla, std::string lib, std::string stdout_path_str, bool *found_lib = nullptr, bool library_not_found_anywhere = false);
    void find_lib(std::vector<std::string> libs, std::vector<std::string> libsPath);
    void check_toolchain(std::regex mask1, std::regex mask2, std::filesystem::path pathToDir);
    std::vector<std::string> linkDirectories(std::regex mask, std::filesystem::path pathToDir);
    std::vector<std::string> libpath(std::filesystem::path pathToJson);
    void check_libs(std::vector<std::string> libs, std::filesystem::path buildDir);
    std::vector<std::string> lib_list(std::filesystem::path pathToJson);
    void libs_info(std::regex mask, std::filesystem::path pathToDir, std::filesystem::path buildDir);
    void cmake_trace();
    int assembly_cmake();
    void set_aname(string name);
    void set_bdir(std::filesystem::path p);
    void set_udir(std::filesystem::path p);
};

#endif // WINDOWS_H
