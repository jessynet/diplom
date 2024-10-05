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
#ifndef PP_UBUNTU_H
#define PP_UBUNTU_H

#include "linux.h"


class Ubuntu : public Linux
{
    int find_install_package_for_lib(std::filesystem::path path, std::string name_so, std::string name_a, bool *found_lib = nullptr, bool library_not_found_anywhere = false);
    int run_command_1(std::vector<std::string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr);
    int find_install_package_1(int* stdout_pipe);

public:
    Ubuntu();

    virtual int assembly_cmake();
    virtual int assembly_perl_build();
    virtual int assembly_perl_make();
    virtual int assembly_php();
    virtual void install_gems();
    virtual void find_lib(std::vector<std::string> lib, std::vector<std::string> libsPath);
    virtual void find_packages(std::list<std::vector<std::string> > names);
    virtual void find_libraries(std::vector<std::string> names, bool flag, std::vector<std::filesystem::path> paths);
};

#endif // PP_UBUNTU_H
