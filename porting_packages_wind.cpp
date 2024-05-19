#include <iostream>
#include <fstream>
#include <windows.h>
#include <conio.h>
#include <regex>
#include <string>
#include <vector>
#include <tchar.h>
#include <dir.h>
#include <experimental/filesystem>

using namespace std;
namespace fs = std::experimental::filesystem;
fs::path curr_path;
int run_command(char* cmd, bool check_stdout = false)
{

    curr_path = fs::current_path();
    STARTUPINFO si;
    PROCESS_INFORMATION piCom;
    DWORD dwExitCode;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    int return_code;
    

    if(check_stdout)
    {
        char path_file[] = "C:\\Windows\\Temp\\archives\\oicb-master\\stdout.txt";
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;
        //перенаправление stdout в файл
        HANDLE h = CreateFile(LPTSTR(path_file), FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        
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
            DeleteFile(LPTSTR(path_file));
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
            DeleteFile(LPTSTR(path_file));

            if(code) return_code = 0;
            else
            {
                cout << "Подозрительных(с путями, которые при установке могут повредить систему) файлов в архиве не обнаружено\n";
                return_code = 1;
            } 
        }
    }
    else
        if(!CreateProcess(NULL, LPTSTR(cmd), NULL, NULL, FALSE, 0, NULL, NULL, &si, &piCom))
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

    if(return_code) return 0;
    return 1;
   
 
}


int main(int argc, char *argv[])
{

    if(argc == 1)
    {
        cerr << "Не указан путь к архиву!\n";
        exit(1);

    }

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
            
    fs::path path3("C:\\Windows\\Temp\\archives");
    path3 /= arch;
    path3 /= "build-"+arch;

    //cout << path3 << endl;
    //template_incorrect_path = "^(.*/)?\\.\\./";
    fs::create_directories(path3);

    string unpack_dir = "C:\\Windows\\Temp\\archives\\" + arch + "\\" + arch;
    string build_dir = "C:\\Windows\\Temp\\archives\\" + arch + "\\build-" + arch;
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
        string cmd = "C:\\Windows\\System32\\tar.exe -tf " +  path_to_archive;
        char check_archive[cmd.size()]; 
        strcpy(check_archive, cmd.c_str());
        //флаг -c говорит о том, что команды считываются из строки
        if(run_command(check_archive,true) != 0)
        {
            cerr<<"Подозрительные файлы в архиве"<<"\n";
            exit(1);
        }
        else
        {   
            string CommandLine = "C:\\Windows\\System32\\tar.exe " + unpack_cmd[1] + " " + path_to_archive + " " + "-C " + dirBuild + "\\" + arch;
            char lpszCommandLine[CommandLine.size()]; 
            strcpy(lpszCommandLine, CommandLine.c_str());
            run_command(lpszCommandLine);
            cout<<"Архив разархивирован"<<"\n";
        }



    }

    return 0;
}
