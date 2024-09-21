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
#include <filesystem>
#if defined(__linux__) || defined(__unix__)
    #include <sys/types.h>
    #include <sys/wait.h>
    #include "opensuse.h"
    #include "ubuntu.h"
    #include "freebsd.h"
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #include <windows.h>
    #include <conio.h>
    #include <tchar.h>
    #include <dir.h>
    #include <nlohmann\json.hpp>
    #include "windows.h"
#endif

#include "os.h"

using namespace std;


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
