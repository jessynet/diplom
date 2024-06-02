#include <iostream>
#include <fstream>
#include <regex>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>
#include <string>
#include <list>
#include <experimental/filesystem>
#include <map>
#include <sstream>
#include "cmake_opensuse.h"
#include <nlohmann/json.hpp>

using namespace std;
namespace fs = std::experimental::filesystem;
using json = nlohmann::json;

fs::path path_to_package_lib_os = "";
vector <string> link_toolchain_dir_os;
string name_archive_os;

int find_install_package_for_lib_os(fs::path path, string name_so, string name_a, bool *found_lib = nullptr, bool library_not_found_anywhere = false)
{
    libname_os = path/name_so;
    int return_code_os = 0;
    int mypipe[2];
    if(pipe(mypipe))
    {
        perror("Ошибка канала");
        exit(1);
    }
    if(library_not_found_anywhere)
    {
        return_code_os = run_command_1_os({"zypper", "se", "--provides", name_so}, false, mypipe, true);
        if(return_code_os) 
        {
            pipe(mypipe);
            libname_os = path/name_a;
            return_code_os = run_command_1_os({"zypper", "se", "--provides", name_a}, false, mypipe, true);
        }
        return return_code_os;

    }
    if(fs::exists(path/name_so))
    {
        if(found_lib)
            *found_lib = true;                      
        cout << "Библиотека " << path/name_so << " найдена" << endl;
        run_command_1_os({"zypper", "se", "--provides", path/name_so}, false, mypipe);

    }
    else
    {
        cout << "Библиотека " << path/name_so << " не найдена" << endl;
        

        libname_os = path/name_a;
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
            run_command_1_os({"zypper", "se", "--provides", path/name_a}, false, mypipe);
        }
        else
        {
            cout << "Библиотека " << path/name_a << " не найдена" << endl;
            return_code_os = 1;
        }

    }

    return return_code_os;

}


void trace_os();

bool is_admin_trace_os() {

    //возвращает идентификатор пользователя для текущего процесса
    //когда программа запускается из под root, то идентификатор пользователя равен 0
    return getuid() == 0; //краткая запись if(getuid()==0) {return true;} else {return false;}

}

int run_command_trace_os(vector <string> cmd, bool need_admin_rights = false, bool first = false)
{

    if (need_admin_rights && !is_admin_trace_os())
        cmd.insert(cmd.begin(), "sudo");
    int return_code = 0;
    if(first)
    {
        fs::path curr_path = fs::current_path();
        string pathtmp = "/tmp/archives/" + name_archive_os;
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
    //cout << fd; // = 3
    if(first) trace_os();
    
    return return_code;

}



list< vector<string> > name_variants_os(string name)
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
    for(string i : name_var.front())
    {
        second_type_name = i;
        int counter = 0;
        int ind = 0;
        
        while(ind < second_type_name.size())
        {
            if(isupper(second_type_name[ind]))
            {
                counter++;
                char t = tolower(second_type_name[ind]);
                second_type_name[ind] = t;
                if(counter > 1)
                {
                    string d;
                    d = second_type_name.substr(0,ind)  + "-" + second_type_name.substr(ind);
                    second_type_name = d;
                }
            }
            ind++;
        }

        second_filter.push_back(second_type_name);
    }
    name_var.push_back(second_filter);

    
    //Добваляем lib в начало и расширения .dll,.a,.dll.a
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

void trace_os()
{
    chdir(path_to_package_lib_os.c_str());
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
                    names = name_variants_os(package_name);
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
                            if(i.substr(0,3) != "lib") //не библиотека
                            {
                                cout << "Установка пакета " << i << endl;
                                libname_os = i;

                                if(run_command_trace_os({"zypper","install", i},true) == 0)
                                {
                                    cout << "Пакет " << i << " установлен" << endl;
                                    inst = true;
                                    break;

                                }
                                    

                            }
                            else //библиотека
                            {
                                vector <string> lkDirs = link_toolchain_dir_os; //Все каталоги, в которых стандартно ищутся библиотеки
                                vector <fs::path> standart_path;
                                vector <string> libs = elem;
                                for(auto p : lkDirs)
                                    standart_path.push_back(p);
                                bool lib_install = false;
                                for(auto l : libs)
                                {
                                    for(auto path : standart_path)
                                        if(fs::exists(path/l))
                                        {
                                            cout << "Библиотека " << path/l << "найдена" << endl;
                                            if(run_command_1_os({"zypper", "se", "--provides", path/l}, false, mypipe) == 0)
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
                                        bool exit = false;
                                        cout << "Библиотека " << l << " нигде не найдена" << endl;
                                        for(auto path : standart_path)
                                        {
                                            libname_os = path/l;
                                            pipe(mypipe);
                                            if(run_command_1_os({"zypper", "se", "--provides", l}, false, mypipe, true) == 0)
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
                                if(j["args"][i] != "HINTS" && j["args"][i] != "PATHS")
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
                                        if(find_install_package_for_lib_os(j, l_name_so, l_name_a, &found_lib) == 0)
                                            break;
                                    if(!found_lib) //нигде не найдена библиотека 
                                    {
                                        cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                                        for(auto j : paths)
                                            if(find_install_package_for_lib_os(j, l_name_so, l_name_a, NULL, true) == 0)
                                                break;

                                    }
                                }

                                else cout << "Нет путей (PATHS/HINTS) для поиска библиотеки " << i << endl;

                            }

                            else
                            {
                                cout << "Поиск осуществляется в дефолтных путях и в указанных дополнительно, если такие есть\n";
                                regex mask1(".*(toolchains).*",regex_constants::icase);
                                vector <string> linkDirs = link_toolchain_dir_os; //Все каталоги, в которых стандартно ищутся библиотеки
                                vector <fs::path> toolchainpath;
                                for(auto p : linkDirs)
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
                                    if(find_install_package_for_lib_os(j, l_name_so, l_name_a, &found_lib) == 0)
                                        break;

                                }

                                if(!found_lib) //нигде не найдена библиотека 
                                {
                                    cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                                    for(auto j : all_paths)
                                        if(find_install_package_for_lib_os(j, l_name_so, l_name_a, NULL, true) == 0)
                                            break;

                                }

                               

                            }


                        }


                    }


                }


            }   
        
        }

    
    }

    in.close();
    fs::remove(p);

}


void opensuse_trace(fs::path unpc_path, fs::path path_to_package, vector <string> link_dirs, string archive_name)
{
    //chdir(unpc_path.c_str());
    path_to_package_lib_os = path_to_package;
    link_toolchain_dir_os = link_dirs;
    name_archive_os = archive_name;
    //chdir("/tmp/archives/xz-5.4.6/xz-5.4.6");
    run_command_trace_os({"cmake", "--trace-format=json-v1", unpc_path},false,true);
}

