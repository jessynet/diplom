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

#include <sys/wait.h>
#include <unistd.h>

#include <iostream>

#include "freebsd.h"

using namespace std;


FreeBsd::FreeBsd()
{
    cerr << "FreeBSD!\n";
}

int FreeBsd::assembly_cmake()
{
    installation({"bash"});
    Unix::cmake_trace();
    return Unix::assembly_cmake();
}

int FreeBsd::assembly_perl_build()
{
    perl_package_build = "p5-App-cpanminus";
    Unix::installation({"perl5"});
    return Unix::assembly_perl_build();
}

int FreeBsd::assembly_perl_make()
{
    Unix::installation({"perl5"});
    return Unix::assembly_perl_make();
}

int FreeBsd::assembly_php()
{
    phpize = {"/usr/local/bin/phpize"};
    Unix::installation({"php83", "php83-pear", "php83-session", "php83-gd"});
    return Unix::assembly_php();
}

void FreeBsd::install_gems()
{
    Unix::installation({"rubygem-grpc"});
    Unix::install_gems();
}

int FreeBsd::run_command_1(vector<string> cmd, bool need_admin_rights, int *stdout_pipe)
{
    if (need_admin_rights && !is_admin())
        cmd.insert(cmd.begin(), "sudo");

    int return_code = 0;
    pid_t pid = fork();
    if (pid == -1)
    {
        cerr << "Не удалось разветвить процесс\n";
        exit(1);
    }

    if (pid == 0) //дочерний процесс
    {
        if (stdout_pipe) {
            close(stdout_pipe[0]);
            dup2(stdout_pipe[1], STDOUT_FILENO);

        }

        char *args[cmd.size()+1];
        for (int i = 0; i < cmd.size(); i++)
        {
            char *cstr = &(cmd[i])[0];
            args[i] = cstr;

        }

        args[cmd.size()] = NULL;

        char* c = args[0];

        execvp(c,args);
        cerr << "Не удалось выполнить комманду exec\n";
        _exit(42);
    }

    //pid>0 родительский процесс
    int status;
    if (stdout_pipe)
    {
        if (find_install_package_1(stdout_pipe)!=0)
            return_code = 1;
    }

    do {
        waitpid(pid, &status, 0);
    } while(!WIFEXITED(status) && !WIFSIGNALED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().

    int child_status;
    if (WEXITSTATUS(status) == 0) child_status = 0;
    else child_status = 1;
    return return_code || child_status;
}

int FreeBsd::find_install_package_1(int *stdout_pipe)
{
    close(stdout_pipe[1]);

    FILE *f = fdopen(stdout_pipe[0], "r");
    if (!f)
    {
        //Функция strerror() возвращает указатель на сообщение об ошибке, связанное с номером ошибки.
        auto es = strerror(errno);
        cerr << "fdopen failure: " << es << endl;
        exit(1);
    }

    char *line= NULL;
    size_t linesize = 0;
    ssize_t linelen;
    bool found_package = false;
    bool install = false;

    //int* ptr;
    //int** ptr1 = &ptr;
    //*ptr1 == ptr
    //**ptr1 == *ptr
    string name_package_for_lib = "";
    regex reg1("Name");
    regex reg2(":");
    while((linelen = getline(&line, &linesize, f)) != -1)
    {
        if (regex_search(line,reg1) && regex_search(line,reg2))
        {
            name_package_for_lib = "";
            name_package_for_lib = line;
            int find_del = name_package_for_lib.find(":");
            name_package_for_lib = name_package_for_lib.substr(find_del + 1);
            int size = name_package_for_lib.size();
            int find_new_str = name_package_for_lib.find("\n");
            name_package_for_lib.erase(find_new_str);
            while(name_package_for_lib[size - 1] == ' ')
            {
                name_package_for_lib.erase(size-1);
                int size = name_package_for_lib.size();
            }
            while(name_package_for_lib[0] == ' ')
                name_package_for_lib.erase(0,1);

            cout << name_package_for_lib << endl;

            if (name_package_for_lib != "")
            {
                found_package = true;
                if (run_command_1({"pkg", "install", "-y", name_package_for_lib},true) == 0)
                {
                    install = true;
                    break; //пока ставится первый найденный пакет
                }
                else cout << "Не удалось установить предоставляющий библиотеку пакет " << name_package_for_lib << endl;
            }
        }
    }

    //высвобождение буфера
    free(line);

    if (!found_package)
        cout << "Не удалось найти пакет, который предоставляет библиотеку/пакет " << libname << endl; //не прерывается программа ибо может собраться пакет и без этой библиотеки

    return !install;
}

void FreeBsd::find_lib(vector<string> libs, vector<string> libsPath)
{
    string path;
    string check_l;
    int mypipe[2];
    string so_lib,a_lib;
    string tmp_lib_name;
    bool found;
    vector <std::filesystem::path> path_copy;
    int mypipe2[2];
    string nl = "";

    for (auto l : libs)
    {
        found = false;
        if (pipe(mypipe))
        {
            perror("Ошибка канала");
            exit(1);
        }
        if (l[0] == '/') //если имя библиотеки начинается с /, то,значит, она написана с полным путем до нее
        {
            //cout << l << endl;
            std::filesystem::path path_tmp(l);
            string lib_name = path_tmp.filename(); //получаем имя библиотеки

            libname = l;
            if (std::filesystem::exists(l))
                cout << "Библиотека " << l << " найдена" << endl;
            else
            {
                cout << "Библиотека " << l << " не найдена" << endl;
                cout << "Поиск пакета, который предоставляет необходимую библиотеку" << endl;
            }

            run_command_1({"pkg","provides","^"+libname}, false, mypipe);
            continue;
        }

        so_lib = "";
        a_lib = "";
        tmp_lib_name = "";

        if (l[0] == '-' && l[1] == 'l') //-lname = libname.so || libname.a
        {
            tmp_lib_name = l.substr(2); //убираем -l
            so_lib = "lib" + tmp_lib_name + ".so";
            a_lib = "lib" + tmp_lib_name + ".a";

        }
        else if (l[0] == '-' && l[1] == 'p') //-pthread=-lpthread=libpthread.so || libpthread.a
        {
            tmp_lib_name = l.substr(1); //убираем -
            so_lib = "lib" + tmp_lib_name + ".so";
            a_lib = "lib" + tmp_lib_name + ".a";
        }
        else //прямое подключение name.a || name.so
        {
            regex r1(".so");
            regex r2(".a");
            if (regex_search(l,r2)) //.a.****
                a_lib = l;
            else if (regex_search(l,r1)) //.so.***
                so_lib = l;
        }

        if (so_lib != "" || a_lib != "")
        {
            for (auto p : libsPath) //ищем по всем путям указанным
            {
                path = p;
                check_l = "";
                for (int i = 0; i < 2; i++)
                    check_l += p[i];
                if (check_l == "-L")
                    path = p.substr(2, p.size() - 2); //путь к пути библиотеки (убираем -L, если передавали такие пути)

                int size = path.size();
                if (path[size - 1] == '/') //удаляется последний / в пути, если он был (нормализация пути)
                {
                    path.erase(size - 1);
                    size = path.size();
                }

                std::filesystem::path path_to_lib = path;

                path_copy.push_back(path_to_lib);

                if (so_lib != "" && a_lib != "") //для случая -lname -pname
                {
                    libname = path_to_lib/so_lib;
                    nl = so_lib;
                    if (std::filesystem::exists(path_to_lib/so_lib))
                    {
                        nl = so_lib;
                        libname = path_to_lib/so_lib;
                        cout << "Библиотека " << libname << " найдена" << endl;
                        string pathl = path_to_lib/so_lib;
                        if (run_command_1({"pkg","provides", "^"+pathl}, false, mypipe) == 0)
                            found = true;
                    }
                    else if (std::filesystem::exists(path_to_lib/a_lib))
                    {
                        nl = a_lib;
                        libname = path_to_lib/a_lib;
                        cout << "Библиотека " << libname << " найдена" << endl;
                        string pathl = path_to_lib/a_lib;
                        if (run_command_1({"pkg","provides","^"+pathl}, false, mypipe) == 0)
                            found = true;

                    }


                }
                else if (so_lib != "") //только .so
                {
                    libname = path_to_lib/so_lib;
                    nl = so_lib;
                    if (std::filesystem::exists(path_to_lib/so_lib))
                    {
                        cout << "Библиотека " << libname << " найдена" << endl;
                        string pathl = path_to_lib/so_lib;
                        if (run_command_1({"pkg","provides", "^"+pathl}, false, mypipe) == 0)
                            found = true;

                    }
                }
                else if (a_lib != "") //только .a
                {

                    libname = path_to_lib/a_lib;
                    nl = a_lib;
                    if (std::filesystem::exists(path_to_lib/a_lib))
                    {
                        cout << "Библиотека " << libname << " найдена" << endl;
                        string pathl = path_to_lib/a_lib;
                        if (run_command_1({"pkg","provides", "^"+pathl}, false, mypipe) == 0)
                            found = true;

                    }

                }

                if (found) break;
            }
        }
        else
        {
            cerr << "Некорректное имя библиотеки\n";
            exit(1);
        }

        if (!found) //библиотека не нашлась ни по одному из путей
        {
            //попытка найти пакет и установить библиотеку ,если она не нашлась(только для тех бибилотек, у которых не полный путь)
            cout << "Библиотека " << l << " не найдена ни по одному из указанных путей" << endl;
            cout << "Поиск пакета, который предоставляет необходимую библиотеку по одному из путей" << endl;
            for (auto i: path_copy)
            {
                libname = i/nl;
                pipe(mypipe2);
                if (run_command_1({"pkg", "provides", "^" + libname}, false, mypipe2) == 0)
                    break;
            }
        }
    }

}

int FreeBsd::find_install_package_for_lib(std::filesystem::path path, string name_so, string name_a, bool *found_lib, bool library_not_found_anywhere)
{
    libname = path/name_so;
    int mypipe[2];
    int return_code = 0;
    if (pipe(mypipe))
    {
        perror("Ошибка канала");
        exit(1);
    }

    if (library_not_found_anywhere)
    {
        return_code = run_command_1({"pkg", "provides", "^" + libname}, false, mypipe);
        if (return_code)
        {
            pipe(mypipe);
            libname = path/name_a;
            return_code = run_command_1({"pkg", "provides", "^" + libname}, false, mypipe);
        }
        return return_code;
    }

    if (std::filesystem::exists(path/name_so))
    {
        if (found_lib)
            *found_lib = true;
        cout << "Библиотека " << path/name_so << " найдена" << endl;
        run_command_1({"pkg", "provides", "^", libname}, false, mypipe);
    }
    else
    {
        cout << "Библиотека " << path/name_so << " не найдена" << endl;

        libname = path/name_a;
        if (pipe(mypipe))
        {
            perror("Ошибка канала");
            exit(1);
        }

        if (std::filesystem::exists(path/name_a))
        {
            if (found_lib)
                *found_lib = true;
            cout << "Библиотека " << path/name_a << " найдена" << endl;
            run_command_1({"pkg", "provides", "^" + libname}, false, mypipe);
        }
        else
        {
            cout << "Библиотека " << path/name_a << " не найдена" << endl;
            return_code = 1;
        }
    }

    return return_code;
}

void FreeBsd::find_packages(list<vector<string> > names)
{
    int mypipe[2];
    bool inst = false;
    for (vector<string> elem : names)
    {
        if (inst) break;
        for (string i : elem)
        {
            if (pipe(mypipe))
            {
                perror("Ошибка канала");
                exit(1);

            }
            if (i.substr(0,3) != "lib" || (i.substr(0,3) == "lib" && i.find("dev") != i.npos)) //не библиотека
            {
                cout << "Установка пакета " << i << endl;
                libname = i;

                if (run_command_trace({"pkg","install", i},true)  == 0)
                {
                    cout << "Пакет " << i << " установлен" << endl;
                    inst = true;
                    break;
                }
            }
            else //библиотека
            {
                vector <std::filesystem::path> standart_path;
                vector <string> libs = elem;
                for (auto p : link_toolchain_dir)
                    standart_path.push_back(p);
                bool lib_install = false;
                for (auto l : libs)
                {
                    for (auto path : standart_path)
                        if (std::filesystem::exists(path/l))
                        {
                            cout << "Библиотека " << path/l << "найдена" << endl;
                            libname = path/l;
                            if (run_command_1({"pkg", "provides", "^" + libname}, false, mypipe) == 0)
                            {
                                lib_install = true;
                                inst = true;
                                break;
                            }
                        }
                    if (lib_install) break;
                }

                if (!lib_install) //ни одна из библиотек не установлена, ищем любую и доставляем
                {
                    for (auto l : libs)
                    {
                        bool exit_found = false;
                        cout << "Библиотека " << l << " нигде не найдена" << endl;
                        for (auto path : standart_path)
                        {
                            libname = path/l;
                            pipe(mypipe);
                            if (run_command_1({"pkg", "provides", "^" + libname}, false, mypipe) == 0)
                            {
                                inst = true;
                                exit_found = true;
                                break;
                            }
                        }
                        if (exit_found)
                            break;
                    }
                }
                break;
            }
        }
    }
}

void FreeBsd::find_libraries(vector<string> names, bool flag, vector<std::filesystem::path> paths)
{
    for (auto i : names)
    {
        if (i != "")
        {
            string l_name_so = "lib" + i + ".so";
            string l_name_a = "lib" + i + ".a";
            if (flag) //не использовать дефолтные пути
            {
                bool found_lib = false;
                if (paths.size() != 0)
                {
                    cout << "Поиск не осуществляется в дефолтных путях\n";
                    for (auto j : paths)
                        if (find_install_package_for_lib(j, l_name_so, l_name_a, &found_lib) == 0)
                            break;
                    if (!found_lib)
                    {
                        cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                        for (auto j : paths)
                            if (find_install_package_for_lib(j, l_name_so, l_name_a, NULL, true) == 0)
                                break;
                    }
                }
                else
                    cout << "Нет путей (PATHS/HINTS) для поиска библиотеки " << i << endl;
            }
            else
            {
                cout << "Поиск осуществляется в дефолтных путях и в указанных дополнительно, если такие есть\n";
                regex mask1(".*(toolchains).*",regex_constants::icase);
                vector <std::filesystem::path> toolchainpath;
                for (auto p : link_toolchain_dir)
                    toolchainpath.push_back(p);
                vector <std::filesystem::path> all_paths;
                all_paths = toolchainpath;

                for (const auto& element : paths)
                    if (element != "") all_paths.push_back(element);

                bool found_lib = false;
                for (auto j : all_paths)
                {
                    if (find_install_package_for_lib(j, l_name_so, l_name_a, &found_lib) == 0)
                        break;
                }

                if (!found_lib)
                {
                    cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                    for (auto j : all_paths)
                        if (find_install_package_for_lib(j, l_name_so, l_name_a, NULL, true) == 0)
                            break;
                }
            }
        }
    }
}




