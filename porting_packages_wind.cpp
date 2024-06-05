#include <iostream>
#include <fstream>
#include <windows.h>
#include <conio.h>
#include <regex>
#include <string>
#include <list>
#include <cmath>
#include <vector>
#include <tchar.h>
#include <dir.h>
#include <experimental/filesystem>
#include <nlohmann\json.hpp>
#include <cctype>

using namespace std;
namespace fs = std::experimental::filesystem;

using json = nlohmann::json;
fs::path curr_path;
fs::path unpack_dir;
fs::path build_dir;
string archive_name;
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

    
    //Добваляем lib в начало и расширения .dll,.a,.dll.a
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

int find_install_package_for_lib(fs::path pt, string dll, string a, string dlla, string stdout_path_str, bool *found_lib = nullptr, bool library_not_found_anywhere = false)
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
            else return_code = 1;

        }

    }

    return return_code;

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
                            if(i.substr(0,3) != "lib")
                            {
                                l_name_dll =  i + ".dll";
                                l_name_a = i + ".a"; 
                                l_name_dll_a =  i + ".dll.a";
                               //l_name_dll_a =  i + ".lib";
                            }
                            else
                            {
                                l_name_dll =  i + ".dll";
                                l_name_a =  i + ".a"; 
                                l_name_dll_a =  i + ".dll.a";
                                //l_name_dll_a =  i + ".lib";

                            }
                            if(flag) //не использовать дефолтные пути
                            {
                                bool found_lib = false;
                                if(paths.size() != 0)
                                {
                                    cout << "Поиск не осуществляется в дефолтных путях\n";
                                    for(auto j : paths)
                                        if(find_install_package_for_lib(j, l_name_dll, l_name_a, l_name_dll_a, stdout_path_str, &found_lib) == 0)
                                            break;
                                    if(!found_lib)
                                    {
                                        cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                                        for(auto j : paths)
                                        {
                                            cout << "Поиск библиотеки " << j.string() + "/" + i << endl;
                                            if(find_install_package_for_lib(j, l_name_dll, l_name_a, l_name_dll_a, stdout_path_str, NULL, true) == 0)
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
                                    if(find_install_package_for_lib(j, l_name_dll, l_name_a, l_name_dll_a, stdout_path_str, &found_lib) == 0)
                                        break;

                                }

                                if(!found_lib)
                                {
                                    cout << "Библиотека " << i << " не найдена не в одном из путей. Попытка найти ее и доставить в нужный каталог\n";
                                    for(auto j : all_paths)
                                    {
                                        cout << "Поиск библиотеки " << j.string() + "/" + i << endl;
                                        if(find_install_package_for_lib(j, l_name_dll, l_name_a, l_name_dll_a, stdout_path_str, NULL, true) == 0)
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

    //cmake_trace();

    chdir(builddir.c_str());

    auto vars = {"CFLAGS", "CXXFLAGS"};
    auto incdir = "C:\\Windows\\Temp\\archives\\oicb-master\\include";
    TCHAR evbuf_in[32768], evbuf_out[32768];
    ssize_t outlen;
    for (auto var : vars) {
        auto evlen = GetEnvironmentVariable(var, evbuf_in, sizeof(evbuf_in));
        if (evlen)
            outlen = snprintf(evbuf_out, sizeof(evbuf_in), "%s -I%s", evbuf_in, incdir);
        else
            outlen = snprintf(evbuf_out, sizeof(evbuf_in), "-I%s", incdir);
        //if (outlen >= sizeof(evbuf))
        //    throw new ...;   // не хватило места
    SetEnvironmentVariable(var, evbuf_out);
}
    sum_code = sum_code || run_command(("\"C:\\msys64\\ucrt64\\bin\\cmake.exe\" -DBSD_ROOT=C:\\msys64\\usr -DCMAKE_POLICY_DEFAULT_CMP0074=NEW -G \"MinGW Makefiles\"  " +  unpack_dir.string() + "/CMakeLists.txt").c_str());

    sum_code = sum_code || run_command("\"C:\\msys64\\ucrt64\\bin\\mingw32-make.exe\" VERBOSE=1");

    chdir(curr_path.string().c_str());

    return sum_code;  
    

}


int main(int argc, char *argv[])
{

    if(argc == 1)
    {
        cerr << "Не указан путь к архиву!\n";
        exit(1);

    }

   // SetEnvironmentVariable(_T("LC_ALL"), _T("ASCII"));

    string path_to_archive = argv[1];
    string dirBuild = "C:\\Windows\\Temp\\archives";

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

    archive_name = arch;
            
    fs::path path3("C:\\Windows\\Temp\\archives");
    path3 /= arch;
    path3 /= "build-"+arch;

    //cout << path3 << endl;
    //template_incorrect_path = "^(.*/)?\\.\\./";
    fs::create_directories(path3);

    unpack_dir = "C:\\Windows\\Temp\\archives\\" + arch + "\\" + arch;
    build_dir = "C:\\Windows\\Temp\\archives\\" + arch + "\\build-" + arch;
    cout << unpack_dir << endl;
    cout << build_dir << endl; 


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

        if(run_command(("C:\\Windows\\System32\\tar.exe -tf " +  path_to_archive).c_str(),true) != 0)
        {
            cerr<<"Подозрительные файлы в архиве"<<"\n";
            exit(1);
        }
        else
        {   
            run_command(("C:\\Windows\\System32\\tar.exe " + unpack_cmd[1] + " " + path_to_archive + " " + "-C " + dirBuild + "\\" + arch).c_str());
            cout<<"Архив разархивирован"<<"\n";
        }

        if(fs::exists(unpack_dir/"CMakeLists.txt") && assembly_cmake() == 0)
        {
            cout << "Собрано с помощью CMake\n";
        
        }


    }

    return 0;
}
