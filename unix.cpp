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
#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

#include <nlohmann/json.hpp>

#include "unix.h"

using namespace std;
using json = nlohmann::json;


void toDo(Os &os)
{
    os.build_unpack_dir();
    vector<string> unpack_cmd;
    enum archiver
    {
        tar,
        gem
    };
    int archiver = tar;
    if(os.get_archive_type() == ".tar")
    {
        unpack_cmd = {"tar", "-xvf"};
    }
    else if(os.get_archive_type() == ".tar.bz2" || os.get_archive_type() == ".tar.bzip2" || os.get_archive_type() == ".tb2" || os.get_archive_type() == ".tbz")
    {
        unpack_cmd = {"tar", "-xvjf"};
    }
    else if(os.get_archive_type() == ".tar.xz" || os.get_archive_type() == ".txz")
    {
        unpack_cmd = {"tar", "-xvJf"};

    }
    else if(os.get_archive_type() == ".tar.gz" || os.get_archive_type() == ".tgz")
    {
        unpack_cmd = {"tar", "-xvzf"};

    }
    else if(os.get_archive_type() == ".gem")
    {
        os.installation("ruby");
        os.install_gems();
        archiver = gem;
    }

    if(archiver == tar)
    {
        os.installation("tar");
        int mypipe[2];
        if(pipe(mypipe))
        {
            perror("Ошибка канала");
            exit(1);
        }
        vector<string> cmd = {"tar", "-tf", os.get_path_to_archive()};
        //флаг -c говорит о том, что команды считываются из строки
        if(os.run_command(cmd,false,mypipe) != 0)
        {
            cerr<<"Подозрительные файлы в архиве"<<"\n";
            exit(1);
        }
        else
        {
            unpack_cmd.push_back(os.get_path_to_archive());
            unpack_cmd.push_back("-C");
            unpack_cmd.push_back(os.get_dirBuild()/os.get_archive_name());
            os.return_code_command(unpack_cmd);
            cout<<"Архив разархивирован"<<"\n";
        }

        if(std::filesystem::exists(os.get_unpack_dir()/"CMakeLists.txt") && os.assembly_cmake() == 0) //CMake
        {
            cout<<"Собрано с помощью CMake"<<"\n";
            os.cd(os.get_build_dir());
            cout << os.get_current_path();
            os.return_code_command({"make", "-n", "install"},true);
            os.cd(os.get_current_path());
        }
        else if(std::filesystem::exists(os.get_unpack_dir()/"configure") && std::filesystem::exists(os.get_unpack_dir()/"Makefile.in")  //GNU Autotools
                && std::filesystem::exists(os.get_unpack_dir()/"config.h.in")  && os.assembly_autotools() == 0)
        {
            cout<<"Собрано с помощью GNU Autotools"<<"\n";
            os.cd(os.get_unpack_dir());
            os.return_code_command({"make", "-n", "install"},true);
            os.cd(os.get_unpack_dir());
        }
        else if(os.gemspec_exists(os.get_unpack_dir()) && os.assembly_gem() == 0) //Ruby тут есть gemspec
        {
            cout<<"Собрано с помощью gem"<<"\n";
            regex r("(.*)\\.gem$", regex_constants::icase);
            string tmp_path = os.find_file(r,os.get_unpack_dir());
            os.return_code_command({"gem", "install", tmp_path},true);
        }
        else if(std::filesystem::exists(os.get_unpack_dir()/"Rakefile") && !os.gemspec_exists(os.get_unpack_dir())) //Ruby тут нет gemspec
        {
            os.return_code_command({"gem", "install", "rake", "rake-compiler"},true);
            os.cd(os.get_unpack_dir());
            if(os.return_code_command({"rake", "--all", "-n"}) == 0) cout<<"Прогон всех шагов завершен успешно"<<"\n";
            os.cd(os.get_current_path());
        }
        else if(std::filesystem::exists(os.get_unpack_dir()/"Build.PL") && os.assembly_perl_build() == 0) //Perl
        {
            cout<<"Собрано с помощью Build и Build.PL"<<"\n";
            os.cd(os.get_unpack_dir());
            os.return_code_command({"./Build", "install"},true);
            os.cd(os.get_current_path());
        }

        else if(std::filesystem::exists(os.get_unpack_dir()/"Makefile.PL") && os.assembly_perl_make() == 0) //Perl
        {
            cout<<"Собрано с помощью make и Makefile.PL"<<"\n";
            os.cd(os.get_unpack_dir());
            os.return_code_command({"make", "-n", "install"},true);
            os.cd(os.get_current_path());
        }
        else if(std::filesystem::exists(os.get_unpack_dir()/"config.m4") && os.assembly_php() == 0) //PHP
        {
            cout<<"Собрано с помощью phpize и GNU Autotools"<<"\n";

            os.cd(os.get_unpack_dir());
            os.return_code_command({"make", "-n", "install"},true);

            string tmp_dir = os.get_unpack_dir()/"modules";

            string module;
            regex mask3("(.*)\\.so$", regex_constants::icase);
            module = os.find_file(mask3,tmp_dir,true);
            cout << "Для включения расширения добавьте в конфигурацию PHP строчку:" << endl
                << "extension=" << module << endl;

        }
        else cout<<"Не удалось собрать пакет"<<"\n";
    }

}


string Unix::os_name(string command)
{
    array<char, 200> buffer;
    string result;
    FILE* pipe = popen(command.c_str(),"r");
    while (fgets(buffer.data(), 200, pipe) != NULL)
    {
        result+= buffer.data();
    }
    pclose(pipe);
    return result;
}

void Unix::build_unpack_dir()
{
    dirBuild = "/tmp/archives";
    template_incorrect_path = "^(.*/)?\\.\\./";
    string path_arch = path_to_archive;
    string type_arch;
    cout << path_arch << "\n";
    filesystem::path path1(path_arch);
    string type_arch_tmp_one = path1.extension();
    size_t pos = path_arch.find(type_arch_tmp_one);
    string path_arch_tmp = path_arch.substr(0, pos);
    filesystem::path path2(path_arch_tmp);

    if (path2.extension() != ".tar")
    {
        type_arch = type_arch_tmp_one;
    }
    else
    {
        string type_arch_tmp_two = path2.extension();
        type_arch = type_arch_tmp_two+type_arch_tmp_one;
    }

    size_t found1 = path_arch.rfind("/");
    size_t found2 = path_arch.find(type_arch);
    int c = found2 - (found1 + 1);
    string arch = path_arch.substr(found1 + 1,c);
    archive_name = arch;
    type_archive = type_arch;
    cout << arch << "  " << type_arch << "\n";

    filesystem::path path3("/tmp/archives");
    path3 /= arch;
    path3 /= "build-"+arch;
    filesystem::create_directories(path3);

    unpack_dir = "/tmp/archives/" + arch + "/" + arch;
    build_dir = "/tmp/archives/" + arch + "/build-" + arch;

    cout << unpack_dir << endl;
    cout << build_dir << endl;
}

int Unix::run_command(std::vector<std::string> cmd, bool need_admin_rights, int *stdout_pipe, bool hide_stderr, std::string *ptr)
{
    if ( (need_admin_rights && !is_admin()))
        cmd.insert(cmd.begin(), "sudo");
    int return_code = 0;
    pid_t pid = fork();
    switch (pid) {
    case -1:
        cerr << "Не удалось разветвить процесс\n";
        exit(1);

    case 0: //дочерний процесс
    {
        if (stdout_pipe) {
            close(stdout_pipe[0]);
            if (hide_stderr)
            {
                int devNull = open("/dev/null", O_WRONLY);
                dup2(devNull, STDERR_FILENO);
            }
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

        execvp(c,args); //если удачно, то не возвращает управления,если ошибка-то вернет управление
        cerr << "Не удалось выполнить комманду exec\n";
        _exit(42);
    }
    }

    //pid>0 родительский процесс
    int status;

    if (stdout_pipe)
    {
        if (ptr)
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
            std::regex reg("CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES");
            while((linelen = getline(&line, &linesize, f)) != -1)
            {
                if (std::regex_search(line, reg))
                {
                    std::string temp_link_dirs = line;
                    int find_sep = temp_link_dirs.find('"');
                    temp_link_dirs = temp_link_dirs.substr(find_sep);
                    temp_link_dirs.erase(0,1);
                    temp_link_dirs.erase(temp_link_dirs.size() - 1);
                    *ptr = temp_link_dirs;
                    break;
                }
            }
            free(line);
            fclose(f);
        }
        else
            return_code = Os::check_tar(stdout_pipe);
    }

    //Если порожденный процесс, заданный параметром pid, к моменту системного вызова находится в
    //состоянии закончил исполнение, системный вызов возвращается немедленно без блокирования текущего процесса.
    do {
        waitpid(pid, &status, 0);
    } while(!WIFEXITED(status) && !WIFSIGNALED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
    //WIFSIGNALED возвращает истинное значение, если дочерний процесс завершился из-за необработанного сигнала.
    //ожидание изменения состояния процесса-потомка
    //завершается либо если корректно завершлся дочерний процесс
    //либо если он был прерван сигналом
    int child_status;
    if (WEXITSTATUS(status) == 0) child_status = 0;
    else child_status = 1;
    return_code = return_code || child_status;
    return return_code;
}

void Unix::find_packages(std::list<std::vector<std::string> > names)
{
    cout << "Поиск пакетов с помощью трассировки\n";

}

void Unix::find_libraries(std::vector<std::string> names, bool flag, std::vector<std::filesystem::path> paths)
{
    cout << "Поиск библиотек с помощью трассировки\n";

}

void Unix::find_lib(std::vector<std::string> libs, std::vector<std::string> libsPath)
{
    cout << "Поиск библиотек с помощью target\n";
}

int Unix::return_code_command(std::vector<std::string> cmd, bool need_admin_rights, int *stdout_pipe, bool hide_stderr)
{
    if (run_command(cmd, need_admin_rights, stdout_pipe, hide_stderr) != 0)
    {
        cerr << "Ошибка выполнения команды\n";
        exit(1);
    }
    return 0;
}

void Unix::install_gems()
{
    return_code_command({"cp", "-r", path_to_archive ,build_dir});
    return_code_command({"gem", "install",  path_to_archive},true);
}

void Unix::installation(const std::vector<std::string> packages) //программа ставится из одноименного пакета
{
    for (auto i : packages)
    {
        if (run_command({"command", "-v", i},false,nullptr,true) != 0)
        {
            std::vector <std::string> in = install;
            in.push_back(i);
            if (run_command(in,true)==0)
                std::cout << i << " : установлено" << "\n";
            else
            {
                cerr << "Не удалось установить: " << i;
                exit(1);
            }
        }
    }
}

void Unix::cd(std::string path)
{
    if (chdir(path.c_str()) != 0) //при успехе возвращается 0, при неудаче возвращается -1
    {
        cerr << "Не удалось перейти в каталог: " << path << endl;
        exit(1);
    }
}

void Unix::check_toolchain(std::regex mask1, std::regex mask2, std::filesystem::path pathToDir)
{
    //for (const auto& elem: il) не будет создавать копии и не позволит вносить какие-либо изменения.
    // Это может предотвратить случайные изменения и четко обозначить намерения.

    //auto — говорит, что компилятор на этапе компиляции должен определить тип
    //переменной на основе типа инициализируемого выражения, & — это ссылка… так что можно видеть entry как
    //int& или char& и т.д.
    //Ключевое слово const перед auto& это не константная ссылка,
    //эта константа относится к объекту, на который ссылается ссылка.Это означает, что итератор не может изменять данные,
    //а может только «читать».

    //Описывает итератор, выполняющий последовательный перебор имен файлов в каталоге.
    //path() возвращает полный путь к файлу
    //path().filename() возвращает имя файла по указанному пути
    std::filesystem::directory_iterator dirIterator1(pathToDir);
    std::string pathToFile;
    std::string target_language;
    std::string toolchain_language;

    for (const auto& entry : dirIterator1) //toolchain.json
    {
        if (!is_directory(entry))
        {
            if (std::regex_match(entry.path().c_str(),mask1))
            {
                pathToFile = entry.path();
                //чтение JSON из файла
                ifstream i(pathToFile);
                json j;
                i >> j;
                toolchain_language = j["toolchains"][0]["language"];
            }
        }
    }

    std::filesystem::directory_iterator dirIterator2(pathToDir); //target.json
    for (const auto& entry : dirIterator2)
    {
        if (!is_directory(entry))
        {
            if (std::regex_match(entry.path().c_str(),mask2))
            {
                pathToFile = entry.path();
                //чтение JSON из файла
                ifstream i(pathToFile);
                json j;
                i >> j;
                if (j["link"].size() != 0)
                    target_language = j["link"]["language"];//может не быть link и соответственно language
                else
                    target_language = "";
                if (target_language!="" && target_language != toolchain_language)
                {
                    cerr << "Параметры toolchain не совпадают с target\n";
                    exit(1);
                }

                cout << "Параметры toolchain совпадают с target\n";
            }
        }
    }
}

std::vector<string> Unix::linkDirectories(std::regex mask, std::filesystem::path pathToDir)
{
    std::string pathToFile;
    std::vector<std::string> linkDirs;
    std::filesystem::directory_iterator dirIterator(pathToDir);
    for (const auto& entry : dirIterator)
    {
        if (!is_directory(entry))
        {
            if (std::regex_match(entry.path().c_str(),mask))
            {
                pathToFile = entry.path();
                ifstream i(pathToFile);
                json j;
                i >> j;
                int size = j["toolchains"][0]["compiler"]["implicit"]["linkDirectories"].size();
                for (int i = 0; i < size; i++)
                    linkDirs.push_back(j["toolchains"][0]["compiler"]["implicit"]["linkDirectories"][i]);
            }
        }
    }

    return linkDirs;
}

std::vector<string> Unix::libp(std::filesystem::path pathToJson)
{

    ifstream i(pathToJson);
    std::vector<std::string> libraryPath;
    json j;
    i >> j;
    if (j["link"].is_null())
        return libraryPath;
    int size = j["link"]["commandFragments"].size();

    for (int ind = 0; ind < size; ind++)
    {
        if (j["link"]["commandFragments"][ind]["role"] == "libraryPath")
            libraryPath.push_back(j["link"]["commandFragments"][ind]["fragment"]);

    }

    return libraryPath;

}

void Unix::check_libs(std::vector<std::string> libs, std::filesystem::path buildDir)
{
    bool check = true;
    int size = libs.size();
    if (size == 0 || size == 1) //Если есть одна строка с -L, то все хорошо (только внешние или внутренние),если нет строки с -L, то все вообще хорошо
    {
        cout << "Правильный порядок включения библиотек при компоновке" << endl; //сначала пользовательские, потом системные
        return;
    }

    //Если больше 1, то надо следить
    //1)Сначала должны идти пользовательские,потом систменые
    //2)Есть только системные
    //3)Есть только пользовательские
    //1 - пользовательские
    //2 - системные
    //Системные - абсолютный путь
    // Пользовательские - относительный путь (или абсолютный путь к каталогу сборки)

    //Перечисление
    enum state
    {
        def, //по умолчанию 0
        internal, //внутренняя, по умолчанию 1
        external //внешняя, по умолчанию 2

    };
    int prev_type_lib = def;
    int curr_type_lib = def;
    std::string build_dir = buildDir;
    int build_dir_name_size = build_dir.size();

    while(build_dir[build_dir_name_size - 1] == '/')
    {
        build_dir.erase(build_dir_name_size - 1); //удаляется 1 символ с build_dir_name_size - 1 индекса
        //удаляется последний / в пути, если он был
        build_dir_name_size = build_dir.size();
    }

    std::string path;
    std::string tmp_path;
    for (auto i: libs)
    {
        prev_type_lib = curr_type_lib;
        path = i.substr(2, i.size() - 2); //путь к пути библиотеки (убираем -L)
        int size = path.size();

        if (path[size - 1] == '/') //удаляется последний / в пути, если он был
        {
            path.erase(size - 1);
            size = path.size();
        }

        if (size == build_dir_name_size)
        {
            if (build_dir == path)
                curr_type_lib = internal;
            else
                curr_type_lib = external;
        }
        else if (size > build_dir_name_size) //Возможно, что какой-то вложенный каталог в каталог сборки
        {
            tmp_path = path.substr(0, build_dir_name_size + 1); //захватываем +1 символ, чтобы проверить, что
            //первые  build_dir_name_size + 1 символов в пути path равны build_dir + "/"
            // /foo/bar /foo давали бы раньше верный резлуьтат при сравнении первых 4 символов
            //теперь сравнивается первые пять /foo/ /foo/ и результат тоже верный.
            //Раньше /foobar /foo давали неверный положительный результат ибо сравнивались первые 4 символа /foo /foo
            // теперь сраваниваем первые 5 символов /foob  /foo/ и видим, что первый путь не является путем
            //к вложенному в /foo/ подкаталогу
            if (tmp_path == (build_dir + "/")) //  Сранивается с добавлением /, так как возможнока ситуация /foo /foobar
                //первые 4 символа равны, но второй путь не является путем к подкаталогу /foo
                curr_type_lib = internal;
            else
                curr_type_lib = external;
        }
        else
            curr_type_lib = external;

        if (prev_type_lib == external && curr_type_lib == internal)
        {
            check = false;
            cerr << "Подозрительный порядок подключения библиотек при компоновке\n";
            exit(1);
        }
    }
}

std::vector<string> Unix::lib_list(std::filesystem::path pathToJson)
{
    std::vector<std::string> lib;
    ifstream i(pathToJson);
    json j;
    i >> j;
    if (j["link"].is_null())
        return lib;
    int size = j["link"]["commandFragments"].size();

    for (int ind = 0; ind < size; ind++)
    {
        if (j["link"]["commandFragments"][ind]["role"] == "libraries")
        {
            std::string tmp = j["link"]["commandFragments"][ind]["fragment"];
            tmp = tmp.substr(0,3);
            if (tmp != "-Wl")
                lib.push_back(j["link"]["commandFragments"][ind]["fragment"]);
        }
    }
    return lib;
}

void Unix::libs(std::regex mask, std::filesystem::path pathToDir, std::filesystem::path buildDir)
{
    //toolchain точно один
    std::regex mask1(".*(toolchains).*",std::regex_constants::icase);

    check_toolchain(mask1, mask, pathToDir);

    std::vector <std::string> linkDirs = linkDirectories(mask1,pathToDir); //Все каталоги, в которых стандартно ищутся библиотеки

    for (auto i: linkDirs)
        cout << i << " ";
    cout << endl;

    std::string pathToFile;
    std::filesystem::directory_iterator dirIterator(pathToDir);
    for (const auto& entry : dirIterator)
    {
        if (!is_directory(entry))
        {
            if (std::regex_match(entry.path().c_str(),mask))
            {
                //Берем бибилиотеки и пути с -L из каждого targeta, так как их может быть несколько
                std::vector<std::string> lib;//Библиотеки
                std::vector<std::string> libPath; //-L/...../
                pathToFile = entry.path();
                lib = lib_list(pathToFile);

                libPath = libp(pathToFile); //Тут пути с -L
                if (lib.size() != 0)
                {
                    for (auto j: lib)
                        cout << j << " ";
                    cout << endl;

                    if (libPath.size() != 0)
                    {
                        for (auto j: libPath)
                            cout << j << " ";
                        cout << endl;
                        check_libs(libPath, buildDir);
                        find_lib(lib, libPath);
                    }
                    else
                        find_lib(lib, linkDirs);
                }
            }
        }
    }
}

void Unix::cmake_libs()
{
    std::filesystem::path path_to_reply = build_dir;
    path_to_reply /= ".cmake";
    path_to_reply /= "api";
    path_to_reply /= "v1";
    path_to_reply /= "reply";

    std::regex mask1(".*(target).*",std::regex_constants::icase);
    libs(mask1, path_to_reply, build_dir);
}

int Unix::assembly_cmake()
{
    installation({"make"});
    installation({"cmake"});
    int sum_code = 0;
    cd(build_dir);
    std::filesystem::path tmp{std::filesystem::current_path()};
    uintmax_t n{std::filesystem::remove_all(tmp / "*")};
    //uintmax_t = unsigned long long

    tmp /= ".cmake";
    tmp /= "api";
    tmp /= "v1";
    tmp /= "query";
    std::filesystem::create_directories(tmp);

    cd(tmp);
    return_code_command({"touch", "codemodel-v2"});
    return_code_command({"touch", "toolchains-v1"});

    cd(build_dir);

    sum_code = sum_code || return_code_command({"cmake", unpack_dir});
    cmake_libs();
    sum_code = sum_code || return_code_command({"make"});

    cd(current_path);
    return sum_code;
}

int Unix::assembly_autotools()
{
    installation({"make"});
    int sum_code = 0;
    installation({"autoconf"});
    installation({"automake"});
    cd(unpack_dir);
    sum_code = sum_code || return_code_command({"./configure"});
    sum_code = sum_code || return_code_command({"make"});
    const auto copyOptions = std::filesystem::copy_options::recursive;
    std::filesystem::copy(unpack_dir, build_dir, copyOptions);
    cd(current_path);
    return sum_code;
}

string Unix::find_file(std::regex mask, std::filesystem::path pathToDir, bool return_name_file)
{
    std::string pathToFile = "";
    std::string namefile = "";

    std::filesystem::directory_iterator dirIterator(pathToDir);
    for (const auto& entry : dirIterator)
    {
        if (!is_directory(entry))
        {
            if (std::regex_match(entry.path().c_str(),mask))
            {
                pathToFile = entry.path();
                namefile = entry.path().filename();
                break;
            }
        }
    }

    if (return_name_file)
        return namefile;
    return pathToFile;
}

int Unix::gemspec_exists(std::string path)
{
    std::regex reg("(.*)\\.gemspec$", std::regex_constants::icase);
    return find_file(reg, path,true) != ""; //если 0, то нет gemspec, если 1, то есть gemspec
}

int Unix::assembly_gem()
{
    int sum_code = 0;
    installation({"git"});
    cd(unpack_dir);
    std::regex mask1(".*(.gemspec)",std::regex_constants::icase);
    std::regex mask2(".*(.gem)",std::regex_constants::icase);
    std::string gemspec_path = find_file(mask1, unpack_dir);
    sum_code = sum_code || return_code_command({"gem","build",gemspec_path});
    std::string gem_path = find_file(mask2, unpack_dir);
    const auto copyOptions = std::filesystem::copy_options::recursive;
    std::filesystem::copy(gem_path, build_dir, copyOptions);
    cd(current_path);
    return sum_code;
}

int Unix::assembly_perl_build()
{
    int sum_code = 0;
    return_code_command({"cpan", "Module::Build"},true);
    installation({perl_package_build});
    cd(unpack_dir);
    sum_code = sum_code || return_code_command({"cpanm", "--installdeps", "."},true);
    sum_code = sum_code || return_code_command({"perl", "Build.PL"},true);
    sum_code = sum_code || return_code_command({"./Build"},true);
    const auto copyOptions = std::filesystem::copy_options::recursive;
    std::filesystem::copy(unpack_dir, build_dir, copyOptions);
    cd(current_path);
    return sum_code;
}

int Unix::assembly_perl_make()
{
    installation({"make"});
    int sum_code = 0;
    cd(unpack_dir);
    sum_code = sum_code || return_code_command({"perl", "./Makefile.PL"});
    sum_code = sum_code || return_code_command({"make"});

    const auto copyOptions = std::filesystem::copy_options::recursive;
    std::filesystem::copy(unpack_dir, build_dir, copyOptions);

    cd(current_path);
    return sum_code;
}

int Unix::assembly_php()
{
    installation({"make"});
    int rc;
    int sum_code = 0;
    cd(unpack_dir);;
    sum_code = sum_code || return_code_command(phpize);
    sum_code = sum_code || return_code_command({"./configure"});
    sum_code = sum_code || return_code_command({"make"});

    const auto copyOptions = std::filesystem::copy_options::recursive;
    std::filesystem::copy(unpack_dir, build_dir, copyOptions);
    cd(current_path);

    return sum_code;
}

std::list<std::vector<string> > Unix::name_variants(std::string name)
{
    std::list< std::vector<std::string> > name_var;
    std::vector<std::string> tmp_names;

    //особый случай (пока один)
    //просто сохрняем в исходном виде
    if (name == "Curses")
    {
        tmp_names = {"Curses", "Ncurses"};
        name_var.push_back(tmp_names);
    }
    else
    {
        tmp_names = {name};
        name_var.push_back(tmp_names);
    }

    std::vector<std::string> first_filter;
    //просто приводим все к нижнему регистру
    for (std::vector<std::string> elem : name_var)
    {
        for (std::string i : elem)
        {
            for (int j = 0; j < i.size(); j++)
                if (isupper(i[j]))
                {
                    char t = tolower(i[j]);
                    i[j] = t;
                }
            first_filter.push_back(i);
        }
    }
    name_var.push_back(first_filter);

    //если несколько букв в верхнем регистре, то вставляем - и просто приводим в нижнему регистру,если одна заглавная
    std::vector<std::string> second_filter;
    std::string second_type_name;
    bool insert_tire;
    for (std::string i : name_var.front())
    {
        second_type_name = i;
        int counter = 0;
        int ind = 0;
        insert_tire = false;

        while(ind < second_type_name.size())
        {
            if (isupper(second_type_name[ind]))
            {
                counter++;
                char t = tolower(second_type_name[ind]);
                second_type_name[ind] = t;
                if (counter > 1)
                {
                    insert_tire = true;
                    std::string d;
                    d = second_type_name.substr(0,ind)  + "-" + second_type_name.substr(ind);
                    second_type_name = d;
                }
            }
            ind++;
        }

        if (insert_tire)
            second_filter.push_back(second_type_name);
    }
    if (second_filter.size() != 0)
        name_var.push_back(second_filter);

    //Добваляем lib в начало и расширения .so,.a
    std::vector<std::string> third_filter;
    std::string third_type_name;
    for (std::vector<std::string> elem : name_var)
    {
        for (std::string i : elem)
        {
            third_type_name = "lib" + i + ".so"; //динамическая библиотека
            third_filter.push_back(third_type_name);

            third_type_name = "lib" + i + ".a"; //статическая библиотека
            third_filter.push_back(third_type_name);
        }
    }
    name_var.push_back(third_filter);

    //+ -devel, -dev
    std::vector<std::string> fourth_filter;
    std::string fourth_type_name;
    for (std::vector<std::string> elem : name_var)
    {
        for (std::string i : elem)
        {
            if (i.substr(0,3) != "lib")
            {
                fourth_type_name = i + "-devel";
                fourth_filter.push_back(fourth_type_name);
                fourth_type_name = i + "-dev";
                fourth_filter.push_back(fourth_type_name);
            }
        }
    }
    name_var.push_back(fourth_filter);

    //libname-devel, libname-dev
    std::vector<std::string> five_filter;
    std::string five_type_name;
    for (auto elem : name_var.back())
    {
        five_type_name = "lib" + elem;
        five_filter.push_back(five_type_name);
    }
    name_var.push_back(five_filter);

    for (std::vector<std::string> elem : name_var)
    {
        cout << "{ ";
        for (std::string i : elem)
            cout << i << " ";
        cout << "}" << endl;
    }

    return name_var;
}

void Unix::trace()
{
    std::filesystem::path path_to_package = "/tmp/archives/" + archive_name;
    chdir(path_to_package.c_str());
    //chdir("/tmp/archives/xz-5.4.6");
    std::filesystem::path pathToDir = ".";
    std::filesystem::directory_iterator dirIterator(pathToDir);
    std::regex reg("^(trace).*");
    fstream in;  //поток для чтения
    std::string line;
    std::filesystem::path p;

    std::map<std::string, std::string> cmd_set;

    for (const auto& entry : dirIterator)
        if (!is_directory(entry)) //файл, а не каталог
        {
            if (std::regex_match(entry.path().filename().c_str(),reg))
            {
                p = entry.path().filename();
                in.open(entry.path().filename()); //связываем поток для чтения(ввода) с файлом
                break;
            }
        }

    if (in.is_open())
    {
        while (getline(in,line))
        {
            if (line[0] == '{')
            {
                stringstream s1;
                json j;
                s1 << line;
                s1 >> j;

                bool flag = false;
                std::vector <std::filesystem::path> paths;
                std::vector <std::string> names;

                if (j["cmd"] == "set")
                {
                    std::vector<std::string> key_value;

                    for (auto i : j["args"])
                        key_value.push_back(i);

                    int count = 0;
                    std::string key;
                    std::string value;
                    for (std::string i : key_value)
                    {
                        count++;
                        if (count == 1)
                            key = i;
                        if (count == 2)
                            value = i;
                    }

                    cmd_set[key] = value;
                    //cout << key << " " << cmd_set[key] << endl;
                }

                else if (j["cmd"] == "find_package")
                {
                    std::string package_name;
                    package_name = j["args"][0];
                    cout << j["args"][0] << endl;
                    std::list <std::vector <std::string> > names;
                    names = name_variants(package_name);
                    find_packages(names);
                }

                else if (j["cmd"] == "find_library")
                {
                    //Пока считается, что только такой формат
                    //find_library (<VAR> NAMES name PATHS paths... NO_DEFAULT_PATH)
                    //find_library (<VAR> NAMES name)
                    //std::string name_lib;
                    std::vector <std::string> names_lib;
                    std::vector <std::string> paths_lib;
                    //cout << j["args"] << endl;

                    if (j["args"][1] == "NAMES")
                    {
                        if (j["args"].size() > 3)
                        {
                            int size = j["args"].size();
                            for (int i = 2; i < size; i++)
                            {
                                std::string jargs = j["args"][i];
                                std::regex r("if ( not in the C library");
                                if (jargs != "HINTS" && jargs != "PATHS" && jargs != "NAMES_PER_DIR" && jargs != "DOC" && !std::regex_search(jargs, r))
                                    names_lib.push_back(j["args"][i]);
                                else
                                    break;
                            }
                        }
                        else
                            names_lib.push_back(j["args"][2]);
                    }
                    else
                        names_lib.push_back(j["args"][1]);

                    bool default_path = false;
                    if (j["args"].size() > 3)
                    {
                        int size = j["args"].size();
                        std::regex reg("[A-Z]*");
                        std::string json_string;
                        int ind;
                        for (int i = 2; i < size; i++)
                        {
                            json_string = j["args"][i];
                            if (j["args"][i] == "HINTS" || j["args"][i] == "PATHS")
                            {
                                ind = i;
                                flag = true;
                            }
                            else if (std::regex_match(json_string,reg))
                            {
                                break;
                            }

                            if (json_string == "NO_DEFAULT_PATH")
                                default_path = true;

                            if (flag && !default_path && ind != i)
                                paths_lib.push_back(j["args"][i]);
                        }
                    }

                    for (auto i : names_lib)
                    {
                        std::string tmp_name = i;
                        while(tmp_name[0] == '$')
                        {
                            tmp_name.erase(0,2);
                            tmp_name.erase(tmp_name.size() - 1);
                            //cout << tmp_name << " " << cmd_set[tmp_name] << " ";
                            if (cmd_set[tmp_name] != "")
                                tmp_name = cmd_set[tmp_name];
                            else
                                tmp_name = "";
                        }
                        cout << endl;
                        names.push_back(tmp_name);
                    }

                    for (auto i : paths_lib)
                    {
                        std::string tmp_path = i;
                        while(tmp_path[0] == '$')
                        {
                            tmp_path.erase(0,1);
                            tmp_path.erase(0,1);
                            tmp_path.erase(tmp_path.size() - 1);
                            //cout << tmp_path << " " << cmd_set[tmp_path] << " ";
                            if (cmd_set[tmp_path] != "")
                                tmp_path = cmd_set[tmp_path];
                            else
                                tmp_path = "";
                        }
                        cout << endl;
                        if (tmp_path != "")
                            paths.push_back(tmp_path);
                    }

                    // for (auto i : names)
                    //     cout << i << " ";
                    // cout << endl;
                    // for (auto j : paths)
                    //     cout << j << " ";
                    // cout << endl;
                    find_libraries(names, flag,paths);
                }
            }
        }
    }

    in.close();
    std::filesystem::remove(p);
}

bool Unix::is_admin() const {
    return getuid() == 0; //краткая запись if(getuid()==0) {return true;} else {return false;}
}

int Unix::run_command_trace(std::vector<std::string> cmd, bool need_admin_rights, bool first)
{

    if (need_admin_rights && !is_admin())
        cmd.insert(cmd.begin(), "sudo");
    int return_code = 0;
    if (first)
    {
        std::filesystem::path curr_path = std::filesystem::current_path();
        std::string pathtmp = "/tmp/archives/" + archive_name;
        chdir(pathtmp.c_str());
        int fd;
        char tmp[] = "./traceXXXXXX";; //0600 права доступа
        fd = mkstemp(tmp);
        dup2(fd, STDERR_FILENO);
        chdir(curr_path.c_str());
    }

    pid_t pid = fork();
    switch (pid) {
    case -1:
        cerr << "Не удалось разветвить процесс\n";
        exit(1);

    case 0: //дочерний процесс
    {
        //stdout_pipe[1] запись
        //stdout_pipe[0] чтение

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
        do {
            waitpid(pid, &status, 0);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
        int child_status;
        child_status = (WEXITSTATUS(status) != 0);
        return_code = return_code || child_status;
        break;

    }
    }

    if (first)
        trace();

    return return_code;
}

void Unix::cmake_trace()
{
    std::filesystem::path path_to_reply = build_dir;
    std::string link_directories;
    std::vector <std::string> link_dirs;

    int mypipe_inf[2];
    pipe(mypipe_inf);
    cd(build_dir);
    run_command({"cmake", "--system-information"},false,mypipe_inf,false, &link_directories);
    char str_without_sep[link_directories.size()];
    strcpy(str_without_sep, link_directories.c_str());
    char *pch = strtok(str_without_sep, ";");

    while(pch != NULL)
    {
        link_dirs.push_back(pch);
        pch = strtok(NULL, ";");
    }

    for (auto i : link_dirs)
        cout << i << endl;
    //cd(current_path);
    link_toolchain_dir = link_dirs;
    run_command_trace({"cmake", "--trace-format=json-v1", unpack_dir},false,true);
    //ptr_cmake_trace(unpack_dir,path_to_package,link_dirs, archive_name);
}
