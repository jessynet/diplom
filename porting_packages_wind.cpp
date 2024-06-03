#include <iostream>
#include <fstream>
#include <windows.h>
#include <conio.h>
#include <regex>
#include <string>
#include <list>
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
        TCHAR path_file[] = _T("C:\\Windows\\Temp\\archives\\xz-master\\stdout.txt");
        //TCHAR path_file[] = _T("C:\\Windows\\Temp\\archives\\oicb-master\\stdout.txt");
        string pathtmp = "C:\\Windows\\Temp\\archives\\" + archive_name + "\\trace-output.txt";
        TCHAR *path_trace = new TCHAR[pathtmp.size() + 1];
        strcpy(path_trace, pathtmp.c_str());

        //char path_file[] = "C:\\Windows\\Temp\\archives\\oicb-master\\stdout.txt";
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;
        //перенаправление stdout в файл
        HANDLE h;
        if(trace)
        {
            h = CreateFile(path_trace, FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            si.dwFlags |= STARTF_USESTDHANDLES;
            si.hStdInput = NULL;
            si.hStdError = h;
            si.hStdOutput = NULL;
        }
        else
        {
            h = CreateFile(LPTSTR(path_file), FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            si.dwFlags |= STARTF_USESTDHANDLES;
            si.hStdInput = NULL;
            si.hStdError = NULL;
            si.hStdOutput = h;
        }

        if(h == INVALID_HANDLE_VALUE)
        {
            cout << "Ошибка дескриптора" << endl;
        }
       
        //создаем новый процесс
        if(!CreateProcess(NULL, LPTSTR(cmd), NULL, NULL, TRUE, 0, NULL, NULL, &si, &piCom)) //если выполняется успешно, то возвращается ненулевое значение
        //если функция выполняется неудачно, то возвращается нулевое значение
        {
            //Закрываем открытый дескриптор файла перед удалением
            CloseHandle(h);
            //Удаляем файл
            if(!trace) DeleteFile(path_trace);
            else DeleteFile(path_file);
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
                ifstream file(path_file); //поток для чтения 
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
                DeleteFile(path_file);

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
    cerr << "Running command {" << cmd << "} in {"<< curr_path << "}" << endl; 
    STARTUPINFO si;
    PROCESS_INFORMATION piCom;
    DWORD dwExitCode;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    int return_code;
    

    TCHAR path_file[] = _T("C:\\Windows\\Temp\\archives\\xz-master\\pacman_stdout.txt");
    
    //TCHAR path_file[] = "C:\\Windows\\Temp\\archives\\oicb-master\\pacman_stdout.txt";
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    //перенаправление stdout в файл
    HANDLE h = CreateFile(path_file, FILE_GENERIC_READ | FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(h == INVALID_HANDLE_VALUE)
    {
        cout << "Ошибка дескриптора" << endl;
        return 1;
    }
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = NULL;
    si.hStdError = NULL;
    si.hStdOutput = h;
        
    //создаем новый процесс
    if(!CreateProcess(NULL, LPTSTR(cmd), NULL, NULL, TRUE, 0, NULL, NULL, &si, &piCom)) //если выполняется успешно, то возвращается ненулевое значение
    //если функция выполняется неудачно, то возвращается нулевое значение
    {
        //Закрываем открытый дескриптор файла перед удалением
        CloseHandle(h);
        //Удаляем файл
        DeleteFile(path_file);
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

        //ifstream file(path_file); //поток для чтения 
        //string s;
        //int code = 0;

        //while(getline(file,s))
        //{
        //    cout << s;

       // }

        //file.close();

        //Закрываем открытый дескриптор файла перед удалением
        CloseHandle(h);

        //Удаляем файл
        //DeleteFile(LPTSTR(path_file));
        return_code = 1;


    }
           
    if(return_code) return 0;
    return 1;
   
 
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


void cmake_trace()
{
    string link_directories;
    vector <string> link_dirs;
    string unpackdir = unpack_dir.generic_string();
    chdir(unpackdir.c_str());

    vector <string> libs_path = {"C:/msys64/ucrt64/lib/gcc/x86_64-w64-mingw32/13.2.0",
                                "C:/msys64/ucrt64/lib/gcc",
                                "C:/msys64/ucrt64/x86_64-w64-mingw32/lib",
                                "C:/msys64/ucrt64/lib"};

    //curr_path = fs::current_path();
    
    chdir(unpackdir.c_str());

    run_command({"\"C:\\msys64\\ucrt64\\bin\\cmake.exe\" --trace-format=json-v1"},true, true);
    string pathtmp = "C:\\Windows\\Temp\\archives\\" + archive_name + "\\trace-output.txt";
    fstream in;  //поток для чтения
    in.open(pathtmp);
    map <string, string> cmd_set;
    string line;
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
                    for(vector <string> elem : names)
                    {
                        for(string i : elem)
                        {
                            if(i.substr(0,3) != "lib")
                                run_command_pacman(("C:\\msys64\\usr\\bin\\pacman.exe -Ss mingw-w64-ucrt-x86_64-" + i).c_str());
                                
                        }

                    }
                }
            }
        }
    }


}


int assembly_cmake()
{
    int sum_code = 0;
    string builddir = build_dir.generic_string();
    chdir(builddir.c_str());
    fs::path curr_path{fs::current_path()};
    fs::path tmp{fs::current_path()};
    uintmax_t n{fs::remove_all(tmp / "*")};

    tmp /= ".cmake";
    tmp /= "api";
    tmp /= "v1";
    tmp /= "query";
    fs::create_directories(tmp);

    string ttmp = tmp.generic_string();

    chdir(ttmp.c_str());

    char path_codemodel[] = "codemodel-v2";
    HANDLE h1 = CreateFile(LPTSTR(path_codemodel), FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CloseHandle(h1);

    char path_toolchain[] = "toolchains-v1";
    HANDLE h2 = CreateFile(LPTSTR(path_toolchain), FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CloseHandle(h2);

    //chdir(builddir.c_str());

    string unpackdir = unpack_dir.generic_string();

    chdir(unpackdir.c_str());

    cmake_trace();

    chdir(builddir.c_str());

    sum_code = sum_code || run_command(("\"C:\\msys64\\ucrt64\\bin\\cmake.exe\" -G \"MinGW Makefiles\" " +  unpack_dir.generic_string() + "/CMakeLists.txt").c_str());

    sum_code = sum_code || run_command("\"C:\\msys64\\ucrt64\\bin\\mingw32-make.exe\"");

    chdir(curr_path.generic_string().c_str());

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
    string type_arch_tmp_one = path1.extension().generic_string();
    size_t pos = path_arch.find(type_arch_tmp_one);
    string path_arch_tmp = path_arch.substr(0, pos);
    fs::path path2(path_arch_tmp);
    if(path2.extension()!=".tar") 
    {   
        type_arch = type_arch_tmp_one;
    }
    else
    {
        string type_arch_tmp_two = path2.extension().generic_string();
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
