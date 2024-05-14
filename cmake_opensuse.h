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

string libname_os = "";

//fd[0] – открыт на чтение, fd[1] – на запись (вспомните STDIN == 0, STDOUT == 1)

bool is_admin_os() {
#ifdef WIN32
    // TODO
    return true;
#else
    return getuid() == 0; //краткая запись if(getuid()==0) {return true;} else {return false;}
#endif
}

int find_install_package_1_os(int* stdout_pipe, int code_funct);
int find_install_package_2_os(int* stdout_pipe);

int run_command_1_os(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool not_found_full_path_lib = false, bool checking_existence_lib = false) {
#ifdef WIN32
    // CreateProcess()
    // CreateProcessAsUser()
    // ShellExecute...
#else
    if (need_admin_rights && !is_admin_os())
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
                if(!checking_existence_lib) //проверка наличия бибилотеки при скачиваниии без установки
                {
                    if(!not_found_full_path_lib) //Если обработка не библиотеки из полного пути, которая не найдена 
                        find_install_package_1_os(stdout_pipe,1);   
                    else
                        if(find_install_package_1_os(stdout_pipe,2) != 0)
                            return_code = 1;
                }
                else
                { 
                    if(find_install_package_2_os(stdout_pipe) != 0)
                        return_code = 1;

                }    
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


int run_command_2_os(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, string *path = nullptr)
{
    #ifdef WIN32
    // CreateProcess()
    // CreateProcessAsUser()
    // ShellExecute...
#else
    if (need_admin_rights && !is_admin_os())
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
               
            if(execvp(c,args)==-1)
            {
                cerr << "Не удалось выполнить комманду exec\n"; 
                _exit(42);
            }    
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
                int size_path_lib = libname_os.size();
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
                        if(tmppath == libname_os) found = true;
                
                    }
                     
                }
                free(line);
            }

            if(!found)
                return_code = 1;

                
            do {
                waitpid(pid, &status, 0);
            } while(!WIFEXITED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
            int child_status;
            if(WEXITSTATUS(status) == 0) child_status = 0;
            else child_status = 1;
            return_code = return_code || child_status; 
        }   
    }
#endif
    return return_code;

}

int find_install_package_1_os(int* stdout_pipe, int code_funct)
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
                    if(run_command_1_os({"zypper", "install", "-y", name_package_for_lib},true) == 0)
                    {
                        install = true;
                        break; //пока ставится первый найденный пакет
                    }
                    
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
                    if(run_command_1_os({"zypper", "install", "--download-only", "-y", name_package_for_lib},true,mypipe2,true,true) == 0)
                    {
                        found_package = true;
                        run_command_1_os({"zypper", "install", "-y", name_package_for_lib},true);
                        install = true;
                        break;

                    }
                }
            }      
            
                                                       
        }
    }

    //высвобождение буфера
    free(line);

    if(!found_package)
        cout << "Не удалось найти пакет, который предоставляет библиотеку " << libname_os << endl; //не прерывается программа ибо может собраться пакет и без этой библиотеки

    if(install) return 0;
    else return 1;
}

int find_install_package_2_os(int* stdout_pipe)
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
                run_command_2_os({"find", "/var/cache", "-type", "f", "-name", name_package_for_lib},true,mypipe3,&path_to_rpm_package);
                cout << path_to_rpm_package;
                int mypipe_4[2];
                if(pipe(mypipe_4))
                {
                    perror("Ошибка канала");
                    exit(1);

                }

                int f = path_to_rpm_package.find("\n");
                path_to_rpm_package.erase(f);

    
                if(run_command_2_os({"rpm", "-ql", path_to_rpm_package},false,mypipe_4) == 0)
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

void find_lib_os(vector<string> libs, vector<string> libsPath)
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

            libname_os = l;
            if(fs::exists(path_tmp))
            {
                cout << "Библиотека " << l << " найдена" << endl;
                run_command_1_os({"zypper", "se", "--provides", l}, false, mypipe);
            }
           else
            {
                cout << "Библиотека " << l << " не найдена" << endl;
                if(run_command_1_os({"zypper", "se", "--provides", lib_name}, false, mypipe, true) != 0)
                    cout << "Не удалось найти пакет, который предоставляет библиотеку " << l << endl;               
            }

            continue;
        
        }

        so_lib = "";
        a_lib = "";
        tmp_lib_name = "";
        libname_os = "";
            
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
                    libname_os = path_to_lib/so_lib;
                    nl = so_lib;
                    if(fs::exists(path_to_lib/so_lib))
                    {   
                        nl = so_lib;
                        libname_os = path_to_lib/so_lib;
                        cout << "Библиотека " << libname_os << " найдена" << endl;
                        run_command_1_os({"zypper", "se", "--provides", path_to_lib/so_lib}, false, mypipe);
                        found = true;
                    }
                    else if(fs::exists(path_to_lib/a_lib))
                    {
                        nl = a_lib;
                        libname_os = path_to_lib/a_lib;
                        cout << "Библиотека " << libname_os << " найдена" << endl;
                        run_command_1_os({"zypper", "se", "--provides", path_to_lib/a_lib}, false, mypipe);
                        found = true;
                      
                    }

                
                }
                else if(so_lib != "") //только .so
                {
                    libname_os = path_to_lib/so_lib;
                    nl = so_lib;
                    if(fs::exists(path_to_lib/so_lib))
                    {
                        cout << "Библиотека " << libname_os << " найдена" << endl;
                        run_command_1_os({"zypper", "se", "--provides", path_to_lib/so_lib}, false, mypipe);
                        found = true;
                      
                    }
                }
                else if(a_lib != "") //только .a
                {
                    libname_os = path_to_lib/a_lib;
                    nl = a_lib;
                    if(fs::exists(path_to_lib/a_lib))
                    {
                        cout << "Библиотека " << libname_os << " найдена" << endl;
                        run_command_1_os({"zypper", "se", "--provides", path_to_lib/a_lib}, false, mypipe);
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
                libname_os = i/nl;
                pipe(mypipe2);
			    if(run_command_1_os({"zypper", "se", "--provides", nl}, false, mypipe2,true) == 0)
				    break;
               //попытка найти пакет и установить библиотеку 
            }
            
        }      
    }
    
}

void check_toolchain_os(regex mask1, regex mask2, fs::path pathToDir)
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

vector<string> linkDirectories_os(regex mask, fs::path pathToDir)
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

vector<string> libpath_os(fs::path pathToJson)
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

void check_libs_os(vector<string> libs, fs::path buildDir)
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

vector<string> lib_list_os(fs::path pathToJson)
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

void libs_os(regex mask, fs::path pathToDir, fs::path buildDir)
{
    //toolchain точно один
    regex mask1(".*(toolchains).*",regex_constants::icase);

    check_toolchain_os(mask1, mask, pathToDir);

    vector <string> linkDirs = linkDirectories_os(mask1,pathToDir); //Все каталоги, в которых стандартно ищутся библиотеки

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
                libs = lib_list_os(pathToFile);

                libPath = libpath_os(pathToFile); //Тут пути с -L
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

                        check_libs_os(libPath, buildDir);

                        find_lib_os(libs, libPath);

                    }

                    else
                        find_lib_os(libs, linkDirs);
                }
                       
            }

        }

    }

}

void find_depend_opensuse(fs::path path_to_reply, fs::path buildDir)
{
    regex mask1(".*(target).*",regex_constants::icase);
    libs_os(mask1, path_to_reply, buildDir);

}
