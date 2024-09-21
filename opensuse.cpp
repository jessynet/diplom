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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include "opensuse.h"

using namespace std;


OpenSuse::OpenSuse()
{
    cerr << "OpenSuse!\n";
}

int OpenSuse::assembly_perl_build()
{
    perl_package_build = "perl-App-cpanminus";
    Unix::installation({"perl"});
    return Unix::assembly_perl_build();
}

int OpenSuse::assembly_perl_make()
{
    Unix::installation({"perl"});
    perl_package_make = "libtirpc-devel";
    Unix::installation({perl_package_make});
    return Unix::assembly_perl_make();
}

int OpenSuse::assembly_php()
{
    Unix::installation({"php7", "php7-devel", "php7-pecl", "php7-pear"});
    phpize = {"/usr/bin/phpize"};
    return Unix::assembly_php();;
}

int OpenSuse::assembly_cmake()
{
    Unix::cmake_trace();
    return Unix::assembly_cmake();
}

void OpenSuse::install_gems()
{
    Unix::installation({"ruby-devel"});
    Unix::install_gems();
}

int OpenSuse::run_command_2(vector<string> cmd, bool need_admin_rights, int *stdout_pipe, string *path)
{
    if (need_admin_rights && !is_admin())
        cmd.insert(cmd.begin(), "sudo");
    int return_code = 0;
    string path_rpm = "";
    pid_t pid = fork();
    switch (pid) {
    case -1:
    {
        cerr << "Не удалось разветвить процесс\n";
        exit(1);
    }
    case 0: //дочерний процесс
    {
        if (stdout_pipe) {
            close(stdout_pipe[0]);
            dup2(stdout_pipe[1], STDOUT_FILENO); //делаем STDOUT_FILENO(стандартный вывод) копией stdout_pipe[1]
            //int dup2(int old_handle, int new_handle)
            //Функция dup2() дублирует old_handle как new_handle. Если имеется файл, который был связан с new_handle до вызова dup2(),
            //то он будет закрыт. В случае успеха возвращается 0, а в случае ошибки —1.
            //close(stdout_pipe[1]);
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
        break;
    }
    default: //pid>0 родительский процесс
    {
        int status; //цикл, так как дочерний процесс может быть прерван на какое-то время сигналов
        //например, заснуть, это будет расцененно как завершение дочернего процесса,но это не так
        bool found = false;
        if (stdout_pipe)
        {

            close(stdout_pipe[1]);

            FILE *f = fdopen(stdout_pipe[0], "r");
            if (!f) {
                auto es = strerror(errno);
                cerr << "fdopen failure: " << es << endl;
                exit(1);
            }

            char *line= NULL;
            size_t linesize = 0;
            ssize_t linelen;
            int size_path_lib = libname.size();
            string tmppath = "";
            while((linelen = getline(&line, &linesize, f)) != -1)
            {
                if (path)
                {
                    *path = line;
                    found = true;
                }
                else
                {
                    tmppath = line;
                    tmppath = tmppath.substr(0, size_path_lib);
                    if (tmppath == libname) found = true;

                }

            }
            free(line);
        }

        if (!found)
            return_code = 1;

        do {
            waitpid(pid, &status, 0);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
        int child_status;
        if (WEXITSTATUS(status) == 0) child_status = 0;
        else child_status = 1;
        return_code = return_code || child_status;
        break;
    }
    }
    return return_code;

}

int OpenSuse::find_install_package_2(int *stdout_pipe)
{
    bool found = false;
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
    string path_to_rpm_package = "";

    string name_package_for_lib = "";

    regex reg1(".rpm");
    while((linelen = getline(&line, &linesize, f)) != -1)
    {
        name_package_for_lib = "";
        if (regex_search(line,reg1))
        {
            string copy_line = line;
            int find_ind = copy_line.find(".rpm");
            int ind_prob;
            for (int i = find_ind; 0<=i; i--) //от .rpm до начала в поисках первого пробела, этот пробел между именем пакета и текстом вывода
                if (line[i] == ' ')
                {
                    ind_prob = i;
                    break;
                }

            for (int j = ind_prob + 1; j < find_ind + 4; j++)
                name_package_for_lib+=line[j];
            if (name_package_for_lib[0] == ' ')
                name_package_for_lib.erase(0,1);
            int size = name_package_for_lib.size();
            while(name_package_for_lib[size - 1] == ' ')
            {
                name_package_for_lib.erase(size - 1);
                size = name_package_for_lib.size();
            }

            cout << name_package_for_lib << endl;

            if (name_package_for_lib != "")
            {
                int mypipe3[2];
                if (pipe(mypipe3))
                {
                    perror("Ошибка канала");
                    exit(1);
                }
                run_command_2({"find", "/var/cache", "-type", "f", "-name", name_package_for_lib},true,mypipe3,&path_to_rpm_package);
                cout << path_to_rpm_package;
                int mypipe_4[2];
                if (pipe(mypipe_4))
                {
                    perror("Ошибка канала");
                    exit(1);
                }

                int f = path_to_rpm_package.find("\n");
                path_to_rpm_package.erase(f);

                if (run_command_2({"rpm", "-ql", path_to_rpm_package},false,mypipe_4) == 0)
                {
                    found = true;
                    break;
                }
            }
        }
    }

    //высвобождение буфера
    free(line);

    return found ? 0 : 1;
}

int OpenSuse::run_command_1(vector<string> cmd, bool need_admin_rights, int *stdout_pipe, bool not_found_full_path_lib, bool checking_existence_lib)
{
    if (need_admin_rights && !is_admin())
        cmd.insert(cmd.begin(), "sudo");
    int return_code = 0;
    pid_t pid = fork();
    switch (pid) {
    case -1:
    {
        cerr << "Не удалось разветвить процесс\n";
        exit(1);
    }
    case 0: //дочерний процесс
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
        break;
    }
    default: //pid>0 родительский процесс
    {
        int status;
        if (stdout_pipe)
        {
            if (!checking_existence_lib) //не требуется проверка предоставлении бибилотеки при скачиваниии без установки
            {
                if (!not_found_full_path_lib) //Библиотека установлена, ищется пакет, из которого она ставилась
                    find_install_package_1(stdout_pipe,1);
                else
                    if (find_install_package_1(stdout_pipe,2) != 0) //поиск пакета для библиотеки, которая не установлена, с конкретно заданым путем
                        return_code = 1;
            }
            else //требуется проверка предоставлении бибилотеки при скачиваниии без установки
            {
                if (find_install_package_2(stdout_pipe) != 0)
                    return_code = 1;

            }
        }

        do {
            waitpid(pid, &status, 0);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
        int child_status;
        if (WEXITSTATUS(status) == 0) child_status = 0;
        else child_status = 1;
        return_code = return_code || child_status;
        break;

    }
    }

    return return_code;
}

int OpenSuse::find_install_package_1(int *stdout_pipe, int code_funct)
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

    while((linelen = getline(&line, &linesize, f)) != -1)
    {
        name_package_for_lib = "";
        if (line[0] != 'S' && count(line, line + linelen, '|') == 3)
        {
            int c = 0;
            int ind1 = 0;
            int ind2 = 0;

            for (int i = 0; i < linelen; i++)
            {
                if (line[i] == '|' && (ind1 == 0 || ind2 == 0))
                {
                    c++;
                    if (c==1) ind1 = i;
                    else if (c==2) ind2 = i;
                }
            }

            for (int j = ind1 + 1; j < ind2; j++)
                name_package_for_lib+=line[j];
            if (name_package_for_lib[0] == ' ')
                name_package_for_lib.erase(0,1);
            int size = name_package_for_lib.size();
            while(name_package_for_lib[size - 1] == ' ')
            {
                name_package_for_lib.erase(size - 1);
                size = name_package_for_lib.size();
            }

            cout << name_package_for_lib << endl;

            if (code_funct == 1)
            {
                if (name_package_for_lib != "")
                {
                    found_package = true;
                    if (run_command_1({"zypper", "install", "-y", name_package_for_lib},true) == 0)
                    {
                        install = true;
                        break; //пока ставится первый найденный пакет
                    }
                    else
                        cout << "Не удалось установить предоставляющий библиотеку пакет " << name_package_for_lib << endl;

                }
            }

            if (code_funct == 2)
            {
                if (name_package_for_lib != "")
                {
                    int mypipe2[2];
                    if (pipe(mypipe2))
                    {
                        perror("Ошибка канала");
                        exit(1);
                    }
                    if (run_command_1({"zypper", "install", "--download-only", "-y", name_package_for_lib},true,mypipe2,true,true) == 0)
                    {
                        found_package = true;
                        if (run_command_1({"zypper", "install", "-y", name_package_for_lib},true) == 0)
                        {
                            install = true;
                            break;
                        }
                        else
                            cout << "Не удалось установить предоставляющий библиотеку пакет " << name_package_for_lib << endl;
                    }
                }
            }
        }
    }

    //высвобождение буфера
    free(line);

    if (!found_package)
        cout << "Не удалось найти пакет, который предоставляет библиотеку/пакет " << libname << endl; //не прерывается программа ибо может собраться пакет и без этой библиотеки

    if (install)
        return 0;
    else
        return 1;
}

void OpenSuse::find_lib(vector<string> libs, vector<string> libsPath)
{
    string path;
    string check_l;
    int mypipe[2];
    int mypipe2[2];
    string so_lib,a_lib;
    string tmp_lib_name;
    bool found;

    vector <std::filesystem::path> path_copy;
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
            if (std::filesystem::exists(path_tmp))
            {
                cout << "Библиотека " << l << " найдена" << endl;
                run_command_1({"zypper", "se", "--provides", l}, false, mypipe);
            }
            else
            {
                cout << "Библиотека " << l << " не найдена" << endl;
                if (run_command_1({"zypper", "se", "--provides", lib_name}, false, mypipe, true) != 0)
                    cout << "Не удалось найти пакет, который предоставляет библиотек/пакет " << l << endl;
            }
            continue;
        }

        so_lib = "";
        a_lib = "";
        tmp_lib_name = "";
        libname = "";

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
            else if (regex_search(l,r1)) //.so.****
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
                        if (run_command_1({"zypper", "se", "--provides", path_to_lib/so_lib}, false, mypipe) == 0)
                            found = true;
                    }
                    else if (std::filesystem::exists(path_to_lib/a_lib))
                    {
                        nl = a_lib;
                        libname = path_to_lib/a_lib;
                        cout << "Библиотека " << libname << " найдена" << endl;
                        if (run_command_1({"zypper", "se", "--provides", path_to_lib/a_lib}, false, mypipe) == 0)
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
                        if (run_command_1({"zypper", "se", "--provides", path_to_lib/so_lib}, false, mypipe) == 0)
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
                        if (run_command_1({"zypper", "se", "--provides", path_to_lib/a_lib}, false, mypipe) == 0)
                            found = true;
                    }
                }
                if (found)
                    break;
            }
        }
        else
        {
            cerr << "Некорректное имя библиотеки\n";
            exit(1);
        }

        if (!found) //библиотека не нашлась ни по одному из путей
        {
            cout << "Библиотека " << l << " не найдена ни по одному из указанных путей" << endl;
            cout << "Поиск пакета, который предоставляет необходимую библиотеку по одному из путей" << endl;
            for (auto i: path_copy)
            {
                libname = i/nl;
                pipe(mypipe2);
                if (run_command_1({"zypper", "se", "--provides", nl}, false, mypipe2,true) == 0)
                    break;
                //попытка найти пакет и установить библиотеку
            }
        }
    }
}

int OpenSuse::find_install_package_for_lib(std::filesystem::path path, string name_so, string name_a, bool *found_lib, bool library_not_found_anywhere)
{
    libname = path/name_so;
    int return_code = 0;
    int mypipe[2];
    if (pipe(mypipe))
    {
        perror("Ошибка канала");
        exit(1);
    }
    if (library_not_found_anywhere)
    {
        return_code = run_command_1({"zypper", "se", "--provides", name_so}, false, mypipe, true);
        if (return_code)
        {
            pipe(mypipe);
            libname = path/name_a;
            return_code = run_command_1({"zypper", "se", "--provides", name_a}, false, mypipe, true);
        }
        return return_code;
    }
    if (std::filesystem::exists(path/name_so))
    {
        if (found_lib)
            *found_lib = true;
        cout << "Библиотека " << path/name_so << " найдена" << endl;
        run_command_1({"zypper", "se", "--provides", path/name_so}, false, mypipe);
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
            run_command_1({"zypper", "se", "--provides", path/name_a}, false, mypipe);
        }
        else
        {
            cout << "Библиотека " << path/name_a << " не найдена" << endl;
            return_code = 1;
        }
    }
    return return_code;

}

void OpenSuse::find_packages(list<vector<string> > names)
{
    bool inst = false;
    int mypipe[2];
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

                if (run_command_trace({"zypper","install", i},true) == 0)
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
                            libname = path/l;
                            cout << "Библиотека " << path/l << "найдена" << endl;
                            if (run_command_1({"zypper", "se", "--provides", path/l}, false, mypipe) == 0)
                            {
                                lib_install = true;
                                inst = true;
                                break;
                            }
                        }
                    if (lib_install)
                        break;
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
                            if (run_command_1({"zypper", "se", "--provides", l}, false, mypipe, true) == 0)
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

void OpenSuse::find_libraries(vector<string> names, bool flag, vector<std::filesystem::path> paths)
{
    for (auto i : names)
    {
        if (i != "")
        {
            string l_name_so = "lib" + i + ".so";
            string l_name_a = "lib" + i + ".a";

            if (flag) //не использовать дефолтные пути, указана опция NO_DEFAULT_PATH
            {
                bool found_lib = false;
                if (paths.size() != 0)
                {
                    cout << "Поиск не осуществляется в дефолтных путях\n";
                    for (auto j : paths)
                        if (find_install_package_for_lib(j, l_name_so, l_name_a, &found_lib) == 0)
                            break;
                    if (!found_lib) //нигде не найдена библиотека
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

                if (!found_lib) //нигде не найдена библиотека
                {
                    cout << "Библиотека " << i << " не найдена ни в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                    for (auto j : all_paths)
                        if (find_install_package_for_lib(j, l_name_so, l_name_a, NULL, true) == 0)
                            break;
                }
            }
        }
    }
}
