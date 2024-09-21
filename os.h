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
#ifndef OS_H
#define OS_H

#include <filesystem>
#include <list>
#include <memory>
#include <regex>
#include <string>

class Os
{
protected:
    std::string current_path = std::filesystem::current_path();
    std::filesystem::path unpack_dir;
    std::filesystem::path build_dir;
    std::string archive_name;
    std::filesystem::path path_to_archive;
    std::filesystem::path dirBuild;
    std::string type_archive;
    std::vector<std::string> install;
    std::string perl_package_build;
    std::string perl_package_make;
    std::vector<std::string> phpize;
    std::vector<std::string> link_toolchain_dir;
    std::string libname;
    std::string template_incorrect_path;

public:

    virtual int run_command(std::vector<std::string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false, std::string *ptr = nullptr)  = 0;

    virtual void installation(std::vector<std::string> packages) = 0;

    void installation(const std::string &package) {std::vector<std::string> pkgs = {package}; installation(pkgs); }

    virtual void build_unpack_dir() = 0;

    virtual void install_gems() = 0;

    virtual void cd(std::string path) = 0;

    virtual int assembly_cmake() = 0;

    virtual int assembly_autotools() = 0;

    virtual void find_lib(std::vector<std::string> lib, std::vector<std::string> libsPath) = 0;

    virtual int gemspec_exists(std::string path) = 0;

    virtual std::string find_file(std::regex mask, std::filesystem::path pathToDir, bool return_name_file = false) = 0;

    virtual int assembly_gem() = 0;

    virtual void cmake_libs() = 0;

    virtual void cmake_trace() = 0;

    virtual int assembly_perl_build() = 0;

    virtual int assembly_perl_make() = 0;

    virtual void find_packages(std::list<std::vector<std::string>> names) = 0;

    virtual void find_libraries(std::vector<std::string> names, bool flag, std::vector<std::filesystem::path> paths) = 0;

    virtual int assembly_php() = 0;

    virtual int return_code_command(std::vector<std::string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false) = 0;

    std::string get_archive_type() const {return type_archive;}
    std::filesystem::path get_unpack_dir() const {return unpack_dir;}
    std::filesystem::path get_build_dir() const {return build_dir;}
    std::filesystem::path get_current_path() const {return current_path;}
    std::filesystem::path get_path_to_archive() const {return path_to_archive;}
    std::filesystem::path get_dirBuild() const {return dirBuild;}
    std::filesystem::path get_archive_name() const {return archive_name;}

    void set_path_to_archive(std::string path);
    void set_install_command(std::vector<std::string> cmd_install);

    int check_tar(int stdout_pipe[2]);
};


#endif // OS_H
