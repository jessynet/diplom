#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <array>
#include <list>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>
#include <fcntl.h>
#include <experimental/filesystem>
#include <nlohmann/json.hpp>
#include <set> //множество(set) представляет такой контейнер, который может хранить только уникальные значения, применяется для
//создания коллекций, которые не должны иметь дубликатов
using namespace std;
namespace fs = std::experimental::filesystem;
using json = nlohmann::json;

string libname_ub = "";

//fd[0] – открыт на чтение, fd[1] – на запись (вспомните STDIN == 0, STDOUT == 1)

bool is_admin_ub() {
#ifdef WIN32
    // TODO
    return true;
#else
    return getuid() == 0; //краткая запись if(getuid()==0) {return true;} else {return false;}
#endif
}

int find_install_package_1_ub(int* stdout_pipe);

int run_command_1_ub(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr) {
#ifdef WIN32
    // CreateProcess()
    // CreateProcessAsUser()
    // ShellExecute...
#else
    if (need_admin_rights && !is_admin_ub())
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
                
                if(find_install_package_1_ub(stdout_pipe)!=0)
                    return_code = 1;   
                   
            }  
            
                
            do {
                waitpid(pid, &status, 0);
            } while(!WIFEXITED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
            int child_status;
            if(WEXITSTATUS(status) == 0) child_status = 0;
            else child_status = 1;
            return_code = return_code || child_status; 
            break;
        }   
    }
#endif
    return return_code;
}

int find_install_package_1_ub(int* stdout_pipe)
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

        if(name_package_for_lib != "")
        {
            
            found_package = true;
            if(run_command_1_ub({"apt", "install", "-y", name_package_for_lib},true) == 0)
            {    
                install = true;
                break; //пока ставится первый найденный пакет
            }
                
        }

        //this file not installed on the system
        //apt-file search /usr/include/fftw3.h

        //libfftw3-dev: /usr/include/fftw3.h 
        //the search is also carried out not in installed packages                                       
        
    }

    //высвобождение буфера
    free(line);

    if(!found_package)
        cout << "Не удалось найти пакет, который предоставляет библиотеку " << libname_ub << endl; //не прерывается программа ибо может собраться пакет и без этой библиотеки

    if(install) return 0;
    else return 1;
}

void find_lib_ub(vector<string> libs, vector<string> libsPath)
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

            libname_ub = l;

            if(fs::exists(l))
                cout << "Библиотека " << l << " найдена" << endl;
            else
            {
                cout << "Библиотека " << l << " не найдена" << endl;
                cout << "Поиск пакета, который предоставляет необходимую библиотеку" << endl;
            }

            run_command_1_ub({"apt-file", "-F", "search", l}, false, mypipe);           
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
            regex r1("(.*)\\.so");
            regex r2("(.*)\\.a");
            if(regex_match(l,r2)) //.a
                a_lib = l;
            else if(regex_match(l,r1)) //.so
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
                    libname_ub = path_to_lib/so_lib;
                    nl = so_lib;
                    if(fs::exists(path_to_lib/so_lib))
                    {   
                        libname_ub = path_to_lib/so_lib;
			            cout << "Библиотека " << libname_ub << " найдена" << endl;
                        run_command_1_ub({"apt-file", "-F",  "search", path_to_lib/so_lib}, false, mypipe);
                        found = true;
                    }
                    else if(fs::exists(path_to_lib/a_lib))
                    {
                        nl = a_lib;
                        libname_ub = path_to_lib/a_lib;
                        cout << "Библиотека " << libname_ub << " найдена" << endl;
                        run_command_1_ub({"apt-file", "-F", "search", path_to_lib/a_lib}, false, mypipe);
                        found = true;
                      
                    }

                
                }
                else if(so_lib != "") //только .so
                {
                    libname_ub = path_to_lib/so_lib;
                    nl = so_lib;
                    if(fs::exists(path_to_lib/so_lib))
                    {
                        cout << "Библиотека " << libname_ub << " найдена" << endl;
                        run_command_1_ub({"apt-file", "-F", "search", path_to_lib/so_lib}, false, mypipe);
                        found = true;
                      
                    }
                }
                else if(a_lib != "") //только .a
                {

                    libname_ub = path_to_lib/a_lib;
                    nl = a_lib;
                    if(fs::exists(path_to_lib/a_lib))
                    {
                        cout << "Библиотека " << libname_ub << " найдена" << endl;
                        run_command_1_ub({"apt-file", "-F", "search", path_to_lib/a_lib}, false, mypipe);
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
                libname_ub = i/nl;
                pipe(mypipe2);
			    if(run_command_1_ub({"apt-file", "-F", "search", i/nl}, false, mypipe2) == 0)
				    break;
               //попытка найти пакет и установить библиотеку 
            }
        }
    }
    
}

void check_toolchain_ub(regex mask1, regex mask2, fs::path pathToDir)
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

vector<string> linkDirectories_ub(regex mask, fs::path pathToDir)
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

vector<string> libpath_ub(fs::path pathToJson)
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

void check_libs_ub(vector<string> libs, fs::path buildDir)
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

vector<string> lib_list_ub(fs::path pathToJson)
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
            tmp = tmp.substr(0,3);
            if(tmp != "-Wl")
                libs.push_back(j["link"]["commandFragments"][ind]["fragment"]);

        }

    }

    return libs;

}

void libs_ub(regex mask, fs::path pathToDir, fs::path buildDir)
{
    //toolchain точно один
    regex mask1(".*(toolchains).*",regex_constants::icase);

    check_toolchain_ub(mask1, mask, pathToDir);

    vector <string> linkDirs = linkDirectories_ub(mask1,pathToDir); //Все каталоги, в которых стандартно ищутся библиотеки

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
                vector<string> libs;//Библиотеки
                vector<string> libPath; //-L/...../
                pathToFile = entry.path();
                libs = lib_list_ub(pathToFile);

                libPath = libpath_ub(pathToFile); //Тут пути с -L
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

                        check_libs_ub(libPath, buildDir);

                        find_lib_ub(libs, libPath);

                    }

                    else
                        find_lib_ub(libs, linkDirs);
                }
                       
            }

        }

    }


}

void find_depend_ubuntu(fs::path path_to_reply, fs::path buildDir)
{
    regex mask1(".*(target).*",regex_constants::icase);
    libs_ub(mask1, path_to_reply, buildDir);

}
