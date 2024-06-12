#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <array>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <list>
#include <algorithm>
#include <list>
#include <cmath>
#include <cctype>
#include <experimental/filesystem>
#if defined(__linux__) || defined(__unix__)
    #include <nlohmann/json.hpp>
    #include <sys/types.h>
    #include <sys/wait.h>
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #include <windows.h>
    #include <conio.h>
    #include <tchar.h>
    #include <dir.h>
    #include <nlohmann\json.hpp>
#endif    

using namespace std;
using json = nlohmann::json;
namespace fs = std::experimental::filesystem;

/*Различие в том, где препроцессор будет начинать поиск файла some.h. Если использовать директиву #include "some.h", то 
сначала будут просмотрены локальные (по отношению к проекту) папки включения файлов. Если использовать #include <some.h>,
то сначала будут просматриваться глобальные (по отношению к проекту) папки включения файлов. Глобальные папки включения -
это папки, прописанные в настройке среды разработки, локальные - это те, которые прописаны в настройках проекта.*/
#if defined(__linux__) || defined(__unix__)
    class Os
    {
        protected:
            string current_path = fs::current_path();
            fs::path unpack_dir;
            fs::path build_dir;
            string archive_name;
            fs::path path_to_archive;
            fs::path dirBuild;
            string type_archive;
            vector<string> install;
            string perl_package_build;
            string perl_package_make;
            vector <string> phpize;
            vector <string> link_toolchain_dir;
            string libname;
            string template_incorrect_path; 

        public:

            virtual int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false, string *ptr = nullptr)  = 0;

            virtual void installation(const vector <string> packages) = 0;

            void installation(string package) {vector<string> pkgs = {package}; installation(pkgs); }

            virtual void build_unpack_dir() = 0;

            virtual void install_gems() = 0;

            virtual void cd(string path) = 0;

            virtual int assembly_cmake() = 0;

            virtual int assembly_autotools() = 0;

            virtual void find_lib(vector<string> lib, vector<string> libsPath) = 0;

            virtual int gemspec_exists(string path) = 0;

            virtual string find_file(regex mask, fs::path pathToDir, bool return_name_file = false) = 0;

            virtual int assembly_gem() = 0;

            virtual void cmake_libs() = 0;

            virtual void cmake_trace() = 0;

            virtual int assembly_perl_build() = 0;

            virtual int assembly_perl_make() = 0;

            virtual void find_packages(list <vector <string> > names) = 0;

            virtual void find_libraries(vector<string> names, bool flag, vector <fs::path> paths) = 0;

            virtual int assembly_php() = 0;

            virtual int return_code_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false) = 0;

            string get_archive_type(){return type_archive;}
            fs::path get_unpack_dir(){return unpack_dir;}
            fs::path get_build_dir(){return build_dir;}
            fs::path get_current_path(){return current_path;}
            fs::path get_path_to_archive(){return path_to_archive;}
            fs::path get_dirBuild(){return dirBuild;}
            fs::path get_archive_name(){return archive_name;}
            void set_path_to_archive(string path){path_to_archive = path;}
            void set_install_command(vector <string> cmd_install){install = cmd_install;}
            int check_tar(int* stdout_pipe)
            {
                int return_code = 0;
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
                regex reg(template_incorrect_path, regex::extended);
                while((linelen = getline(&line, &linesize, f)) != -1)
                {
                        
                    if(regex_match(line, reg))
                    {
                        return_code = 1;
                        break;
                    }
                    
                }
                free(line);
                fclose(f);

                return return_code;

            }
            
    };

    class Unix : public Os
    { 
        public:
            string os_name(string command) 
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

            virtual void build_unpack_dir()
            {
                dirBuild = "/tmp/archives";
                template_incorrect_path = "^(.*/)?\\.\\./";
                string path_arch = path_to_archive;
                string type_arch;
                cout << path_arch << "\n";
                fs::path path1(path_arch);
                string type_arch_tmp_one = path1.extension();
                size_t pos = path_arch.find(type_arch_tmp_one);
                string path_arch_tmp = path_arch.substr(0, pos);
                fs::path path2(path_arch_tmp);
                if(path2.extension()!=".tar") 
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
                
                fs::path path3("/tmp/archives");
                path3 /= arch;
                path3 /= "build-"+arch;
                fs::create_directories(path3);

                unpack_dir = "/tmp/archives/" + arch + "/" + arch;
                build_dir = "/tmp/archives/" + arch + "/build-" + arch;

                cout << unpack_dir << endl;
                cout << build_dir << endl;
            }  

            bool is_admin() {
            
                return getuid() == 0; //краткая запись if(getuid()==0) {return true;} else {return false;}
            
            }

            virtual int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false, string *ptr = nullptr) 
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
                            if(hide_stderr)
                            {
                                int devNull = open("/dev/null", O_WRONLY);
                                dup2(devNull, STDERR_FILENO);
                            }
                            dup2(stdout_pipe[1], STDOUT_FILENO); 
                        }

                        char *args[cmd.size()+1];
                        for(int i = 0; i < cmd.size(); i++)
                        {
                            char *cstr = &(cmd[i])[0];
                            args[i] = cstr;

                        }

                        args[cmd.size()] = NULL;

                        char* c = args[0];
                        
                        execvp(c,args); //если удачно, то не возвращает управления,если ошибка-то вернет управление
                        cerr << "Не удалось выполнить комманду exec\n"; 
                        _exit(42);
                        break;
                        
                    }
                    default: //pid>0 родительский процесс
                    {
                        int status;
                            
                        if (stdout_pipe) 
                        {
                            if(ptr)
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
                                regex reg("CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES");
                                while((linelen = getline(&line, &linesize, f)) != -1)
                                {
                                    if(regex_search(line, reg))
                                    {
                                        string temp_link_dirs = line;
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
                            else return_code = Os::check_tar(stdout_pipe);

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
                        if(WEXITSTATUS(status) == 0) child_status = 0;
                        else child_status = 1;
                        return_code = return_code || child_status; 
                        break;
                    }   
                }
            
                return return_code;
            } 

            virtual void find_packages(list <vector <string> > names)
            {
                cout << "Поиск пакетов с помощью трассировки\n";

            }
            virtual void find_libraries(vector<string> names, bool flag, vector <fs::path> paths)
            {
                cout << "Поиск библиотек с помощью трассировки\n";

            }

            virtual void find_lib(vector<string> libs, vector<string> libsPath)
            {
                cout << "Поиск библиотек с помощью target\n";
            }

            virtual int return_code_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false)
            {
                if(run_command(cmd, need_admin_rights, stdout_pipe, hide_stderr) != 0) 
                {
                    cerr << "Ошибка выполнения команды\n";
                    exit(1);
                }
                else return 0;

            }
    
            virtual void install_gems()
            {
                return_code_command({"cp", "-r", path_to_archive ,build_dir});
                return_code_command({"gem", "install",  path_to_archive},true);    
            }

            virtual void installation(const vector <string> packages) //программа ставится из одноименного пакета 
            {
                for(auto i : packages)
                {
                    if(run_command({"command", "-v", i},false,nullptr,true) != 0)
                    {
                        vector <string> in = install;
                        in.push_back(i);
                        if(run_command(in,true)==0)
                            cout << i << " : установлено" << "\n";
                        else
                        {
                            cerr << "Не удалось установить: " << i;
                            exit(1);
                        }
                    }
                }
            } 

            virtual void cd(string path)
            {
                if(chdir(path.c_str()) != 0) //при успехе возвращается 0, при неудаче возвращается -1
                {
                    cerr << "Не удалось перейти в каталог: " << path << endl;
                    exit(1);
                }
            }
            void check_toolchain(regex mask1, regex mask2, fs::path pathToDir)
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
                fs::directory_iterator dirIterator1(pathToDir);
                string pathToFile;
                string target_language;
                string toolchain_language;
                
                for(const auto& entry : dirIterator1) //toolchain.json
                {
                    if(!is_directory(entry))
                    {
                        if(regex_match(entry.path().c_str(),mask1))
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

                fs::directory_iterator dirIterator2(pathToDir); //target.json
                for(const auto& entry : dirIterator2)
                {
                    if(!is_directory(entry))
                    {
                        if(regex_match(entry.path().c_str(),mask2))
                        {
                            pathToFile = entry.path();
                            //чтение JSON из файла
                            ifstream i(pathToFile);
                            json j;
                            i >> j;
                            if(j["link"].size() != 0)
                                target_language = j["link"]["language"];//может не быть link и соответственно language
                            else
                                target_language = "";
                            if(target_language!="" && target_language != toolchain_language)
                            {
                                cerr << "Параметры toolchain не совпадают с target\n";
                                exit(1);
                            }
                            else
                            {
                                cout << "Параметры toolchain совпадают с target\n";
                            }

                        }

                    }
                }

            }

            vector<string> linkDirectories(regex mask, fs::path pathToDir)
            {
                string pathToFile;
                vector<string> linkDirs;
                fs::directory_iterator dirIterator(pathToDir);
                for(const auto& entry : dirIterator)
                {
                    if(!is_directory(entry))
                    {
                        if(regex_match(entry.path().c_str(),mask))
                        {
                            pathToFile = entry.path();
                            ifstream i(pathToFile);
                            json j;
                            i >> j;
                            int size = j["toolchains"][0]["compiler"]["implicit"]["linkDirectories"].size();
                            for(int i = 0; i < size; i++)
                                linkDirs.push_back(j["toolchains"][0]["compiler"]["implicit"]["linkDirectories"][i]);
                    
                                            
                        }
                    }
                }

                return linkDirs;

            }

            vector<string> libp(fs::path pathToJson)
            {

                ifstream i(pathToJson);
                vector<string> libraryPath;
                json j;
                i >> j;
                if(j["link"].is_null())
                    return libraryPath;
                int size = j["link"]["commandFragments"].size();

                for (int ind = 0; ind < size; ind++)
                {
                    if(j["link"]["commandFragments"][ind]["role"] == "libraryPath")
                        libraryPath.push_back(j["link"]["commandFragments"][ind]["fragment"]);

                }

                return libraryPath;
                
            }

            void check_libs(vector<string> libs, fs::path buildDir)
            {
                bool check = true;
                int size = libs.size();
                if(size == 0 || size == 1) //Если есть одна строка с -L, то все хорошо (только внешние или внутренние),если нет строки с -L, то все вообще хорошо
                {    
                                
                    cout << "Правильный порядок включения библиотек при компоновке" << endl; //сначала пользовательские, потом системные 
                
                }
                else //Если больше 1, то надо следить
                {
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
                    string build_dir = buildDir;
                    int build_dir_name_size = build_dir.size();

                    while(build_dir[build_dir_name_size - 1] == '/')
                    {
                        build_dir.erase(build_dir_name_size - 1); //удаляется 1 символ с build_dir_name_size - 1 индекса
                    //удаляется последний / в пути, если он был
                        build_dir_name_size = build_dir.size();
                    }

                    string path;
                    string tmp_path;
                    for(auto i: libs)
                    {
                        prev_type_lib = curr_type_lib;
                        path = i.substr(2, i.size() - 2); //путь к пути библиотеки (убираем -L)
                        int size = path.size();

                        if(path[size - 1] == '/') //удаляется последний / в пути, если он был
                        {
                            path.erase(size - 1);
                            size = path.size();
                        }

                        if(size == build_dir_name_size)
                        {
                            if(build_dir == path)
                                curr_type_lib = internal;
                            else
                                curr_type_lib = external;
                        }
                        else if(size > build_dir_name_size) //Возможно, что какой-то вложенный каталог в каталог сборки
                        {
                            tmp_path = path.substr(0, build_dir_name_size + 1); //захватываем +1 символ, чтобы проверить, что 
                            //первые  build_dir_name_size + 1 символов в пути path равны build_dir + "/"
                            // /foo/bar /foo давали бы раньше верный резлуьтат при сравнении первых 4 символов
                            //теперь сравнивается первые пять /foo/ /foo/ и результат тоже верный.
                            //Раньше /foobar /foo давали неверный положительный результат ибо сравнивались первые 4 символа /foo /foo
                            // теперь сраваниваем первые 5 символов /foob  /foo/ и видим, что первый путь не является путем
                            //к вложенному в /foo/ подкаталогу
                            if(tmp_path == (build_dir + "/")) //  Сранивается с добавлением /, так как возможнока ситуация /foo /foobar
                            //первые 4 символа равны, но второй путь не является путем к подкаталогу /foo
                                curr_type_lib = internal;
                            else
                                curr_type_lib = external;

                        } 
                        else
                            curr_type_lib = external;

                        if(prev_type_lib == external && curr_type_lib == internal)
                        {
                            check = false;
                            cerr << "Подозрительный порядок подключения библиотек при компоновке\n";
                            exit(1);
                            
                        }

                    }

                }

            }

            vector<string> lib_list(fs::path pathToJson)
            {
                vector<string> lib;
                ifstream i(pathToJson);
                json j;
                i >> j;
                if(j["link"].is_null())
                    return lib;
                int size = j["link"]["commandFragments"].size();

                for (int ind = 0; ind < size; ind++)
                {
                    if(j["link"]["commandFragments"][ind]["role"] == "libraries")
                    {
                        string tmp = j["link"]["commandFragments"][ind]["fragment"];
                        tmp = tmp.substr(0,3);
                        if(tmp != "-Wl")
                            lib.push_back(j["link"]["commandFragments"][ind]["fragment"]);

                    }

                }

                return lib;

            }

            

            void libs(regex mask, fs::path pathToDir, fs::path buildDir)
            {
                //toolchain точно один
                regex mask1(".*(toolchains).*",regex_constants::icase);

                check_toolchain(mask1, mask, pathToDir);

                vector <string> linkDirs = linkDirectories(mask1,pathToDir); //Все каталоги, в которых стандартно ищутся библиотеки

                for(auto i: linkDirs)
                    cout << i << " ";
                cout << endl;

                string pathToFile;
                fs::directory_iterator dirIterator(pathToDir);
                for(const auto& entry : dirIterator)
                {
                    if(!is_directory(entry))
                    {
                        if(regex_match(entry.path().c_str(),mask))
                        {
                            //Берем бибилиотеки и пути с -L из каждого targeta, так как их может быть несколько
                            vector<string> lib;//Библиотеки
                            vector<string> libPath; //-L/...../
                            pathToFile = entry.path();
                            lib = lib_list(pathToFile);

                            libPath = libp(pathToFile); //Тут пути с -L
                            if(lib.size() != 0)
                            {
                                for(auto j: lib)
                                    cout << j << " ";
                                cout << endl;
                            
                                if(libPath.size() != 0)
                                {
                                    for(auto j: libPath)
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

            void cmake_libs()
            {
                fs::path path_to_reply = build_dir;
                path_to_reply /= ".cmake";
                path_to_reply /= "api";
                path_to_reply /= "v1";
                path_to_reply /= "reply";

                regex mask1(".*(target).*",regex_constants::icase);
                libs(mask1, path_to_reply, build_dir);

            }

            virtual int assembly_cmake()
            {   
                installation({"make"});
                installation({"cmake"});
                int sum_code = 0;
                cd(build_dir);
                fs::path tmp{fs::current_path()};
                uintmax_t n{fs::remove_all(tmp / "*")};
                //uintmax_t = unsigned long long

                tmp /= ".cmake";
                tmp /= "api";
                tmp /= "v1";
                tmp /= "query";
                fs::create_directories(tmp);

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

            virtual int assembly_autotools()
            {
                installation({"make"});
                int sum_code = 0;
                installation({"autoconf"});
                installation({"automake"});
                cd(unpack_dir);
                sum_code = sum_code || return_code_command({"./configure"});
                sum_code = sum_code || return_code_command({"make"});
                const auto copyOptions = fs::copy_options::recursive;
                fs::copy(unpack_dir, build_dir, copyOptions);
                cd(current_path);
                return sum_code;
            }

            virtual string find_file(regex mask, fs::path pathToDir, bool return_name_file = false)
            {
                string pathToFile = "";
                string namefile = "";

                fs::directory_iterator dirIterator(pathToDir);
                for(const auto& entry : dirIterator)
                {
                    if(!is_directory(entry))
                    {
                        if(regex_match(entry.path().c_str(),mask))
                        {
                            pathToFile = entry.path();
                            namefile = entry.path().filename();
                            break;
                        }

                    }

                }

                if(return_name_file) return namefile;

                return pathToFile;

            }

            virtual int gemspec_exists(string path)
            {
                regex reg("(.*)\\.gemspec$", regex_constants::icase);
                return find_file(reg, path,true) != ""; //если 0, то нет gemspec, если 1, то есть gemspec
            }

            virtual int assembly_gem()
            {
                int sum_code = 0;
                installation({"git"});
                cd(unpack_dir);
                regex mask1(".*(.gemspec)",regex_constants::icase);
                regex mask2(".*(.gem)",regex_constants::icase);
                string gemspec_path = find_file(mask1, unpack_dir);
                sum_code = sum_code || return_code_command({"gem","build",gemspec_path});
                string gem_path = find_file(mask2, unpack_dir);
                const auto copyOptions = fs::copy_options::recursive;
                fs::copy(gem_path, build_dir, copyOptions);
                cd(current_path);
                return sum_code;
            }

            virtual int assembly_perl_build()
            {
                int sum_code = 0;
                return_code_command({"cpan", "Module::Build"},true);
                installation({perl_package_build});
                cd(unpack_dir);
                sum_code = sum_code || return_code_command({"cpanm", "--installdeps", "."},true);
                sum_code = sum_code || return_code_command({"perl", "Build.PL"},true);
                sum_code = sum_code || return_code_command({"./Build"},true);
                const auto copyOptions = fs::copy_options::recursive;
                fs::copy(unpack_dir, build_dir, copyOptions);
                cd(current_path);
                return sum_code;

            }

            virtual int assembly_perl_make()
            {
                installation({"make"});
                int sum_code = 0;
                cd(unpack_dir);
                sum_code = sum_code || return_code_command({"perl", "./Makefile.PL"});
                sum_code = sum_code || return_code_command({"make"});

                const auto copyOptions = fs::copy_options::recursive;
                fs::copy(unpack_dir, build_dir, copyOptions);

                cd(current_path);
                return sum_code;
            }

            virtual int assembly_php()
            {
                installation({"make"});
                int rc;
                int sum_code = 0;
                cd(unpack_dir);;
                sum_code = sum_code || return_code_command(phpize);
                sum_code = sum_code || return_code_command({"./configure"});
                sum_code = sum_code || return_code_command({"make"});

                const auto copyOptions = fs::copy_options::recursive;
                fs::copy(unpack_dir, build_dir, copyOptions);
                cd(current_path);

                return sum_code;
            }
            
            
            list< vector<string> > name_variants(string name)
            {
                list< vector<string> > name_var;
                vector<string> tmp_names;

                

                //особый случай (пока один)
                //просто сохрняем в исходном виде
                if(name == "Curses")
                {
                    tmp_names = {"Curses", "Ncurses"};
                    name_var.push_back(tmp_names);
                }
                else
                {
                    tmp_names = {name};
                    name_var.push_back(tmp_names);

                }

                vector<string> first_filter; 
                //просто приводим все к нижнему регистру
                for(vector<string> elem : name_var)
                {
                    for(string i : elem)
                    {
                        for(int j = 0; j < i.size(); j++)
                            if(isupper(i[j]))
                            {
                                char t = tolower(i[j]);
                                i[j] = t;
                            }
                        first_filter.push_back(i);
                    }
                }
                name_var.push_back(first_filter);

                //если несколько букв в верхнем регистре, то вставляем - и просто приводим в нижнему регистру,если одна заглавная
                vector<string> second_filter; 
                string second_type_name;
                bool insert_tire;
                for(string i : name_var.front())
                {
                    second_type_name = i;
                    int counter = 0;
                    int ind = 0;
                    insert_tire = false;
                    
                    while(ind < second_type_name.size())
                    {
                        if(isupper(second_type_name[ind]))
                        {
                            counter++;
                            char t = tolower(second_type_name[ind]);
                            second_type_name[ind] = t;
                            if(counter > 1)
                            {
                                insert_tire = true;
                                string d;
                                d = second_type_name.substr(0,ind)  + "-" + second_type_name.substr(ind);
                                second_type_name = d;
                            }
                        }
                        ind++;
                    }

                    if(insert_tire) second_filter.push_back(second_type_name);
                }
                if(second_filter.size() != 0) name_var.push_back(second_filter);

                
                //Добваляем lib в начало и расширения .so,.a
                vector<string> third_filter;
                string third_type_name;
                for(vector<string> elem : name_var)
                {
                    for(string i : elem)
                    {
                        third_type_name = "lib" + i + ".so"; //динамическая библиотека
                        third_filter.push_back(third_type_name);

                        third_type_name = "lib" + i + ".a"; //статическая библиотека
                        third_filter.push_back(third_type_name);

                    }

                }

                name_var.push_back(third_filter);



                //+ -devel, -dev
                vector<string> fourth_filter;
                string fourth_type_name;
                for(vector<string> elem : name_var)
                {
                    for(string i : elem)
                    {
                        if(i.substr(0,3) != "lib")
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
                vector<string> five_filter;
                string five_type_name;
                for(auto elem : name_var.back())
                {
                    five_type_name = "lib" + elem;
                    five_filter.push_back(five_type_name);               

                }

                name_var.push_back(five_filter);


                for(vector<string> elem : name_var)
                {
                    cout << "{ ";
                    for(string i : elem)
                        cout << i << " ";
                    cout << "}" << endl;
                }

                return name_var;

            }

            void trace()
            {
                fs::path path_to_package = "/tmp/archives/" + archive_name;
                chdir(path_to_package.c_str());
                //chdir("/tmp/archives/xz-5.4.6");
                fs::path pathToDir = ".";
                fs::directory_iterator dirIterator(pathToDir);
                regex reg("^(trace).*");
                fstream in;  //поток для чтения
                string line;
                fs::path p;

                map <string, string> cmd_set;

                for(const auto& entry : dirIterator)
                    if(!is_directory(entry)) //файл, а не каталог
                    {
                        if(regex_match(entry.path().filename().c_str(),reg))
                        {
                            p = entry.path().filename();
                            in.open(entry.path().filename()); //связываем поток для чтения(ввода) с файлом
                            break;

                        }
                    }

                
                if(in.is_open())
                {
                    while(getline(in,line))
                    {
                        if(line[0] == '{')
                        {
                            stringstream s1;
                            json j;
                            s1 << line;
                            s1 >> j;


                            bool flag = false;
                            vector <fs::path> paths;
                            vector <string> names;

                            if(j["cmd"] == "set")
                            {
                                vector<string> key_value;

                                for(auto i : j["args"])
                                    key_value.push_back(i);

                                int count = 0;
                                string key;
                                string value;
                                for(string i : key_value)
                                {
                                    count++;
                                    if(count == 1)
                                        key = i;
                                    if(count == 2)
                                        value = i;
                                }
                            
                                cmd_set[key] = value;
                                //cout << key << " " << cmd_set[key] << endl;
                            
                            }

                            else if(j["cmd"] == "find_package")
                            {
                                string package_name;
                                package_name = j["args"][0];
                                cout << j["args"][0] << endl;
                                list <vector <string> > names;
                                names = name_variants(package_name);
                                find_packages(names);
                            }

                            else if(j["cmd"] == "find_library")
                            {
                                //Пока считается, что только такой формат
                                //find_library (<VAR> NAMES name PATHS paths... NO_DEFAULT_PATH)
                                //find_library (<VAR> NAMES name)
                                //string name_lib;
                                vector <string> names_lib;
                                vector <string> paths_lib;
                                //cout << j["args"] << endl;  

                                if(j["args"][1] == "NAMES")
                                {
                                    if(j["args"].size() > 3)
                                    {
                                        int size = j["args"].size();
                                        for(int i = 2; i < size; i++)
                                        {
                                            string jargs = j["args"][i];
                                            regex r("if not in the C library");
                                            if(jargs != "HINTS" && jargs != "PATHS" && jargs != "NAMES_PER_DIR" && jargs != "DOC" && !regex_search(jargs, r))
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
                                if(j["args"].size() > 3)
                                {
                                    int size = j["args"].size();
                                    regex reg("[A-Z]*");
                                    string json_string;
                                    int ind;
                                    for(int i = 2; i < size; i++)
                                    {
                                        json_string = j["args"][i];
                                        if(j["args"][i] == "HINTS" || j["args"][i] == "PATHS")
                                        {
                                            ind = i;
                                            flag = true;
                                            
                                        }
                                        else if(regex_match(json_string,reg))
                                        {
                                            break;
                                        }

                                        if(json_string == "NO_DEFAULT_PATH") default_path = true;
                                            
                                        if(flag && !default_path && ind != i) paths_lib.push_back(j["args"][i]);
                                        
                                    }
                                    

                                }

                                for(auto i : names_lib)
                                {

                                    string tmp_name = i;
                                    while(tmp_name[0] == '$')
                                    {
                                        tmp_name.erase(0,2);
                                        tmp_name.erase(tmp_name.size() - 1);
                                        //cout << tmp_name << " " << cmd_set[tmp_name] << " ";
                                        if(cmd_set[tmp_name] != "")
                                            tmp_name = cmd_set[tmp_name];
                                        else
                                            tmp_name = "";
                                    }
                                    cout << endl;

                                    names.push_back(tmp_name);

                                }

                                for(auto i : paths_lib)
                                {

                                    string tmp_path = i;
                                    while(tmp_path[0] == '$')
                                    {
                                        tmp_path.erase(0,1);
                                        tmp_path.erase(0,1);
                                        tmp_path.erase(tmp_path.size() - 1);
                                        //cout << tmp_path << " " << cmd_set[tmp_path] << " ";
                                        if(cmd_set[tmp_path] != "")
                                            tmp_path = cmd_set[tmp_path];
                                        else
                                            tmp_path = "";
                                    }
                                    cout << endl;
                                    if(tmp_path != "")
                                        paths.push_back(tmp_path);

                                }

                                /*for(auto i : names)
                                cout << i << " ";
                                cout << endl;

                                for(auto j : paths)
                                    cout << j << " ";
                                cout << endl; */
                                find_libraries(names, flag,paths);
                            }


                        }   
                    
                    }

                }

                in.close();
                fs::remove(p);
            }

            int run_command_trace(vector <string> cmd, bool need_admin_rights = false, bool first = false)
            {

                if (need_admin_rights && !is_admin())
                    cmd.insert(cmd.begin(), "sudo");
                int return_code = 0;
                if(first)
                {
                    fs::path curr_path = fs::current_path();
                    string pathtmp = "/tmp/archives/" + archive_name;
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
                    {
                        cerr << "Не удалось разветвить процесс\n";
                        exit(1);
                    }
                    case 0: //дочерний процесс
                    {
                        //stdout_pipe[1] запись 
                        //stdout_pipe[0] чтение
                    
                        char *args[cmd.size()+1];
                        for(int i = 0; i < cmd.size(); i++)
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
                if(first) trace();
                
                return return_code;

            }

            virtual void cmake_trace()
            {
                fs::path path_to_reply = build_dir;
                string link_directories;
                vector <string> link_dirs;

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

                for(auto i : link_dirs)
                    cout << i << endl;
                //cd(current_path);

                link_toolchain_dir = link_dirs;

                run_command_trace({"cmake", "--trace-format=json-v1", unpack_dir},false,true);


                //ptr_cmake_trace(unpack_dir,path_to_package,link_dirs, archive_name);

            }


    };

    class FreeBsd : public Unix
    {
        public:
            FreeBsd()
            {
                cerr << "FreeBSD!\n";
            }

            virtual int assembly_cmake()
            {
                installation({"bash"});
                Unix::cmake_trace(); 
                return Unix::assembly_cmake();
                
            }

            virtual int assembly_perl_build()
            {
                perl_package_build = "p5-App-cpanminus";
                Unix::installation({"perl5"});
                return Unix::assembly_perl_build();
            }

            virtual int assembly_perl_make()
            {
                Unix::installation({"perl5"});
                return Unix::assembly_perl_make();

            }

            virtual int assembly_php()
            {
                phpize = {"/usr/local/bin/phpize"};
                Unix::installation({"php83", "php83-pear", "php83-session", "php83-gd"});
                return Unix::assembly_php();
            }
        
            virtual void install_gems()
            {
                Unix::installation({"rubygem-grpc"});
                Unix::install_gems();
            }

            int run_command_1(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr) 
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
                        for(int i = 0; i < cmd.size(); i++)
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
                            
                            if(find_install_package_1(stdout_pipe)!=0)
                                return_code = 1;   
                            
                        }  
                        
                            
                        do {
                            waitpid(pid, &status, 0);
                        } while(!WIFEXITED(status) && !WIFSIGNALED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
                        int child_status;
                        if(WEXITSTATUS(status) == 0) child_status = 0;
                        else child_status = 1;
                        return_code = return_code || child_status; 
                        break;

                    }   
                }
                return return_code;
            }

            int find_install_package_1(int* stdout_pipe)
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
                    if(regex_search(line,reg1) && regex_search(line,reg2))
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

                        if(name_package_for_lib != "")
                        {
                            
                            found_package = true;
                            if(run_command_1({"pkg", "install", "-y", name_package_for_lib},true) == 0)
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

                if(!found_package)
                    cout << "Не удалось найти пакет, который предоставляет библиотеку/пакет " << libname << endl; //не прерывается программа ибо может собраться пакет и без этой библиотеки

                if(install) return 0;
                else return 1;
            }

            virtual void find_lib(vector<string> libs, vector<string> libsPath)
            {

                string path;
                string check_l;
                int mypipe[2];
                string so_lib,a_lib;
                string tmp_lib_name; 
                bool found;
                vector <fs::path> path_copy;
                int mypipe2[2];
                string nl = "";


                for(auto l : libs)
                {
                    found = false;
                    if(pipe(mypipe)) 
                    {
                        perror("Ошибка канала");
                        exit(1);
                    }
                    if(l[0] == '/') //если имя библиотеки начинается с /, то,значит, она написана с полным путем до нее
                    {
                        //cout << l << endl;
                        fs::path path_tmp(l);
                        string lib_name = path_tmp.filename(); //получаем имя библиотеки

                        libname = l;
                        if(fs::exists(l))
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
                        
                    if(l[0] == '-' && l[1] == 'l') //-lname = libname.so || libname.a
                    {
                        tmp_lib_name = l.substr(2); //убираем -l
                        so_lib = "lib" + tmp_lib_name + ".so";
                        a_lib = "lib" + tmp_lib_name + ".a"; 

                    }    
                    else if(l[0] == '-' && l[1] == 'p') //-pthread=-lpthread=libpthread.so || libpthread.a
                    {
                        tmp_lib_name = l.substr(1); //убираем -
                        so_lib = "lib" + tmp_lib_name + ".so";
                        a_lib = "lib" + tmp_lib_name + ".a";
                    }    
                    else //прямое подключение name.a || name.so
                    {
                        regex r1(".so");
                        regex r2(".a");
                        if(regex_search(l,r2)) //.a.****
                            a_lib = l;
                        else if(regex_search(l,r1)) //.so.***
                            so_lib = l;          
                    }

                    if(so_lib != "" || a_lib != "")
                    {
                        for(auto p : libsPath) //ищем по всем путям указанным
                        {
                            path = p;
                            check_l = "";
                            for(int i = 0; i < 2; i++)
                                check_l += p[i];
                            if(check_l == "-L")
                                path = p.substr(2, p.size() - 2); //путь к пути библиотеки (убираем -L, если передавали такие пути)
                            
                            int size = path.size();
                            if(path[size - 1] == '/') //удаляется последний / в пути, если он был (нормализация пути)
                            {
                                path.erase(size - 1);
                                size = path.size();
                            }

                            fs::path path_to_lib = path;

                            path_copy.push_back(path_to_lib);

                            if(so_lib != "" && a_lib != "") //для случая -lname -pname
                            {
                                libname = path_to_lib/so_lib;
                                nl = so_lib;
                                if(fs::exists(path_to_lib/so_lib))
                                {   
                                    nl = so_lib;
                                    libname = path_to_lib/so_lib;
                                    cout << "Библиотека " << libname << " найдена" << endl;
                                    string pathl = path_to_lib/so_lib;
                                    if(run_command_1({"pkg","provides", "^"+pathl}, false, mypipe) == 0)
                                        found = true;
                                }
                                else if(fs::exists(path_to_lib/a_lib))
                                {
                                    nl = a_lib;
                                    libname = path_to_lib/a_lib;
                                    cout << "Библиотека " << libname << " найдена" << endl;
                                    string pathl = path_to_lib/a_lib;
                                    if(run_command_1({"pkg","provides","^"+pathl}, false, mypipe) == 0)
                                        found = true;
                                
                                }

                            
                            }
                            else if(so_lib != "") //только .so
                            {
                                libname = path_to_lib/so_lib;
                                nl = so_lib;
                                if(fs::exists(path_to_lib/so_lib))
                                {
                                    cout << "Библиотека " << libname << " найдена" << endl;
                                    string pathl = path_to_lib/so_lib;
                                    if(run_command_1({"pkg","provides", "^"+pathl}, false, mypipe) == 0)
                                        found = true;
                                
                                }
                            }
                            else if(a_lib != "") //только .a
                            {

                                libname = path_to_lib/a_lib;
                                nl = a_lib;
                                if(fs::exists(path_to_lib/a_lib))
                                {
                                    cout << "Библиотека " << libname << " найдена" << endl;
                                    string pathl = path_to_lib/a_lib;
                                    if(run_command_1({"pkg","provides", "^"+pathl}, false, mypipe) == 0)
                                        found = true;
                                    
                                }    

                            }

                            if(found) break;
                        }
                    }
                    else
                    {
                        cerr << "Некорректное имя библиотеки\n";
                        exit(1);
                    }

                    if(!found) //библиотека не нашлась ни по одному из путей
                    {
                        //попытка найти пакет и установить библиотеку ,если она не нашлась(только для тех бибилотек, у которых не полный путь)
                        cout << "Библиотека " << l << " не найдена ни по одному из указанных путей" << endl;
                        cout << "Поиск пакета, который предоставляет необходимую библиотеку по одному из путей" << endl;
                        for(auto i: path_copy)
                        {
                            libname = i/nl;
                            pipe(mypipe2);
                            if(run_command_1({"pkg", "provides", "^" + libname}, false, mypipe2) == 0)
                                break;
                        }
                    } 
                }
            
            }

            int find_install_package_for_lib(fs::path path, string name_so, string name_a, bool *found_lib = nullptr, bool library_not_found_anywhere = false)
            {
                libname = path/name_so;
                int mypipe[2];
                int return_code = 0;
                if(pipe(mypipe))
                {
                    perror("Ошибка канала");
                    exit(1);
                }

                if(library_not_found_anywhere)
                {
                    return_code = run_command_1({"pkg", "provides", "^" + libname}, false, mypipe);
                    if(return_code) 
                    {
                        pipe(mypipe);
                        libname = path/name_a;
                        return_code = run_command_1({"pkg", "provides", "^" + libname}, false, mypipe);
                    }
                    return return_code;

                }
                if(fs::exists(path/name_so))
                {
                    if(found_lib)
                        *found_lib = true;                      
                    cout << "Библиотека " << path/name_so << " найдена" << endl;
                    run_command_1({"pkg", "provides", "^", libname}, false, mypipe);

                }
                else
                {
                    cout << "Библиотека " << path/name_so << " не найдена" << endl;

                    libname = path/name_a;
                    if(pipe(mypipe))
                    {   
                        perror("Ошибка канала");
                        exit(1);
                    }

                    if(fs::exists(path/name_a))
                    {
                        if(found_lib)
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

            virtual void find_packages(list <vector <string> > names)
            {
                int mypipe[2];
                bool inst = false;
                for(vector<string> elem : names)
                {
                    if(inst) break;
                    for(string i : elem)
                    {
                        if(pipe(mypipe))
                        {
                            perror("Ошибка канала");
                            exit(1);

                        }
                        if(i.substr(0,3) != "lib" || (i.substr(0,3) == "lib" && i.find("dev") != i.npos)) //не библиотека
                        {
                            cout << "Установка пакета " << i << endl;
                            libname = i;

                            if(run_command_trace({"pkg","install", i},true)  == 0)
                            {
                                cout << "Пакет " << i << " установлен" << endl;
                                inst = true;
                                break;

                            }
                                        

                        }
                        else //библиотека
                        {
                            vector <fs::path> standart_path;
                            vector <string> libs = elem;
                            for(auto p : link_toolchain_dir)
                                standart_path.push_back(p);
                            bool lib_install = false;
                            for(auto l : libs)
                            {
                                for(auto path : standart_path)
                                    if(fs::exists(path/l))
                                    {
                                        cout << "Библиотека " << path/l << "найдена" << endl;
                                        libname = path/l;
                                        if(run_command_1({"pkg", "provides", "^" + libname}, false, mypipe) == 0)
                                        {
                                            lib_install = true;
                                            inst = true;
                                            break;
                                        }                                 
                                    }
                                        if(lib_install) break;
                            }

                            if(!lib_install) //ни одна из библиотек не установлена, ищем любую и доставляем
                            {
                                for(auto l : libs)
                                {
                                    bool exit_found = false;
                                    cout << "Библиотека " << l << " нигде не найдена" << endl;
                                    for(auto path : standart_path)
                                    {
                                        libname = path/l;
                                        pipe(mypipe);
                                        if(run_command_1({"pkg", "provides", "^" + libname}, false, mypipe) == 0)
                                        {
                                            inst = true;
                                            exit_found = true;
                                            break;

                                        }                             
                                    }
                                    if(exit_found) break;
                                            
                                }

                            }

                            break;

                        }
                                
                    } 
                }

            }

            virtual void find_libraries(vector<string> names, bool flag, vector <fs::path> paths)
            {
                for(auto i : names)
                {
                    if(i != "")
                    {
                        string l_name_so = "lib" + i + ".so";
                        string l_name_a = "lib" + i + ".a"; 
                        if(flag) //не использовать дефолтные пути
                        {
                            bool found_lib = false;
                            if(paths.size() != 0)
                            {
                                cout << "Поиск не осуществляется в дефолтных путях\n";
                                for(auto j : paths)
                                    if(find_install_package_for_lib(j, l_name_so, l_name_a, &found_lib) == 0)
                                        break;
                                if(!found_lib)
                                {
                                    cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                                    for(auto j : paths)
                                        if(find_install_package_for_lib(j, l_name_so, l_name_a, NULL, true) == 0)
                                            break;

                                }
                            }

                            else cout << "Нет путей (PATHS/HINTS) для поиска библиотеки " << i << endl;

                        }

                        else
                        {
                            cout << "Поиск осуществляется в дефолтных путях и в указанных дополнительно, если такие есть\n";
                            regex mask1(".*(toolchains).*",regex_constants::icase);
                            vector <fs::path> toolchainpath;
                            for(auto p : link_toolchain_dir)
                                toolchainpath.push_back(p);
                            vector <fs::path> all_paths;
                            all_paths = toolchainpath;

                            for(const auto& element : paths)
                                if(element != "") all_paths.push_back(element);

                            bool found_lib = false;
                            for(auto j : all_paths)
                            {
                                if(find_install_package_for_lib(j, l_name_so, l_name_a, &found_lib) == 0)
                                    break;

                            }

                            if(!found_lib)
                            {
                                cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                                for(auto j : all_paths)
                                    if(find_install_package_for_lib(j, l_name_so, l_name_a, NULL, true) == 0)
                                        break;

                            }

                        }


                    }


                }

            }


    };

    class Linux : public Unix
    {
        public:
            string os_name(string path)
            {
                ifstream in(path);
                string os_name;
                if(in.is_open())
                {
                    regex r("^(NAME=).*");
                
                    while(getline(in,os_name))
                    {
                        if(regex_match(os_name,r) == true)
                        {
                            break;
                        }
                    }
                }

                in.close();

                return os_name;
            }
            

    };

    class OpenSuse : public Linux
    {
        public:
            OpenSuse()
            {
                cerr << "OpenSuse!\n";
            }

            virtual int assembly_perl_build()
            {
                perl_package_build = "perl-App-cpanminus";
                Unix::installation({"perl"});
                return Unix::assembly_perl_build();
            }

            virtual int assembly_perl_make()
            {
                Unix::installation({"perl"});
                perl_package_make = "libtirpc-devel";
                Unix::installation({perl_package_make});
                return Unix::assembly_perl_make();

            }

            virtual int assembly_php()
            {
                Unix::installation({"php7", "php7-devel", "php7-pecl", "php7-pear"});
                phpize = {"/usr/bin/phpize"};
                return Unix::assembly_php();;
            }

            virtual int assembly_cmake()
            {
                Unix::cmake_trace(); 
                return Unix::assembly_cmake();
                
            }

            virtual void install_gems()
            {
                Unix::installation({"ruby-devel"});
                Unix::install_gems();
            }

            int run_command_2(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, string *path = nullptr)
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
                        for(int i = 0; i < cmd.size(); i++)
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
                                if(path)
                                {
                                    *path = line;
                                    found = true;
                                }
                                else
                                {
                                    tmppath = line;
                                    tmppath = tmppath.substr(0, size_path_lib);
                                    if(tmppath == libname) found = true;
                            
                                }
                                
                            }
                            free(line);
                        }

                        if(!found)
                            return_code = 1;

                            
                        do {
                            waitpid(pid, &status, 0);
                        } while(!WIFEXITED(status) && !WIFSIGNALED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
                        int child_status;
                        if(WEXITSTATUS(status) == 0) child_status = 0;
                        else child_status = 1;
                        return_code = return_code || child_status; 
                        break;
                    }   
                }
                return return_code;

            }

            int find_install_package_2(int* stdout_pipe)
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
                    if(regex_search(line,reg1))
                    {
                        
                        string copy_line = line;
                        int find_ind = copy_line.find(".rpm");
                        int ind_prob;
                        for(int i = find_ind; 0<=i; i--) //от .rpm до начала в поисках первого пробела, этот пробел между именем пакета и текстом вывода
                            if(line[i] == ' ')
                            {
                                ind_prob = i;
                                break;

                            }
                                
                        for(int j = ind_prob + 1; j < find_ind + 4; j++)
                            name_package_for_lib+=line[j];
                        if(name_package_for_lib[0] == ' ')
                            name_package_for_lib.erase(0,1);
                        int size = name_package_for_lib.size();
                        while(name_package_for_lib[size - 1] == ' ')
                        {
                            name_package_for_lib.erase(size - 1);
                            size = name_package_for_lib.size();
                        }

                        cout << name_package_for_lib << endl;
                                    
                        if(name_package_for_lib != "")
                        {
                            int mypipe3[2];
                            if(pipe(mypipe3))
                            {
                                perror("Ошибка канала");
                                exit(1);
                            }
                            run_command_2({"find", "/var/cache", "-type", "f", "-name", name_package_for_lib},true,mypipe3,&path_to_rpm_package);
                            cout << path_to_rpm_package;
                            int mypipe_4[2];
                            if(pipe(mypipe_4))
                            {
                                perror("Ошибка канала");
                                exit(1);

                            }

                            int f = path_to_rpm_package.find("\n");
                            path_to_rpm_package.erase(f);

                
                            if(run_command_2({"rpm", "-ql", path_to_rpm_package},false,mypipe_4) == 0)
                            {
                                found = true; 
                                break;
                            }
                        }
                                                                
                    }
                }

                //высвобождение буфера
                free(line);

                if(found) return 0;
                else return 1;

            }

            int run_command_1(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool not_found_full_path_lib = false, bool checking_existence_lib = false) 
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
                        for(int i = 0; i < cmd.size(); i++)
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
                            if(!checking_existence_lib) //не требуется проверка предоставлении бибилотеки при скачиваниии без установки
                            {
                                if(!not_found_full_path_lib) //Библиотека установлена, ищется пакет, из которого она ставилась
                                    find_install_package_1(stdout_pipe,1);   
                                else
                                    if(find_install_package_1(stdout_pipe,2) != 0) //поиск пакета для библиотеки, которая не установлена, с конкретно заданым путем
                                        return_code = 1;
                            }
                            else //требуется проверка предоставлении бибилотеки при скачиваниии без установки
                            { 
                                if(find_install_package_2(stdout_pipe) != 0)
                                    return_code = 1;

                            }    
                        }
                            
                        do {
                            waitpid(pid, &status, 0);
                        } while(!WIFEXITED(status) && !WIFSIGNALED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
                        int child_status;
                        if(WEXITSTATUS(status) == 0) child_status = 0;
                        else child_status = 1;
                        return_code = return_code || child_status; 
                        break;

                    }   
                }

                return return_code;
            }

            int find_install_package_1(int* stdout_pipe, int code_funct)
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
                    if(line[0] != 'S' && count(line, line + linelen, '|') == 3)
                    {
                        int c = 0;
                        int ind1 = 0;
                        int ind2 = 0;
                        
                                        
                        for(int i = 0; i < linelen; i++)
                        {
                            if(line[i] == '|' && (ind1 == 0 || ind2 == 0))
                            {
                                c++;
                                if(c==1) ind1 = i;
                                else if(c==2) ind2 = i;
                            }
                        }

                        for(int j = ind1 + 1; j < ind2; j++)
                            name_package_for_lib+=line[j];
                        if(name_package_for_lib[0] == ' ')
                            name_package_for_lib.erase(0,1);
                        int size = name_package_for_lib.size();
                        while(name_package_for_lib[size - 1] == ' ')
                        {
                            name_package_for_lib.erase(size - 1);
                            size = name_package_for_lib.size();
                        }

                        cout << name_package_for_lib << endl;

                        if(code_funct == 1)
                        {
                            if(name_package_for_lib != "")
                            {
                                found_package = true;
                                if(run_command_1({"zypper", "install", "-y", name_package_for_lib},true) == 0)
                                {
                                    install = true;
                                    break; //пока ставится первый найденный пакет
                                }
                                else cout << "Не удалось установить предоставляющий библиотеку пакет " << name_package_for_lib << endl;
                                
                            }
                        }

                        if(code_funct == 2)  
                        {
                            if(name_package_for_lib != "")
                            {
                                int mypipe2[2];
                                if(pipe(mypipe2))
                                {
                                    perror("Ошибка канала");
                                    exit(1);
                                }
                                if(run_command_1({"zypper", "install", "--download-only", "-y", name_package_for_lib},true,mypipe2,true,true) == 0)
                                {
                                    found_package = true;
                                    if(run_command_1({"zypper", "install", "-y", name_package_for_lib},true) == 0)
                                    {
                                        install = true;
                                        break;
                                    }
                                    else cout << "Не удалось установить предоставляющий библиотеку пакет " << name_package_for_lib << endl;

                                }
                            }
                        }      
                        
                                                                
                    }
                }

                //высвобождение буфера
                free(line);

                if(!found_package)
                    cout << "Не удалось найти пакет, который предоставляет библиотеку/пакет " << libname << endl; //не прерывается программа ибо может собраться пакет и без этой библиотеки

                if(install) return 0;
                else return 1;
            }

            virtual void find_lib(vector<string> libs, vector<string> libsPath)
            {
                string path;
                string check_l;
                int mypipe[2];
                int mypipe2[2];
                string so_lib,a_lib;
                string tmp_lib_name; 
                bool found;

                vector <fs::path> path_copy;
                string nl = "";

                for(auto l : libs)
                {
                    found = false;
                    if(pipe(mypipe)) 
                    {
                        perror("Ошибка канала");
                        exit(1);
                    }
                    if(l[0] == '/') //если имя библиотеки начинается с /, то,значит, она написана с полным путем до нее
                    {
                        //cout << l << endl;
                        fs::path path_tmp(l);
                        string lib_name = path_tmp.filename(); //получаем имя библиотеки

                        libname = l;
                        if(fs::exists(path_tmp))
                        {
                            cout << "Библиотека " << l << " найдена" << endl;
                            run_command_1({"zypper", "se", "--provides", l}, false, mypipe);
                        }
                        else
                        {
                            cout << "Библиотека " << l << " не найдена" << endl;
                            if(run_command_1({"zypper", "se", "--provides", lib_name}, false, mypipe, true) != 0)
                                cout << "Не удалось найти пакет, который предоставляет библиотек/пакет " << l << endl;               
                        }

                        continue;
                    
                    }

                    so_lib = "";
                    a_lib = "";
                    tmp_lib_name = "";
                    libname = "";
                        
                    if(l[0] == '-' && l[1] == 'l') //-lname = libname.so || libname.a
                    {
                        tmp_lib_name = l.substr(2); //убираем -l
                        so_lib = "lib" + tmp_lib_name + ".so";
                        a_lib = "lib" + tmp_lib_name + ".a"; 

                    }    
                    else if(l[0] == '-' && l[1] == 'p') //-pthread=-lpthread=libpthread.so || libpthread.a
                    {
                        tmp_lib_name = l.substr(1); //убираем -
                        so_lib = "lib" + tmp_lib_name + ".so";
                        a_lib = "lib" + tmp_lib_name + ".a";
                    }    
                    else //прямое подключение name.a || name.so
                    {
                        regex r1(".so");
                        regex r2(".a");
                        if(regex_search(l,r2)) //.a.****
                            a_lib = l;
                        else if(regex_search(l,r1)) //.so.****
                            so_lib = l;          
                    }

                    if(so_lib != "" || a_lib != "")
                    {
                        for(auto p : libsPath) //ищем по всем путям указанным
                        {
                            path = p;
                            check_l = "";
                            for(int i = 0; i < 2; i++)
                                check_l += p[i];
                            if(check_l == "-L")
                                path = p.substr(2, p.size() - 2); //путь к пути библиотеки (убираем -L, если передавали такие пути)
                            
                            int size = path.size();
                            if(path[size - 1] == '/') //удаляется последний / в пути, если он был (нормализация пути)
                            {
                                path.erase(size - 1);
                                size = path.size();
                            }

                            fs::path path_to_lib = path;

                            path_copy.push_back(path_to_lib);

                            if(so_lib != "" && a_lib != "") //для случая -lname -pname
                            {
                                libname = path_to_lib/so_lib;
                                nl = so_lib;
                                if(fs::exists(path_to_lib/so_lib))
                                {   
                                    nl = so_lib;
                                    libname = path_to_lib/so_lib;
                                    cout << "Библиотека " << libname << " найдена" << endl;
                                    if(run_command_1({"zypper", "se", "--provides", path_to_lib/so_lib}, false, mypipe) == 0)
                                        found = true;
                                }
                                else if(fs::exists(path_to_lib/a_lib))
                                {
                                    nl = a_lib;
                                    libname = path_to_lib/a_lib;
                                    cout << "Библиотека " << libname << " найдена" << endl;
                                    if(run_command_1({"zypper", "se", "--provides", path_to_lib/a_lib}, false, mypipe) == 0)
                                        found = true;
                                
                                }

                            
                            }
                            else if(so_lib != "") //только .so
                            {
                                libname = path_to_lib/so_lib;
                                nl = so_lib;
                                if(fs::exists(path_to_lib/so_lib))
                                {
                                    cout << "Библиотека " << libname << " найдена" << endl;
                                    if(run_command_1({"zypper", "se", "--provides", path_to_lib/so_lib}, false, mypipe) == 0)
                                        found = true;
                                
                                }
                            }
                            else if(a_lib != "") //только .a
                            {
                                libname = path_to_lib/a_lib;
                                nl = a_lib;
                                if(fs::exists(path_to_lib/a_lib))
                                {
                                    cout << "Библиотека " << libname << " найдена" << endl;
                                    if(run_command_1({"zypper", "se", "--provides", path_to_lib/a_lib}, false, mypipe) == 0)
                                        found = true;
                                    
                                }    

                            }

                            if(found) break;
                        }
                    }
                    else
                    {
                        cerr << "Некорректное имя библиотеки\n";
                        exit(1);
                    }

                    if(!found) //библиотека не нашлась ни по одному из путей
                    {
                        cout << "Библиотека " << l << " не найдена ни по одному из указанных путей" << endl;
                        cout << "Поиск пакета, который предоставляет необходимую библиотеку по одному из путей" << endl;
                        for(auto i: path_copy)
                        {
                            libname = i/nl;
                            pipe(mypipe2);
                            if(run_command_1({"zypper", "se", "--provides", nl}, false, mypipe2,true) == 0)
                                break;
                        //попытка найти пакет и установить библиотеку 
                        }
                        
                    }      
                }
            }

            int find_install_package_for_lib(fs::path path, string name_so, string name_a, bool *found_lib = nullptr, bool library_not_found_anywhere = false)
            {
                libname = path/name_so;
                int return_code = 0;
                int mypipe[2];
                if(pipe(mypipe))
                {
                    perror("Ошибка канала");
                    exit(1);
                }
                if(library_not_found_anywhere)
                {
                    return_code = run_command_1({"zypper", "se", "--provides", name_so}, false, mypipe, true);
                    if(return_code) 
                    {
                        pipe(mypipe);
                        libname = path/name_a;
                        return_code = run_command_1({"zypper", "se", "--provides", name_a}, false, mypipe, true);
                    }
                    return return_code;

                }
                if(fs::exists(path/name_so))
                {
                    if(found_lib)
                        *found_lib = true;                      
                    cout << "Библиотека " << path/name_so << " найдена" << endl;
                    run_command_1({"zypper", "se", "--provides", path/name_so}, false, mypipe);

                }
                else
                {
                    cout << "Библиотека " << path/name_so << " не найдена" << endl;
                    

                    libname = path/name_a;
                    if(pipe(mypipe))
                    {   
                        perror("Ошибка канала");
                        exit(1);
                    }

                    if(fs::exists(path/name_a))
                    {
                        if(found_lib)
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

            virtual void find_packages(list <vector <string> > names)
            {
                bool inst = false;
                int mypipe[2];
                for(vector<string> elem : names)
                {
                    if(inst) break;
                    for(string i : elem)
                    {
                        if(pipe(mypipe))
                        {
                            perror("Ошибка канала");
                            exit(1);

                        }
                        if(i.substr(0,3) != "lib" || (i.substr(0,3) == "lib" && i.find("dev") != i.npos)) //не библиотека
                        {
                            cout << "Установка пакета " << i << endl;
                            libname = i;

                            if(run_command_trace({"zypper","install", i},true) == 0)
                            {
                                cout << "Пакет " << i << " установлен" << endl;
                                inst = true;
                                break;

                            }
                                                    

                        }
                        else //библиотека
                        {
                            vector <fs::path> standart_path;
                            vector <string> libs = elem;
                            for(auto p : link_toolchain_dir)
                                standart_path.push_back(p);
                            bool lib_install = false;
                            for(auto l : libs)
                            {
                                for(auto path : standart_path)
                                    if(fs::exists(path/l))
                                    {
                                        libname = path/l;
                                        cout << "Библиотека " << path/l << "найдена" << endl;
                                        if(run_command_1({"zypper", "se", "--provides", path/l}, false, mypipe) == 0)
                                        {
                                            lib_install = true;
                                            inst = true;
                                            break;
                                        }                                 
                                    }
                                if(lib_install) break;
                            }

                            if(!lib_install) //ни одна из библиотек не установлена, ищем любую и доставляем
                            {
                                for(auto l : libs)
                                {
                                    bool exit_found = false;
                                    cout << "Библиотека " << l << " нигде не найдена" << endl;
                                    for(auto path : standart_path)
                                    {
                                        libname = path/l;
                                        pipe(mypipe);
                                        if(run_command_1({"zypper", "se", "--provides", l}, false, mypipe, true) == 0)
                                        {
                                            inst = true;
                                            exit_found = true;
                                            break;

                                        }                             
                                    }
                                    if(exit_found) break;
                                                        
                                }

                            }

                            break;

                        }
                    }
                                        
                } 
                
            }

            virtual void find_libraries(vector <string> names, bool flag, vector <fs::path> paths)
            {
                for(auto i : names)
                {
                    if(i != "")
                    {
                        string l_name_so = "lib" + i + ".so";
                        string l_name_a = "lib" + i + ".a"; 
                                        
                        if(flag) //не использовать дефолтные пути, указана опция NO_DEFAULT_PATH
                        {   
                            bool found_lib = false;
                            if(paths.size() != 0)
                            {
                                cout << "Поиск не осуществляется в дефолтных путях\n";
                                for(auto j : paths)
                                    if(find_install_package_for_lib(j, l_name_so, l_name_a, &found_lib) == 0)
                                        break;
                                if(!found_lib) //нигде не найдена библиотека 
                                {
                                    cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                                    for(auto j : paths)
                                        if(find_install_package_for_lib(j, l_name_so, l_name_a, NULL, true) == 0)
                                            break;

                                }
                            }

                            else cout << "Нет путей (PATHS/HINTS) для поиска библиотеки " << i << endl;

                        }

                        else
                        {
                            cout << "Поиск осуществляется в дефолтных путях и в указанных дополнительно, если такие есть\n";
                            regex mask1(".*(toolchains).*",regex_constants::icase);
                            vector <fs::path> toolchainpath;
                            for(auto p : link_toolchain_dir)
                                toolchainpath.push_back(p);
                            vector <fs::path> all_paths;
                            all_paths = toolchainpath;

                            for(const auto& element : paths)
                                if(element != "") all_paths.push_back(element);

                            bool found_lib = false;
                            for(auto j : all_paths)
                            {
                                if(find_install_package_for_lib(j, l_name_so, l_name_a, &found_lib) == 0)
                                    break;
                            }

                            if(!found_lib) //нигде не найдена библиотека 
                            {
                                cout << "Библиотека " << i << " не найдена ни в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                                for(auto j : all_paths)
                                    if(find_install_package_for_lib(j, l_name_so, l_name_a, NULL, true) == 0)
                                        break;

                            }           

                        }


                    }

                }

            }

    };

    class Ubuntu : public Linux
    {
        public:
            Ubuntu()
            {
                cerr << "Ubuntu!\n";
            }

            virtual int assembly_cmake()
            {
                //installation({"libncurses-dev", "libreadline-dev", "libbsd-dev"});
                Unix::cmake_trace(); 
                return Unix::assembly_cmake();
                
            }

            virtual int assembly_perl_build()
            {

                perl_package_build = "cpanminus";
                Unix::installation({"perl"});
                return Unix::assembly_perl_build();
            }

            virtual int assembly_perl_make()
            {
                Unix::installation({"perl"});
                perl_package_make = "libtirpc-dev";
                Unix::installation({perl_package_make});
                return Unix::assembly_perl_make();

            }

            virtual int assembly_php()
            {
                phpize = {"/usr/bin/phpize"};
                Unix::installation({"php", "php-dev", "php-pear"});
                return Unix::assembly_php();

            }

            virtual void install_gems()
            {
                Unix::installation({"ruby-dev"});
                Unix::install_gems();
            }

            virtual void find_lib(vector<string> lib, vector<string> libsPath)
            {
                string path;
                string check_l;
                int mypipe[2];
                int mypipe2[2];
                string so_lib,a_lib;
                string tmp_lib_name; 
                bool found;
                vector <fs::path> path_copy;
                string nl = "";

                for(auto l : lib)
                {
                    found = false;
                    if(pipe(mypipe)) 
                    {
                        perror("Ошибка канала");
                        exit(1);
                    }
                    if(l[0] == '/') //если имя библиотеки начинается с /, то,значит, она написана с полным путем до нее
                    {
                        //cout << l << endl;
                        fs::path path_tmp(l);
                        string lib_name = path_tmp.filename(); //получаем имя библиотеки

                        libname = l;

                        if(fs::exists(l))
                            cout << "Библиотека " << l << " найдена" << endl;
                        else
                        {
                            cout << "Библиотека " << l << " не найдена" << endl;
                            cout << "Поиск пакета, который предоставляет необходимую библиотеку" << endl;
                        }

                        run_command_1({"apt-file", "-F", "search", l}, false, mypipe);         
                        continue;
                    
                    }

                    so_lib = "";
                    a_lib = "";
                    tmp_lib_name = "";
                        
                    if(l[0] == '-' && l[1] == 'l') //-lname = libname.so || libname.a
                    {
                        tmp_lib_name = l.substr(2); //убираем -l
                        so_lib = "lib" + tmp_lib_name + ".so";
                        a_lib = "lib" + tmp_lib_name + ".a"; 

                    }    
                    else if(l[0] == '-' && l[1] == 'p') //-pthread=-lpthread=libpthread.so || libpthread.a
                    {
                        tmp_lib_name = l.substr(1); //убираем -
                        so_lib = "lib" + tmp_lib_name + ".so";
                        a_lib = "lib" + tmp_lib_name + ".a";
                    }    
                    else //прямое подключение name.a || name.so
                    {
                        regex r1(".so");
                        regex r2(".a");
                        if(regex_search(l,r2)) //.a.***
                            a_lib = l;
                        else if(regex_search(l,r1)) //.so.***
                            so_lib = l;          
                    }

                    if(so_lib != "" || a_lib != "")
                    {
                        for(auto p : libsPath) //ищем по всем путям указанным
                        {
                            path = p;
                            check_l = "";
                            for(int i = 0; i < 2; i++)
                                check_l += p[i];
                            if(check_l == "-L")
                                path = p.substr(2, p.size() - 2); //путь к пути библиотеки (убираем -L, если передавали такие пути)
                            
                            int size = path.size();
                            if(path[size - 1] == '/') //удаляется последний / в пути, если он был (нормализация пути)
                            {
                                path.erase(size - 1);
                                size = path.size();
                            }

                            fs::path path_to_lib = path;

                            path_copy.push_back(path_to_lib);

                            if(so_lib != "" && a_lib != "") //для случая -lname -pname
                            {
                                libname = path_to_lib/so_lib;
                                nl = so_lib;
                                if(fs::exists(path_to_lib/so_lib))
                                {   
                                    libname = path_to_lib/so_lib;
                                    cout << "Библиотека " << libname << " найдена" << endl;
                                    if(run_command_1({"apt-file", "-F",  "search", path_to_lib/so_lib}, false, mypipe) == 0)
                                        found = true;
                                }
                                else if(fs::exists(path_to_lib/a_lib))
                                {
                                    nl = a_lib;
                                    libname = path_to_lib/a_lib;
                                    cout << "Библиотека " << libname << " найдена" << endl;
                                    if(run_command_1({"apt-file", "-F", "search", path_to_lib/a_lib}, false, mypipe) == 0)
                                        found = true;
                                
                                }

                            
                            }
                            else if(so_lib != "") //только .so
                            {
                                libname = path_to_lib/so_lib;
                                nl = so_lib;
                                if(fs::exists(path_to_lib/so_lib))
                                {
                                    cout << "Библиотека " << libname << " найдена" << endl;
                                    if(run_command_1({"apt-file", "-F", "search", path_to_lib/so_lib}, false, mypipe) == 0)
                                        found = true;
                                
                                }
                            }
                            else if(a_lib != "") //только .a
                            {

                                libname = path_to_lib/a_lib;
                                nl = a_lib;
                                if(fs::exists(path_to_lib/a_lib))
                                {
                                    cout << "Библиотека " << libname << " найдена" << endl;
                                    if(run_command_1({"apt-file", "-F", "search", path_to_lib/a_lib}, false, mypipe) == 0)
                                        found = true;
                                    
                                }    

                            }

                            if(found) break;
                        }
                    }
                    else
                    {
                        cerr << "Некорректное имя библиотеки\n";
                        exit(1);
                    }

                    if(!found) //библиотека не нашлась ни по одному из путей
                    {
                        cout << "Библиотека " << l << " не найдена ни по одному из указанных путей" << endl;
                        cout << "Поиск пакета, который предоставляет необходимую библиотеку по одному из путей" << endl;
                        for(auto i: path_copy)
                        {
                            libname = i/nl;
                            pipe(mypipe2);
                            if(run_command_1({"apt-file", "-F", "search", i/nl}, false, mypipe2) == 0)
                                break;
                        //попытка найти пакет и установить библиотеку 
                        }
                    }
                }

            }

            int find_install_package_for_lib(fs::path path, string name_so, string name_a, bool *found_lib = nullptr, bool library_not_found_anywhere = false)
            {
                libname = path/name_so;
                int mypipe[2];
                int return_code = 0;
                if(pipe(mypipe))
                {
                    perror("Ошибка канала");
                    exit(1);
                }

                if(library_not_found_anywhere)
                {
                    string reg_path;
                    reg_path = "^" + libname;
                    return_code = run_command_1({"apt-file", "-F", "search", reg_path}, false, mypipe);
                    if(return_code) 
                    {
                        pipe(mypipe);
                        libname = path/name_a;
                        reg_path = "^" + libname;
                        return_code = run_command_1({"apt-file", "-F", "search", reg_path}, false, mypipe);
                    }
                    return return_code;

                }
                if(fs::exists(path/name_so))
                {
                    if(found_lib)
                        *found_lib = true;                      
                    cout << "Библиотека " << path/name_so << " найдена" << endl;
                    string reg_path = path/name_so;
                    reg_path = "^" + reg_path;
                    run_command_1({"apt-file", "-F", "search", reg_path}, false, mypipe);

                }
                else
                {
                    cout << "Библиотека " << path/name_so << " не найдена" << endl;

                    libname = path/name_a;
                    if(pipe(mypipe))
                    {   
                        perror("Ошибка канала");
                        exit(1);
                    }

                    if(fs::exists(path/name_a))
                    {
                        if(found_lib)
                            *found_lib = true;  
                        string reg_path = path/name_so;
                        reg_path = "^" + reg_path;                                 
                        cout << "Библиотека " << path/name_a << " найдена" << endl;
                        run_command_1({"apt-file", "-F", "search", reg_path}, false, mypipe);
                    }
                    else
                    {
                        cout << "Библиотека " << path/name_a << " не найдена" << endl;
                        return_code = 1;

                    }

                }

                return return_code;

            }

            virtual void find_packages(list <vector <string> > names)
            {
                int mypipe[2];
                bool inst = false;
                for(vector<string> elem : names)
                {
                    if(inst) break;
                    for(string i : elem)
                    {
                        if(pipe(mypipe))
                        {
                            perror("Ошибка канала");
                            exit(1);

                        }
                        if(i.substr(0,3) != "lib" || (i.substr(0,3) == "lib" && i.find("dev") != i.npos)) //не библиотека 
                        {
                            cout << "Установка пакета " << i << endl;
                            libname = i;

                            if(run_command_trace({"apt","install", "-y", i},true) == 0)
                            {
                                cout << "Пакет " << i << " установлен" << endl;
                                inst = true;
                                break;

                            }
                                        

                        }
                        else //библиотека
                        {
                            vector <fs::path> standart_path;
                            vector <string> libs = elem;
                            for(auto p : link_toolchain_dir)
                                standart_path.push_back(p);
                            bool lib_install = false;
                            for(auto l : libs)
                            {
                                for(auto path : standart_path)
                                    if(fs::exists(path/l))
                                    {
                                        cout << "Библиотека " << path/l << "найдена" << endl;
                                        libname = path/l;
                                        string reg_path = path/l;
                                        reg_path = "^" + reg_path;
                                        if(run_command_1({"apt-file", "-F", "search", reg_path}, false, mypipe) == 0)
                                        {
                                            lib_install = true;
                                            inst = true;
                                            break;
                                        }                                 
                                    }
                                if(lib_install) break;
                            }

                            if(!lib_install) //ни одна из библиотек не установлена, ищем любую и доставляем
                            {
                                for(auto l : libs)
                                {
                                    bool exit_found = false;
                                    cout << "Библиотека " << l << " нигде не найдена" << endl;
                                    for(auto path : standart_path)
                                    {
                                        libname = path/l;
                                        string reg_path;
                                        reg_path = "^" + libname;
                                        pipe(mypipe);
                                        if(run_command_1({"apt-file", "-F", "search", reg_path}, false, mypipe) == 0)
                                        {
                                            inst = true;
                                            exit_found = true;
                                            break;

                                        }                             
                                    }
                                    if(exit_found) break;
                                            
                                }

                            }

                            break;

                        }
                            
                    } 
                }
                

            }

            int run_command_1(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr) 
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
                        for(int i = 0; i < cmd.size(); i++)
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
                            
                            if(find_install_package_1(stdout_pipe)!=0)
                                return_code = 1;   
                            
                        }  
                        
                            
                        do {
                            waitpid(pid, &status, 0);
                        } while(!WIFEXITED(status) && !WIFSIGNALED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
                        int child_status;
                        if(WEXITSTATUS(status) == 0) child_status = 0;
                        else child_status = 1;
                        return_code = return_code || child_status; 
                        break;

                    }   
                }

                return return_code;
            }

            int find_install_package_1(int* stdout_pipe)
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

                    name_package_for_lib = line;

                    int find_del = name_package_for_lib.find(":");

                    name_package_for_lib = name_package_for_lib.substr(0, find_del);

                    cout << name_package_for_lib << endl;

                    if(name_package_for_lib != "" && name_package_for_lib != "libeditreadline-dev") //второе условие —  полученно вручную и нужно для рещения конфликта в реализациях readline
                    {
                        
                        found_package = true;
                        if(run_command_1({"apt", "install", "-y", name_package_for_lib},true) == 0)
                        {    
                            install = true;
                            break; //пока ставится первый найденный пакет
                        }
                        else cout << "Не удалось установить предоставляющий библиотеку пакет " << name_package_for_lib << endl;
                            
                    }

                    //this file not installed on the system
                    //apt-file search /usr/include/fftw3.h

                    //libfftw3-dev: /usr/include/fftw3.h 
                    //the search is also carried out not in installed packages                                       
                    
                }

                //высвобождение буфера
                free(line);

                if(!found_package)
                    cout << "Не удалось найти пакет, который предоставляет библиотеку/пакет " << libname << endl; //не прерывается программа ибо может собраться пакет и без этой библиотеки

                if(install) return 0;
                else return 1;
            }

            virtual void find_libraries(vector <string> names, bool flag, vector <fs::path> paths)
            {
                for(auto i : names)
                {
                    if(i != "")
                    {
                        string l_name_so = "lib" + i + ".so";
                        string l_name_a = "lib" + i + ".a"; 
                        if(flag) //не использовать дефолтные пути
                        {
                            bool found_lib = false;
                            if(paths.size() != 0)
                            {
                                cout << "Поиск не осуществляется в дефолтных путях\n";
                                for(auto j : paths)
                                    if(find_install_package_for_lib(j, l_name_so, l_name_a, &found_lib) == 0)
                                        break;
                                if(!found_lib)
                                {
                                    cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                                    for(auto j : paths)
                                        if(find_install_package_for_lib(j, l_name_so, l_name_a, NULL, true) == 0)
                                            break;
                                }
                            }

                            else cout << "Нет путей (PATHS/HINTS) для поиска библиотеки " << i << endl;
                                    
                        }

                        else
                        {
                            cout << "Поиск осуществляется в дефолтных путях и в указанных дополнительно, если такие есть\n";
                            regex mask1(".*(toolchains).*",regex_constants::icase);
                            vector <fs::path> toolchainpath;
                            for(auto p : link_toolchain_dir)
                                toolchainpath.push_back(p);
                            vector <fs::path> all_paths;
                            all_paths = toolchainpath;

                            for(const auto& element : paths)
                                if(element != "") all_paths.push_back(element);
                            bool found_lib = false;
                            for(auto j : all_paths)
                            {
                                if(find_install_package_for_lib(j, l_name_so, l_name_a, &found_lib) == 0)
                                    break;

                            }

                            if(!found_lib)
                            {
                                cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                                for(auto j : all_paths)
                                    if(find_install_package_for_lib(j, l_name_so, l_name_a) == 0)
                                        break;
                                            
                                            
                            }

                        }


                    }


                }

            }

        
    };

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

            if(fs::exists(os.get_unpack_dir()/"CMakeLists.txt") && os.assembly_cmake() == 0) //CMake
            {
                cout<<"Собрано с помощью CMake"<<"\n";
                os.cd(os.get_build_dir());
                cout << os.get_current_path();
                os.return_code_command({"make", "-n", "install"},true); 
                os.cd(os.get_current_path());
            }
            else if(fs::exists(os.get_unpack_dir()/"configure") && fs::exists(os.get_unpack_dir()/"Makefile.in")  //GNU Autotools
                    && fs::exists(os.get_unpack_dir()/"config.h.in")  && os.assembly_autotools() == 0)
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
            else if(fs::exists(os.get_unpack_dir()/"Rakefile") && !os.gemspec_exists(os.get_unpack_dir())) //Ruby тут нет gemspec
            {
                os.return_code_command({"gem", "install", "rake", "rake-compiler"},true);
                os.cd(os.get_unpack_dir());
                if(os.return_code_command({"rake", "--all", "-n"}) == 0) cout<<"Прогон всех шагов завершен успешно"<<"\n";  
                os.cd(os.get_current_path());  
            }
            else if(fs::exists(os.get_unpack_dir()/"Build.PL") && os.assembly_perl_build() == 0) //Perl
            {
                cout<<"Собрано с помощью Build и Build.PL"<<"\n";
                os.cd(os.get_unpack_dir());
                os.return_code_command({"./Build", "install"},true);
                os.cd(os.get_current_path());
            }

            else if(fs::exists(os.get_unpack_dir()/"Makefile.PL") && os.assembly_perl_make() == 0) //Perl
            {
                cout<<"Собрано с помощью make и Makefile.PL"<<"\n";
                os.cd(os.get_unpack_dir());
                os.return_code_command({"make", "-n", "install"},true);
                os.cd(os.get_current_path());
            }
            else if(fs::exists(os.get_unpack_dir()/"config.m4") && os.assembly_php() == 0) //PHP
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
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    class Windows
    {
        protected:
            fs::path curr_path;
            fs::path unpack_dir;
            fs::path build_dir;
            string archive_name;
        public:
            int run_command(const char* cmd, bool redirection_stdout = false, bool trace = false)
            {

                //CMAKE_PREFIX_PATH (если нет,то создать, иначе добавить через ;)
                //pacman.exe для msys
                //Тут unicode c_cpp_properties.json????
                curr_path = fs::current_path();
                cerr << "Running command {" << cmd << "} in {"<< curr_path << "}" << endl; 
                STARTUPINFO si;
                PROCESS_INFORMATION piCom;
                DWORD dwExitCode;

                ZeroMemory(&si, sizeof(STARTUPINFO));
                si.cb = sizeof(STARTUPINFO);

                int return_code;
                

                if(redirection_stdout)
                {
                    fs::path stdout_path = build_dir;
                    if (trace)
                        stdout_path /= "trace-output.txt";
                    else
                        stdout_path /= "stdout.txt";
                    string stdout_path_str = stdout_path.string();

                    SECURITY_ATTRIBUTES sa;
                    sa.nLength = sizeof(sa);
                    sa.lpSecurityDescriptor = NULL;
                    sa.bInheritHandle = TRUE;
                    //перенаправление stdout/stderr в файл
                    HANDLE h = CreateFile(stdout_path_str.c_str(), GENERIC_READ|FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            
                    if(h == INVALID_HANDLE_VALUE)
                    {
                        cerr << "Ошибка открытия/создания " << stdout_path_str << endl;
                        return 1;
                    }       

                    si.dwFlags |= STARTF_USESTDHANDLES;
                    si.hStdInput = NULL;
                    si.hStdOutput = h;
                    if(trace)
                        si.hStdError = h;
                
                    auto cmdcopy = strdup(cmd); //копирование строк
                    if(!CreateProcess(NULL, cmdcopy, NULL, NULL, TRUE, 0, NULL, NULL, &si, &piCom)) //если выполняется успешно, то возвращается ненулевое значение
                    //если функция выполняется неудачно, то возвращается нулевое значение
                    {
                        //Закрываем открытый дескриптор файла перед удалением
                        CloseHandle(h);
                        //Удаляем файл
                        DeleteFile(stdout_path_str.c_str());
                        cerr << "Не удалось создать дочерний процесс\n";
                        return_code = 0;
                    
                    }
                    else
                    {
                        //приостанавливаем выполнение родительского процесса,
                        //пока не завершится дочерний процесс
                        WaitForSingleObject(piCom.hProcess, INFINITE);
                        // дочерний процесс завершился; получаем код его завершения
                        GetExitCodeProcess(piCom.hProcess, &dwExitCode);
                        //закрываем дескрипторы этого процесса
                        CloseHandle(piCom.hThread);
                        CloseHandle(piCom.hProcess);

                        if(!trace)
                        {
                            regex template_incorrect_path(R"(^(.*\\)?\.\.\\)");
                            ifstream file(stdout_path_str); //поток для чтения 
                            string s;
                            int code = 0;

                            while(getline(file,s))
                            {
                                if(regex_match(s,template_incorrect_path))
                                {
                                    code = 1;
                                    break;

                                }

                            }

                            file.close();

                            //Закрываем открытый дескриптор файла перед удалением
                            CloseHandle(h);

                            //Удаляем файл
                            DeleteFile(stdout_path_str.c_str());

                            if(code) return_code = 0;
                            else
                            {
                                cout << "Подозрительных(с путями, которые при установке могут повредить систему) файлов в архиве не обнаружено\n";
                                return_code = 1;
                            } 


                        }
                    
                        //Закрываем открытый дескриптор файла перед удалением
                        CloseHandle(h);
                    }
                    
                    
                }
                else {
                    auto cmdcopy = strdup(cmd); //копирование строк
                    if(!CreateProcess(NULL, cmdcopy, NULL, NULL, FALSE, 0, NULL, NULL, &si, &piCom))
                    {
                        cerr << "Не удалось создать дочерний процесс\n";
                        return_code = 0;
                    }
                    else
                    {
                        //приостанавливаем выполнение родительского процесса,
                        //пока не завершится дочерний процесс
                        WaitForSingleObject(piCom.hProcess, INFINITE);
                        // дочерний процесс завершился; получаем код его завершения
                        GetExitCodeProcess(piCom.hProcess, &dwExitCode);
                        //закрываем дескрипторы этого процесса
                        CloseHandle(piCom.hThread);
                        CloseHandle(piCom.hProcess);
                        return_code = 1;

                    } 
                }

                if(return_code) return 0;
                return 1;
            
            
            }

            int run_command_pacman(const char* cmd)
            {

                curr_path = fs::current_path();
                cerr << "Running PACMAN command {" << cmd << "} in {"<< curr_path << "}" << endl; 
                int return_code;
                if(system(cmd) == 0)
                {
                    fs::path stdout_path = build_dir;
                    stdout_path /= "pacman_stdout.txt";
                    auto stdout_path_str = stdout_path.string();

                    ifstream pacman_file(stdout_path_str.c_str());  //поток для чтения 
                    string s;

                    size_t found;

                    while(getline(pacman_file,s))
                    {
                        found = s.rfind(" ");
                        s.erase(found);
                        found = s.rfind(" ");
                        string ss = s.substr(found + 1);
                        cout << "Пакет,предоставляющий библиотеку, - " << ss << endl;
                        cout << "Установка пакета " << ss << endl;
                        string command = "C:\\msys64\\usr\\bin\\pacman.exe -S -y" + ss + " 2> NUL";
                        cout << command << endl;
                        system(command.c_str());


                    }
                    return_code = 0;
                }
                else
                {
                    cout << "Не удалось найти пакет, предоставляющий библиотеку" << endl;
                    return_code = 1;
                } 
                
                return return_code;
                
            }


            int run_command_pacman_check(const char* cmd, string full_path)
            {

                curr_path = fs::current_path();
                cerr << "Running PACMAN command {" << cmd << "} in {"<< curr_path << "}" << endl; 
                int return_code = 1;
                if(system(cmd) == 0)
                {
                    fs::path stdout_path = build_dir;
                    stdout_path /= "pacman_stdout.txt";
                    auto stdout_path_str = stdout_path.string();

                    ifstream pacman_file(stdout_path_str.c_str());  //поток для чтения 
                    string s;

                    size_t found;

                    int num_string = 0;
                    string package_name;
                    string tmppath;

                    while(getline(pacman_file,s))
                    {
                        num_string++;
                        if(s[0] != ' ')
                        {
                            package_name = s;
                            found = package_name.rfind(" ");
                            package_name.erase(found);
                        }              
                        else 
                        {
                            tmppath = s;
                            int size_str = tmppath.size();
                            while(tmppath[0] == ' ')
                                tmppath.erase(0,1);
                            
                            while(tmppath[size_str - 1] == ' ')
                                tmppath.erase(size_str - 1);           
                        

                            string ss;
                            int ind = full_path.size() - tmppath.size(); //в консоль выводится путь неполный, а относительно каталога 
                            //среды: clang,mingw,ucrt...., если неполный путь длиннее, чем требуемый полный путь, то это точно не подходящий вариант
                            if(ind > 0 && tmppath == full_path.substr(ind))
                            {
                                //если пакет ставит библиотеку туда, куда надо, то устанавливаем пакет
                                cout << "Установка пакета " << package_name << endl;
                                string command = "C:\\msys64\\usr\\bin\\pacman.exe -S -y" + package_name + " 2> NUL";
                                cout << command << endl;
                                if(system(command.c_str()) == 0)
                                {
                                    cout << "Пакет " << package_name << " установлен" << endl;
                                    return_code = 0;
                                    break;

                                }

                            }
                        }

                    }
                }
                else
                {
                    cout << "Не удалось найти пакет, предоставляющий библиотеку" << endl;
                    return_code = 1;
                } 

                
                return return_code;
                
            }


            list< vector<string> > name_variants(string name)
            {
                list< vector<string> > name_var;
                vector<string> tmp_names;

                

                //особый случай (пока один)
                //просто сохрняем в исходном виде
                if(name == "Curses")
                {
                    tmp_names = {"Curses", "Ncurses"};
                    name_var.push_back(tmp_names);
                }
                else
                {
                    tmp_names = {name};
                    name_var.push_back(tmp_names);

                }

                vector<string> first_filter; 
                //просто приводим все к нижнему регистру
                for(vector<string> elem : name_var)
                {
                    for(string i : elem)
                    {
                        for(int j = 0; j < i.size(); j++)
                            if(isupper(i[j]))
                            {
                                char t = tolower(i[j]);
                                i[j] = t;
                            }
                        first_filter.push_back(i);
                    }
                }
                name_var.push_back(first_filter);

                //если несколько букв в верхнем регистре, то вставляем - и переводим все заглавные к нижнему регистру
                vector<string> second_filter; 
                string second_type_name;
                bool insert_tire; 
                for(string i : name_var.front())
                {
                    second_type_name = i;
                    int counter = 0;
                    int ind = 0;
                    insert_tire = false;
                    
                    while(ind < second_type_name.size())
                    {
                        if(isupper(second_type_name[ind]))
                        {
                            counter++;
                            char t = tolower(second_type_name[ind]);
                            second_type_name[ind] = t;
                            if(counter > 1)
                            {
                                insert_tire = true;
                                string d;
                                d = second_type_name.substr(0,ind)  + "-" + second_type_name.substr(ind);
                                second_type_name = d;
                            }
                        }
                        ind++;
                    }
                    if(insert_tire) second_filter.push_back(second_type_name); //если одна заглавная, то она просто станет строчной и будет повтор первого фильтра
                }
                if(second_filter.size() != 0)  name_var.push_back(second_filter);

                
                //Добваляем lib в начало и расширения .dll,.a,.dll.a, .lib
                vector<string> third_filter;
                string third_type_name;
                for(vector<string> elem : name_var)
                {
                    for(string i : elem)
                    {
                        third_type_name = "lib" + i + ".dll"; //динамическая библиотека
                        third_filter.push_back(third_type_name);

                        third_type_name = "lib" + i + ".a"; //статическая библиотека
                        third_filter.push_back(third_type_name);

                        third_type_name = "lib" + i + ".dll.a"; //специфика MSYS, статические библиотеки
                        third_filter.push_back(third_type_name);

                        third_type_name = "lib" + i + ".lib"; //статические библиотеки
                        third_filter.push_back(third_type_name);
                    }

                }

                name_var.push_back(third_filter);



                //+ -devel
                vector<string> fourth_filter;
                for(vector<string> elem : name_var)
                {
                    for(string i : elem)
                    {
                        if(i.substr(0,3) != "lib")
                        {
                            string fourth_type_name = i + "-devel";
                            fourth_filter.push_back(fourth_type_name);
                            
                        }
                        
                    }

                }

                name_var.push_back(fourth_filter);


                for(vector<string> elem : name_var)
                {
                    cout << "{ ";
                    for(string i : elem)
                        cout << i << " ";
                    cout << "}" << endl;
                }

                return name_var;

            }

            int find_install_package_for_lib(fs::path pt, string dll, string a, string dlla, string lib, string stdout_path_str, bool *found_lib = nullptr, bool library_not_found_anywhere = false)
            {
                int return_code = 0;
                string ppt = pt.string();
                HANDLE hd = CreateFile(stdout_path_str.c_str(), GENERIC_READ|FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                CloseHandle(hd);
                string command;
            
                if(library_not_found_anywhere)
                {
                    command = "C:\\msys64\\usr\\bin\\pacman.exe -Fx " + dll + " >> " + stdout_path_str;
                    return_code = run_command_pacman_check(command.c_str(), ppt + "/" + dll);
                    if(return_code) 
                    {
                        command = "C:\\msys64\\usr\\bin\\pacman.exe -Fx " + a + " >> " + stdout_path_str;
                        return_code =  run_command_pacman_check(command.c_str(), ppt + "/" + a);
                        if(return_code)
                        {
                            command = "C:\\msys64\\usr\\bin\\pacman.exe -Fx " + dlla + " >> " + stdout_path_str;
                            return_code =  run_command_pacman_check(command.c_str(), ppt + "/" + dlla);
                            if(return_code)
                            {
                                command = "C:\\msys64\\usr\\bin\\pacman.exe -Fx " + lib + " >> " + stdout_path_str;
                                return_code =  run_command_pacman_check(command.c_str(), ppt + "/" + dlla);

                            }
                        }
                    }
                    return return_code;

                }
                if(fs::exists(pt/dll))
                {
                    if(found_lib)
                        *found_lib = true;                      
                    cout << "Библиотека " << ppt << "/" << dll << " найдена" << endl;
                    command = "C:\\msys64\\usr\\bin\\pacman.exe -Qo " +  ppt + "/" + dll + " >> " + stdout_path_str;
                    run_command_pacman(command.c_str());

                }
                else
                {
                    cout << "Библиотека " << ppt << "/" << dll << " не найдена" << endl;

                    if(fs::exists(pt/a))
                    {
                        if(found_lib)
                            *found_lib = true;                                  
                        cout << "Библиотека " << ppt << "/" << a << " найдена" << endl;
                        command = "C:\\msys64\\usr\\bin\\pacman.exe -Qo " +  ppt + "/" + a + " >> " + stdout_path_str;
                        run_command_pacman(command.c_str());
                    }
                    else
                    {
                        cout << "Библиотека " << ppt << "/" << a << " не найдена" << endl;
                        if(fs::exists(pt/dlla))
                        {
                            if(found_lib)
                                *found_lib = true;                                  
                            cout << "Библиотека " << ppt << "/" << dlla << " найдена" << endl;
                            command = "C:\\msys64\\usr\\bin\\pacman.exe -Qo " +  ppt + "/" + dlla + " >> " + stdout_path_str;
                            run_command_pacman(command.c_str());
                        }
                        else
                        {
                            cout << "Библиотека " << ppt << "/" << dlla << " не найдена" << endl;
                            if(fs::exists(pt/lib))
                            {
                                if(found_lib)
                                    *found_lib = true;                                  
                                cout << "Библиотека " << ppt << "/" << lib << " найдена" << endl;
                                command = "C:\\msys64\\usr\\bin\\pacman.exe -Qo " +  ppt + "/" + lib + " >> " + stdout_path_str;
                                run_command_pacman(command.c_str());
                            }
                            else 
                            {
                                cout << "Библиотека " << ppt << "/" << lib << " не найдена"  << endl;
                                return_code = 1;
                            }

                            
                        }

                        
                    }

                }

                return return_code;

            }


            void find_lib(vector<string> libs, vector<string> libsPath)
            {

                string path;
                string check_l;
                int mypipe[2];
                int mypipe2[2];
                string so_lib,a_lib;
                string tmp_lib_name; 
                bool found;
                vector <fs::path> path_copy;
                string nl = "";
                fs::path stdout_path = build_dir;
                stdout_path /= "pacman_stdout.txt";
                auto stdout_path_str = stdout_path.string();

                for(auto l : libs)
                {
                    found = false;
                    if(l[0] == '/') //если имя библиотеки начинается с /, то,значит, она написана с полным путем до нее
                    {
                        //cout << l << endl;
                        fs::path path_tmp(l);
                        string lib_name = path_tmp.filename().string(); //получаем имя библиотеки

                        if(fs::exists(l))
                        {
                            cout << "Библиотека " << l << " найдена" << endl;
                            string command = "C:\\msys64\\usr\\bin\\pacman.exe -Qo " +  l + " >> " + stdout_path_str;
                            run_command_pacman(command.c_str());
                        }
                        else
                        {
                            cout << "Библиотека " << l << " не найдена" << endl;
                            cout << "Поиск пакета, который предоставляет необходимую библиотеку" << endl;
                            string command = "C:\\msys64\\usr\\bin\\pacman.exe -Fx " + l + " >> " + stdout_path_str;
                            run_command_pacman_check(command.c_str(), l);
                        }

                
                        continue;
                    
                    }

                    tmp_lib_name = "";

                    bool type_libs[3] {false,false,false};
                    regex t1(".dll");
                    regex t2(".dll.a");
                    regex t3(".lib");
                    regex t4(".a");
                    if(l[0] == '-' && l[1] == 'l') //-lname = libname.dll || libname.a || libname.lib || libname.dll.a
                    {
                        tmp_lib_name = l.substr(2); //убираем -l
                        type_libs[0] = true;

                    }    
                    else if(l[0] == '-' && l[1] == 'p') //-pthread=-lpthread=libpthread.dll || libpthread.a || libpthread.lib || libpthread.dll.a
                    {
                        tmp_lib_name = l.substr(1); //убираем -
                        type_libs[1] = true;
                    }    
                    else if(regex_search(l,t1) || regex_search(l,t2) || regex_search(l,t3) || regex_search(l,t4) ) //прямое подключение name.a || name.dll || name.lib || name.dll.a
                    {
                        type_libs[2] = true;          
                    }

                    if(type_libs[0] || type_libs[1] || type_libs[2])
                    {
                        for(auto p : libsPath) //ищем по всем путям указанным
                        {
                            path = p;
                            check_l = "";
                            for(int i = 0; i < 2; i++)
                                check_l += p[i];
                            if(check_l == "-L")
                                path = p.substr(2, p.size() - 2); //путь к пути библиотеки (убираем -L, если передавали такие пути)
                            
                            int size = path.size();
                            if(path[size - 1] == '/') //удаляется последний / в пути, если он был (нормализация пути)
                            {
                                path.erase(size - 1);
                                size = path.size();
                            }

                            fs::path path_to_lib = path;

                            path_copy.push_back(path_to_lib);

                            if(type_libs[0] || type_libs[1]) //для случая -lname и -pname
                            {
                                find_install_package_for_lib(path_to_lib, "lib" + tmp_lib_name + ".dll", "lib" + tmp_lib_name + ".a", "lib" + tmp_lib_name + ".dll.a", "lib" + tmp_lib_name + ".lib", stdout_path_str, &found);
                                                            
                            }
                            else if(type_libs[2]) //прямое подключение библиотеки (полное имя и расширение)
                            {

                                if(fs::exists(path_to_lib/l))
                                {
                                    string command = "C:\\msys64\\usr\\bin\\pacman.exe -Qo " +  path_to_lib.string() + "/" + l + " >> " + stdout_path_str;
                                    if(run_command_pacman(command.c_str()) == 0)
                                        found = true;
                                    
                                }    

                            }

                            if(found) break;
                        }
                    }
                    else
                    {
                        cerr << "Некорректное имя библиотеки\n";
                        exit(1);
                    }

                    if(!found) //библиотека не нашлась ни по одному из путей
                    {
                        cout << "Библиотека " << l << " не найдена ни по одному из указанных путей" << endl;
                        cout << "Поиск пакета, который предоставляет необходимую библиотеку по одному из путей" << endl;
                        for(auto i: path_copy)
                        {
                            
                            if(type_libs[2] == false && find_install_package_for_lib(i, "lib" + tmp_lib_name + ".dll", "lib" + tmp_lib_name + ".a", "lib" + tmp_lib_name + ".dll.a", "lib" + tmp_lib_name + ".lib", stdout_path_str, NULL, true) == 0)
                                break;
                            else if(type_libs[2])
                            {
                                string command = "C:\\msys64\\usr\\bin\\pacman.exe -Fx " + l + " >> " + stdout_path_str;
                                if(run_command_pacman_check(command.c_str(), i.string() + "/" + l) == 0) break;
                            }
                        //попытка найти пакет и установить библиотеку 
                        }
                    }
                }
                
            }

            void check_toolchain(regex mask1, regex mask2, fs::path pathToDir)
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
                fs::directory_iterator dirIterator1(pathToDir);
                string pathToFile;
                string target_language;
                string toolchain_language;
                
                for(const auto& entry : dirIterator1) //toolchain.json
                {
                    if(!is_directory(entry))
                    {
                        if(regex_match(entry.path().string().c_str(),mask1))
                        {
                            pathToFile = entry.path().string();
                            //чтение JSON из файла
                            ifstream i(pathToFile);
                            json j;
                            i >> j;
                            toolchain_language = j["toolchains"][0]["language"];

                        }

                    }
                }

                fs::directory_iterator dirIterator2(pathToDir); //target.json
                for(const auto& entry : dirIterator2)
                {
                    if(!is_directory(entry))
                    {
                        if(regex_match(entry.path().string().c_str(),mask2))
                        {
                            pathToFile = entry.path().string();
                            //чтение JSON из файла
                            ifstream i(pathToFile);
                            json j;
                            i >> j;
                            if(j["link"].size() != 0)
                                target_language = j["link"]["language"];//может не быть link и соответственно language
                            else
                                target_language = "";
                            if(target_language!="" && target_language != toolchain_language)
                            {
                                cerr << "Параметры toolchain не совпадают с target\n";
                                exit(1);
                            }
                            else
                            {
                                cout << "Параметры toolchain совпадают с target\n";
                            }

                        }

                    }
                }

            }

            vector<string> linkDirectories(regex mask, fs::path pathToDir)
            {
                string pathToFile;
                vector<string> linkDirs;
                fs::directory_iterator dirIterator(pathToDir);
                for(const auto& entry : dirIterator)
                {
                    if(!is_directory(entry))
                    {
                        if(regex_match(entry.path().string().c_str(),mask))
                        {
                            pathToFile = entry.path().string();
                            ifstream i(pathToFile);
                            json j;
                            i >> j;
                            int size = j["toolchains"][0]["compiler"]["implicit"]["linkDirectories"].size();
                            for(int i = 0; i < size; i++)
                                linkDirs.push_back(j["toolchains"][0]["compiler"]["implicit"]["linkDirectories"][i]);
                    
                                            
                        }
                    }
                }

                return linkDirs;

            }

            vector<string> libpath(fs::path pathToJson)
            {

                ifstream i(pathToJson);
                vector<string> libraryPath;
                json j;
                i >> j;
                if(j["link"].is_null())
                    return libraryPath;
                int size = j["link"]["commandFragments"].size();

                for (int ind = 0; ind < size; ind++)
                {
                    if(j["link"]["commandFragments"][ind]["role"] == "libraryPath")
                        libraryPath.push_back(j["link"]["commandFragments"][ind]["fragment"]);

                }

                return libraryPath;
                
            }

            void check_libs(vector<string> libs, fs::path buildDir)
            {
                bool check = true;
                int size = libs.size();
                if(size == 0 || size == 1) //Если есть одна строка с -L, то все хорошо (только внешние или внутренние),если нет строки с -L, то все вообще хорошо
                {    
                                
                    cout << "Правильный порядок включения библиотек при компоновке" << endl; //сначала пользовательские, потом системные 
                
                }
                else //Если больше 1, то надо следить
                {
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
                    string bdir = build_dir.string();
                    int build_dir_name_size = bdir.size();

                    while(bdir[build_dir_name_size - 1] == '/')
                    {
                        bdir.erase(build_dir_name_size - 1); //удаляется 1 символ с build_dir_name_size - 1 индекса
                    //удаляется последний / в пути, если он был
                        build_dir_name_size = bdir.size();
                    }

                    string path;
                    string tmp_path;
                    for(auto i: libs)
                    {
                        prev_type_lib = curr_type_lib;
                        path = i.substr(2, i.size() - 2); //путь к пути библиотеки (убираем -L)
                        int size = path.size();

                        if(path[size - 1] == '/') //удаляется последний / в пути, если он был
                        {
                            path.erase(size - 1);
                            size = path.size();
                        }

                        if(size == build_dir_name_size)
                        {
                            if(bdir == path)
                                curr_type_lib = internal;
                            else
                                curr_type_lib = external;
                        }
                        else if(size > build_dir_name_size) //Возможно, что какой-то вложенный каталог в каталог сборки
                        {
                            tmp_path = path.substr(0, build_dir_name_size + 1); //захватываем +1 символ, чтобы проверить, что 
                            //первые  build_dir_name_size + 1 символов в пути path равны build_dir + "/"
                            // /foo/bar /foo давали бы раньше верный резлуьтат при сравнении первых 4 символов
                            //теперь сравнивается первые пять /foo/ /foo/ и результат тоже верный.
                            //Раньше /foobar /foo давали неверный положительный результат ибо сравнивались первые 4 символа /foo /foo
                            // теперь сраваниваем первые 5 символов /foob  /foo/ и видим, что первый путь не является путем
                            //к вложенному в /foo/ подкаталогу
                            if(tmp_path == (bdir + "/")) //  Сранивается с добавлением /, так как возможнока ситуация /foo /foobar
                            //первые 4 символа равны, но второй путь не является путем к подкаталогу /foo
                                curr_type_lib = internal;
                            else
                                curr_type_lib = external;

                        } 
                        else
                            curr_type_lib = external;

                        if(prev_type_lib == external && curr_type_lib == internal)
                        {
                            check = false;
                            cerr << "Подозрительный порядок подключения библиотек при компоновке\n";
                            exit(1);
                            
                        }

                    }

                }

            }

            vector<string> lib_list(fs::path pathToJson)
            {
                vector<string> libs;
                ifstream i(pathToJson);
                json j;
                i >> j;
                if(j["link"].is_null())
                    return libs;
                int size = j["link"]["commandFragments"].size();

                for (int ind = 0; ind < size; ind++)
                {
                    if(j["link"]["commandFragments"][ind]["role"] == "libraries")
                    {
                        string tmp = j["link"]["commandFragments"][ind]["fragment"];
                        if(tmp.substr(0,3) != "-Wl")
                        {
                            if(tmp.find(' ') != tmp.npos) //если есть хоть один пробел (несколько библиотек в одной строке для Windows)
                            {
                                for(size_t p=0, q=0; p!=tmp.npos; p=q) //npos хранит -1, используется для  индикации отсутствия позиция символа или подстроки в строке
                                    libs.push_back(tmp.substr(p+(p!=0), (q=tmp.find(" ", p+1))-p-(p!=0)));
                            }
                            else  libs.push_back(j["link"]["commandFragments"][ind]["fragment"]);
                            
                        }
                        
                    }

                }

                return libs;
                
            }

            void libs_info(regex mask, fs::path pathToDir, fs::path buildDir)
            {
                //toolchain точно один
                regex mask1(".*(toolchains).*",regex_constants::icase);

                check_toolchain(mask1, mask, pathToDir);

                vector <string> linkDirs = linkDirectories(mask1,pathToDir); //Все каталоги, в которых стандартно ищутся библиотеки

                for(auto i: linkDirs)
                    cout << i << " ";
                cout << endl;

                string pathToFile;
                fs::directory_iterator dirIterator(pathToDir);
                for(const auto& entry : dirIterator)
                {
                    if(!is_directory(entry))
                    {
                        if(regex_match(entry.path().string().c_str(),mask))
                        {
                            //Берем бибилиотеки и пути с -L из каждого targeta, так как их может быть несколько
                            vector<string> libs;//Библиотеки
                            vector<string> libPath; //-L/...../
                            pathToFile = entry.path().string();
                            libs = lib_list(pathToFile);

                            libPath = libpath(pathToFile); //Тут пути с -L
                            if(libs.size() != 0)
                            {
                                for(auto j: libs)
                                    cout << j << " ";
                                cout << endl;
                            
                                if(libPath.size() != 0)
                                {
                                    for(auto j: libPath)
                                        cout << j << " ";
                                    cout << endl;

                                    check_libs(libPath, buildDir);

                                    find_lib(libs, libPath);

                                }

                                else
                                    find_lib(libs, linkDirs);
                            }
                                
                        }

                    }

                }


            }



            void cmake_trace()
            {
                string link_directories;
                vector <string> link_dirs;
                string unpackdir = unpack_dir.string();
                chdir(unpackdir.c_str());

                vector <string> libs_path = {"C:/msys64/ucrt64/lib/gcc/x86_64-w64-mingw32/13.2.0",
                                            "C:/msys64/ucrt64/lib/gcc",
                                            "C:/msys64/ucrt64/x86_64-w64-mingw32/lib",
                                            "C:/msys64/ucrt64/lib"};

                //curr_path = fs::current_path();
                
                chdir(unpackdir.c_str());

                run_command({"\"C:\\msys64\\ucrt64\\bin\\cmake.exe\" --trace-format=json-v1"},true, true);
                string pathtmp = build_dir.string() + "\\trace-output.txt";
                fstream in;  //поток для чтения
                in.open(pathtmp);
                map <string, string> cmd_set;
                string line;
                fs::path stdout_path = build_dir;
                stdout_path /= "pacman_stdout.txt";
                auto stdout_path_str = stdout_path.string();
                if(in.is_open())
                {
                    while(getline(in,line))
                    {
                        if(line[0] == '{')
                        {
                            stringstream s1;
                            json j;
                            s1 << line;
                            s1 >> j;


                            bool flag = false;
                            vector <fs::path> paths;
                            vector <string> names;

                            if(j["cmd"] == "set")
                            {
                                vector<string> key_value;

                                for(auto i : j["args"])
                                    key_value.push_back(i);

                                int count = 0;
                                string key;
                                string value;
                                for(string i : key_value)
                                {
                                    count++;
                                    if(count == 1)
                                        key = i;
                                    if(count == 2)
                                        value = i;
                                }
                            
                                cmd_set[key] = value;
                                //cout << key << " " << cmd_set[key] << endl;
                            
                            }

                            else if(j["cmd"] == "find_package")
                            {
                                string package_name;
                                package_name = j["args"][0];
                                cout << j["args"][0] << endl;
                                list < vector<string> > names;
                                names = name_variants(package_name);
                                bool inst = false;
                                for(vector <string> elem : names)
                                {
                                    if(inst) break;
                                    for(string i : elem)
                                    {
                                        if(i.substr(0,3) != "lib") //пакет
                                        {
                                            cout << "Установка пакета " << i << endl;
                                            string command = "C:\\msys64\\usr\\bin\\pacman.exe -S -y mingw-w64-ucrt-x86_64-" + i + " 2> NUL";
                                            cout << command << endl;
                                            if(system(command.c_str()) == 0)
                                            {
                                                cout << "Пакет mingw-w64-ucrt-x86_64-" << i << " установлен" << endl;
                                                inst = true;
                                                break;
                                            }
                                        }

                                        else //библиотека
                                        {
                                            vector<fs::path> standart_path;
                                            vector <string> libs = elem;
                                            for(auto p : libs_path)
                                                standart_path.push_back(p);
                                            bool lib_install = false;
                                            for(auto l : libs)
                                            {
                                                for(auto path : standart_path)
                                                    if(fs::exists(path/l))
                                                    {
                                                        string tmppath = path.string();
                                                        tmppath = tmppath + "/" + l;
                                                        cout << "Библиотека " << l << " найдена" << endl;
                                                        string command = "C:\\msys64\\usr\\bin\\pacman.exe -Qo " + tmppath + " >> " + stdout_path_str;
                                                        lib_install = true;
                                                        HANDLE hd = CreateFile(stdout_path_str.c_str(), GENERIC_READ|FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                                                        CloseHandle(hd);
                                                        if(run_command_pacman(command.c_str()) == 0)
                                                        {
                                                            inst = true;
                                                            break;
                                                        }

                                                    }
                                                if(lib_install) break;
                                            }

                                            if(!lib_install) //ни одна из библиотек не установлена, ищем любую и доставляем
                                            {
                                                for(auto l : libs)
                                                {
                                                    bool exit = false;
                                                    cout << "Библиотека " << l << " нигде не найдена" << endl;
                                                    for(auto path : standart_path)
                                                    {
                                                        cout << "Поиск библиотеки " << path.string() + "/" + l << endl;
                                                        string tmppath = path.string();
                                                        tmppath = tmppath + "/" + l;
                                                        string command = "C:\\msys64\\usr\\bin\\pacman.exe -Fx " + l + " >> " + stdout_path_str;
                                                        HANDLE hd = CreateFile(stdout_path_str.c_str(), GENERIC_READ|FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                                                        CloseHandle(hd);
                                                        if(run_command_pacman_check(command.c_str(),tmppath) == 0)
                                                        {
                                                            inst = true;
                                                            exit = true;
                                                            break;

                                                        }
                                                    }

                                                    if(exit) break;

                                                }


                                            }

                                            break;

                                        }

                                            
                                    }

                                }
                            }

                            else if(j["cmd"] == "find_library")
                            {
                                //string name_lib;
                                vector <string> names_lib;
                                vector <string> paths_lib;
                                //cout << j["args"] << endl;  
                            

                                if(j["args"][1] == "NAMES")
                                {
                                    if(j["args"].size() > 3)
                                    {
                                        int size = j["args"].size();
                                        for(int i = 2; i < size; i++)
                                        {
                                            string jargs = j["args"][i];
                                            regex r("if not in the C library");
                                            if(jargs != "HINTS" && jargs != "PATHS" && jargs != "NAMES_PER_DIR" && jargs != "DOC" && !regex_search(jargs, r)) 
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
                                if(j["args"].size() > 3)
                                {
                                    int size = j["args"].size();
                                    regex reg("[A-Z]*");
                                    string json_string;
                                    int ind;
                                    for(int i = 2; i < size; i++)
                                    {
                                        json_string = j["args"][i];
                                        if(j["args"][i] == "HINTS" || j["args"][i] == "PATHS")
                                        {
                                            ind = i;
                                            flag = true;
                                            
                                        }
                                        else if(regex_match(json_string,reg))
                                        {
                                            break;
                                        }

                                        if(json_string == "NO_DEFAULT_PATH") default_path = true;
                                            
                                        if(flag && !default_path && ind != i) paths_lib.push_back(j["args"][i]);
                                        
                                    }
                                    

                                }

                                for(auto i : names_lib)
                                {

                                    string tmp_name = i;
                                    while(tmp_name[0] == '$')
                                    {
                                        tmp_name.erase(0,1);
                                        tmp_name.erase(0,1);
                                        tmp_name.erase(tmp_name.size() - 1);
                                        //cout << tmp_name << " " << cmd_set[tmp_name] << " ";
                                        if(cmd_set[tmp_name] != "")
                                            tmp_name = cmd_set[tmp_name];
                                        else
                                            tmp_name = "";
                                    }
                                    cout << endl;

                                    names.push_back(tmp_name);

                                }

                                for(auto i : paths_lib)
                                {

                                    string tmp_path = i;
                                    while(tmp_path[0] == '$')
                                    {
                                        tmp_path.erase(0,1);
                                        tmp_path.erase(0,1);
                                        tmp_path.erase(tmp_path.size() - 1);
                                        //cout << tmp_path << " " << cmd_set[tmp_path] << " ";
                                        if(cmd_set[tmp_path] != "")
                                            tmp_path = cmd_set[tmp_path];
                                        else
                                            tmp_path = "";
                                    }
                                    cout << endl;
                                    if(tmp_path != "")
                                        paths.push_back(tmp_path);

                                }

                                for(auto i : names)
                                    cout << i << " ";
                                cout << endl;

                                //for(auto j : paths)
                                //    cout << j << " ";
                                //cout << endl;

                                for(auto i : names)
                                {
                                    if(i != "")
                                    {
                                        //{"args":["Intl_LIBRARY","NAMES","intl","libintl","NAMES_PER_DIR","DOC","libintl libraries (if not in the C library)"],"cmd":"find_library",
                                        //"file":"C:/msys64/ucrt64/share/cmake/Modules/FindIntl.cmake","frame":2,"global_frame":2,"line":132,"line_end":135,"time":1717533716.8236122}
                                        string l_name_dll;
                                        string l_name_a; 
                                        string l_name_dll_a;
                                        string l_name_lib;
                                        l_name_dll =  i + ".dll";
                                        l_name_a = i + ".a"; 
                                        l_name_dll_a =  i + ".dll.a";
                                        l_name_lib =  i + ".lib";
                                        if(flag) //не использовать дефолтные пути
                                        {
                                            bool found_lib = false;
                                            if(paths.size() != 0)
                                            {
                                                cout << "Поиск не осуществляется в дефолтных путях\n";
                                                for(auto j : paths)
                                                    if(find_install_package_for_lib(j, l_name_dll, l_name_a, l_name_dll_a, l_name_lib, stdout_path_str, &found_lib) == 0)
                                                        break;
                                                if(!found_lib)
                                                {
                                                    cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                                                    for(auto j : paths)
                                                    {
                                                        cout << "Поиск библиотеки " << j.string() + "/" + i << endl;
                                                        if(find_install_package_for_lib(j, l_name_dll, l_name_a, l_name_dll_a, l_name_lib, stdout_path_str, NULL, true) == 0)
                                                            break;
                                                    }
                                                }
                                            }

                                            else cout << "Нет путей (PATHS/HINTS) для поиска библиотеки " << i << endl;
                                            
                                        }

                                        else
                                        {
                                            cout << "Поиск осуществляется в дефолтных путях и в указанных дополнительно, если такие есть\n";
                                            vector <fs::path> toolchainpath;
                                            for(auto p : libs_path)
                                                toolchainpath.push_back(p);
                                            vector <fs::path> all_paths;
                                            all_paths = toolchainpath;

                                            for(const auto& element : paths)
                                            {
                                                if(element != "") all_paths.push_back(element);

                                            }

                                            bool found_lib = false;
                                            for(auto j : all_paths)
                                            {
                                                if(find_install_package_for_lib(j, l_name_dll, l_name_a, l_name_dll_a, l_name_lib, stdout_path_str, &found_lib) == 0)
                                                    break;

                                            }

                                            if(!found_lib)
                                            {
                                                cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                                                for(auto j : all_paths)
                                                {
                                                    cout << "Поиск библиотеки " << j.string() + "/" + i << endl;
                                                    if(find_install_package_for_lib(j, l_name_dll, l_name_a, l_name_dll_a,l_name_lib, stdout_path_str, NULL, true) == 0)
                                                        break;
                                                }                                        
                                                    
                                            }

                                        }


                                    }


                                }


                            }
                        
                        }
                    }
                }

                in.close();

            }

            int assembly_cmake()
            {
                int sum_code = 0;
                string builddir = build_dir.string();
                chdir(builddir.c_str());
                fs::path curr_path{fs::current_path()};
                fs::path tmp{fs::current_path()};
                uintmax_t n{fs::remove_all(tmp / "*")};

                tmp /= ".cmake";
                tmp /= "api";
                tmp /= "v1";
                tmp /= "query";
                fs::create_directories(tmp);

                string ttmp = tmp.string();

                chdir(ttmp.c_str());

                char path_codemodel[] = "codemodel-v2";
                HANDLE h1 = CreateFile(LPTSTR(path_codemodel), FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                CloseHandle(h1);

                char path_toolchain[] = "toolchains-v1";
                HANDLE h2 = CreateFile(LPTSTR(path_toolchain), FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                CloseHandle(h2);

                //chdir(builddir.c_str());

                string unpackdir = unpack_dir.string();

                chdir(unpackdir.c_str());

                cmake_trace();

                chdir(builddir.c_str());

                fs::path inc_dir {"C:\\Windows\\Temp\\archives\\oicb-master\\include"};
                regex mask1(".*(target).*",regex_constants::icase);

                fs::path reply_path = build_dir;
                reply_path /= ".cmake";
                reply_path /= "api";
                reply_path /= "v1";
                reply_path /= "reply";
            
                sum_code = sum_code || run_command(("\"C:\\msys64\\ucrt64\\bin\\cmake.exe\" -G \"MinGW Makefiles\" "+  unpack_dir.string() + "/CMakeLists.txt").c_str());

                libs_info(mask1,reply_path, build_dir);

                sum_code = sum_code || run_command("\"C:\\msys64\\ucrt64\\bin\\mingw32-make.exe\"");

                chdir(curr_path.string().c_str()); 

                
                return sum_code; 
                
            }

            void set_aname(string name) {archive_name = name;}
            void set_bdir(fs::path p) {build_dir = p;}
            void set_udir(fs::path p) {unpack_dir = p;}

    };

#endif

int main(int argc, char *argv[])
{
    if(argc == 1)
    {
        cerr << "Не указан путь к архиву\n";
        exit(1);
    }

string os;
#if defined(__linux__)
    Linux system;
    os = system.os_name("/etc/os-release");
    regex reg1(".*(opensuse).*",regex_constants::icase);
    regex reg2(".*(ubuntu).*",regex_constants::icase);
    if(regex_match(os,reg1) == true)
    {   
        OpenSuse distrib;
        distrib.set_path_to_archive(argv[1]);
        distrib.set_install_command({"zypper", "install", "-y"}); 
        toDo(distrib);
        
    }
    else if(regex_match(os,reg2) == true)
    {
        Ubuntu distrib;
        distrib.set_path_to_archive(argv[1]);
        distrib.set_install_command({"apt", "install", "-y"});
        toDo(distrib);
        
    }

#elif defined(__APPLE__)
    //apple code(other kinds of apple platforms)  

#elif defined(__unix__)
    Unix system;
    os = system.os_name("uname -a");
    regex reg3(".*(freebsd).*\n",regex_constants::icase);
    if(regex_match(os,reg3) == true) 
    {
        FreeBsd distrib;
        distrib.set_install_command({"pkg", "install", "-y"});
        distrib.set_path_to_archive(argv[1]);
        toDo(distrib);
    }

#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    //define something for Windows (32-bit and 64-bit, this part is common)
    /* _Win32 is usually defined by compilers targeting 32 or   64 bit Windows systems */

    #ifdef _WIN64
        string path_to_archive = argv[1];
        string dirBuild = "C:\\Windows\\Temp\\archives";

        Windows system;

        string path_arch = path_to_archive;
        string type_arch;
        cout << path_arch << "\n";
        fs::path path1(path_arch);
        string type_arch_tmp_one = path1.extension().string();
        size_t pos = path_arch.find(type_arch_tmp_one);
        string path_arch_tmp = path_arch.substr(0, pos);
        fs::path path2(path_arch_tmp);
        if(path2.extension()!=".tar") 
        {   
            type_arch = type_arch_tmp_one;
        }
        else
        {
            string type_arch_tmp_two = path2.extension().string();
            type_arch = type_arch_tmp_two+type_arch_tmp_one;

        }
        size_t found1 = path_arch.rfind("\\");
        size_t found2 = path_arch.find(type_arch);
        int c = found2 - (found1 + 1);
        string arch = path_arch.substr(found1 + 1,c);
    
        cout << arch << "  " << type_arch << "\n";

        string archive_name = arch;
                
        fs::path path3("C:\\Windows\\Temp\\archives");
        path3 /= arch;
        path3 /= "build-"+arch;

        //cout << path3 << endl;
        //template_incorrect_path = "^(.*/)?\\.\\./";
        fs::create_directories(path3);

        fs::path unpack_dir = "C:\\Windows\\Temp\\archives\\" + arch + "\\" + arch;
        fs::path build_dir = "C:\\Windows\\Temp\\archives\\" + arch + "\\build-" + arch;
        cout << unpack_dir << endl;
        cout << build_dir << endl; 

        system.set_bdir(build_dir);
        system.set_udir(unpack_dir);
        system.set_aname(archive_name);

        vector<string> unpack_cmd;
        enum archiver
        {
            tar,
            gem
        };

        int archiver = tar;
        if(type_arch == ".tar")
        {
            unpack_cmd = {"tar", "-xvf"};     
        }
        else if(type_arch == ".tar.bz2" || type_arch == ".tar.bzip2" || type_arch == ".tb2" || type_arch == ".tbz")
        {
            unpack_cmd = {"tar", "-xvjf"}; 
        }
        else if(type_arch == ".tar.xz" || type_arch == ".txz")
        {
            unpack_cmd = {"tar", "-xvJf"};

        }
        else if(type_arch == ".tar.gz" || type_arch == ".tgz")
        {
            unpack_cmd = {"tar", "-xvzf"};

        }
        else if(type_arch == ".gem")
        {
            //installation("ruby");
            //install_gems();
            archiver = gem;  
        }

        if(archiver == tar)
        {   
        //installation("tar");
            //флаг -c говорит о том, что команды считываются из строки

            if(system.run_command(("C:\\Windows\\System32\\tar.exe -tf " +  path_to_archive).c_str(),true) != 0)
            {
                cerr<<"Подозрительные файлы в архиве"<<"\n";
                exit(1);
            }
            else
            {   
                system.run_command(("C:\\Windows\\System32\\tar.exe " + unpack_cmd[1] + " " + path_to_archive + " " + "-C " + dirBuild + "\\" + arch).c_str());
                cout<<"Архив разархивирован"<<"\n";
            }

            if(fs::exists(unpack_dir/"CMakeLists.txt") && system.assembly_cmake() == 0)
            {
                cout << "Собрано с помощью CMake\n";
            
            }


        }
    #else
        //define something for Windows (32-bit only)
    #endif
    
#endif

}