#include <iostream>
#include <fstream>
#include <windows.h>
#include <conio.h>
#include <regex>
#include <string>
#include <algorithm>
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
        else //прямое подключение name.a || name.dll || name.lib || name.dll.a
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

//void add_c_compile_options(const char *opts) {
 //   auto vars = {"CFLAGS", "CXXFLAGS"};
 //   TCHAR evbuf_in[32767], evbuf_out[32767];
 //   ssize_t outlen;
  //  for (auto var : vars) {
 //       auto evlen = GetEnvironmentVariable(var, evbuf_in, sizeof(evbuf_in));

	// Format in Unix-style way, and get rid of colon:
	// C:/Foo becomes /C/Foo
 //   fs::path inc_dir {"C:\\Windows\\Temp\\archives\\oicb-master\\include"};
//	auto dir = inc_dir.string();
	//dir[1] = dir[0];
	//dir[0] = '/';

//        if (evlen)
 //           outlen = snprintf(evbuf_out, sizeof(evbuf_out), "%s %s", evbuf_in, opts);
 //       else
  //          outlen = snprintf(evbuf_out, sizeof(evbuf_out), "%s", opts);
//        if (outlen >= sizeof(evbuf_out))
//            throw new runtime_error("too long envvar");   // не хватило места
//	cerr << "setting " << var << " to [" << outlen << "]'" << evbuf_out << "'" << endl;
 //       if (!SetEnvironmentVariable(var, evbuf_out))
//		cerr << "FAILED!";
 //   }
//}

//void add_c_header_overlay(const char *name, const char *text) {
//    fs::path inc_dir {"C:\\Windows\\Temp\\archives\\oicb-master\\include"};
//    ofstream hf(inc_dir / name);    // TODO create subdirectories if needed
//    hf << text;
//    hf << "\n";
//}

//void add_c_header_overlay(const char *name, const char *text) {
   // fs::path inc_dir {"C:\\Windows\\Temp\\archives\\oicb-master\\include"};
    //ofstream hf(inc_dir / name);    // TODO create subdirectories if needed
    //hf << "#include_next <" << name << ">\n";
    //hf << text;
    //hf << "\n";
    
//}


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

    /*add_c_compile_options((string("-I") + inc_dir.string()).c_str());
    add_c_compile_options("-DNO_OLDNAMES");
    add_c_compile_options("-Dgid_t=int");
    add_c_compile_options("-Duid_t=int");

    add_c_header_overlay("unistd.h",
		    "#ifdef setmode\n"
		    "# undef setmode\n"
		    "#endif\n"
		    );

    DeleteFile((build_dir / "CMakeCache.txt").string().c_str());
    sum_code = sum_code || run_command(("\"C:\\msys64\\ucrt64\\bin\\cmake.exe\" -DBSD_ROOT=C:\\msys64\\usr -DCMAKE_POLICY_DEFAULT_CMP0074=NEW -G \"MinGW Makefiles\"  " +  unpack_dir.string() + "/CMakeLists.txt").c_str());

    sum_code = sum_code || run_command("\"C:\\msys64\\ucrt64\\bin\\mingw32-make.exe\" VERBOSE=1");

    chdir(curr_path.string().c_str()); */
    //add_c_compile_options((string("-I") + inc_dir.string()).c_str());
    //add_c_compile_options("-Dgid_t=int");
    //add_c_compile_options("-Duid_t=int");

    /*add_c_header_overlay("io.h",
                    "#ifdef NO_OLDNAMES\n"
                    "# define OVERLAY_HAD_NO_OLDNAMES\n"
                    "#else\n"
                    "# define NO_OLDNAMES\n"
                    "#endif\n"
                    "\n"
                    "#include_next <io.h>\n"
                    "\n"
                    "#ifdef OVERLAY_HAD_NO_OLDNAMES\n"
                    "# undef OVERLAY_HAD_NO_OLDNAMES\n"
                    "#else\n"
                    "# undef NO_OLDNAMES\n"
                    "#endif\n"
                    );*/

    //DeleteFile((build_dir / "CMakeCache.txt").string().c_str());

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
