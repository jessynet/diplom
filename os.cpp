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

#include <unistd.h>
#include <iostream>
#include "os.h"


using namespace std;

void Os::set_path_to_archive(string path)
{
    path_to_archive = path;
}

void Os::set_install_command(vector<string> cmd_install)
{
    install = cmd_install;
}

int Os::check_tar(int stdout_pipe[2])
{
    int return_code = 0;
    close(stdout_pipe[1]);
    FILE *f = fdopen(stdout_pipe[0], "r");
    if (!f) {
        auto es = strerror(errno);
        cerr << "fdopen failure: " << es << endl;
        exit(1);
        // FIXME
    }

    char *line = NULL;
    size_t linesize = 0;
    ssize_t linelen;
    regex reg(template_incorrect_path, regex::extended);
    while((linelen = getline(&line, &linesize, f)) != -1)
    {
        if (regex_match(line, reg))
        {
            return_code = 1;
            break;
        }
    }
    free(line);
    fclose(f);

    return return_code;

}
