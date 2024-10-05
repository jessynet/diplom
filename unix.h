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
#ifndef PP_UNIX_H
#define PP_UNIX_H

#include "os.h"


void toDo(Os &os);

class Unix : public Os
{
    void check_toolchain(std::regex mask1, std::regex mask2, std::filesystem::path pathToDir);
    std::vector<std::string> linkDirectories(std::regex mask, std::filesystem::path pathToDir);
    std::vector<std::string> libp(std::filesystem::path pathToJson);
    void check_libs(std::vector<std::string> libs, std::filesystem::path buildDir);
    std::vector<std::string> lib_list(std::filesystem::path pathToJson);
    void libs(std::regex mask, std::filesystem::path pathToDir, std::filesystem::path buildDir);
    void cmake_libs();

    std::list< std::vector<std::string> > name_variants(std::string name);
    void trace();

protected:
    bool is_admin() const;
    int run_command_trace(std::vector <std::string> cmd, bool need_admin_rights = false, bool first = false);

public:
    std::string os_name(std::string command);

    virtual void build_unpack_dir();
    virtual int run_command(std::vector<std::string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false, std::string *ptr = nullptr);
    virtual void find_packages(std::list <std::vector <std::string> > names);
    virtual void find_libraries(std::vector<std::string> names, bool flag, std::vector <std::filesystem::path> paths);
    virtual void find_lib(std::vector<std::string> libs, std::vector<std::string> libsPath);
    virtual int return_code_command(std::vector<std::string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false);
    virtual void install_gems();
    virtual void installation(const std::vector <std::string> packages);
    virtual void cd(std::string path);

    virtual int assembly_cmake();
    virtual int assembly_autotools();
    virtual std::string find_file(std::regex mask, std::filesystem::path pathToDir, bool return_name_file = false);
    virtual int gemspec_exists(std::string path);
    virtual int assembly_gem();
    virtual int assembly_perl_build();
    virtual int assembly_perl_make();
    virtual int assembly_php();

    virtual void cmake_trace();
};


#endif // PP_UNIX_H
