#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <array>
#include <experimental/filesystem>
using namespace std;
namespace fs = std::experimental::filesystem;

string os;
string unpack_dir;
string build_dir;

/*class package
{
    public:
        string path;
        string arch_name;
        string os;
        string unpack_dir;
        string build_dir;


        package(string os_type, string full_path)
        {
            setData(os_type, full_path);
        }

        void setData(string os_type, string full_path)
        {
            os = os_type;
            path = full_path;

        }

        string getOS()
        {
            return os;

        }

        string getPath()
        {
            return path;

        }

        string getAN()
        {
            return arch_name;

        }

        string getUD()
        {
            return unpack_dir;
            
        }

        string getBD()
        {
            return build_dir;
            
        }


};*/

int returnCode(string command)
{
    return(system(command.c_str()));
}

void doCommand(string command)
{
    system(command.c_str());

}

void installation (string package)
{
    if(returnCode("command -v " + package + " >/dev/null") != 0)
    {
        string install;
        if(os=="opensuse") install = "sudo zypper install -y " + package;
        else if(os=="ubuntu") install = "sudo apt install -y " + package;
        else if(os=="freebsd") install = "sudo pkg install -y " + package;
        if(returnCode(install)==0)
            cout<<package<<" : установлено"<<"\n";
        else
        {
            cout<<"Не удалось установить: "<<package;
            exit(0);
        }
    }
}

void check_dependencies(string arch_name, string path)
{
    string command("sudo cpanm --installdeps " + unpack_dir+ " 2>"+"/tmp/archives/" + arch_name + "/log.txt");

    FILE* pipe = popen(command.c_str(), "w");
    pclose(pipe);

    string s;
    ifstream file(unpack_dir +"/../log.txt");
    regex reg(".*(is not installed).*",regex_constants::icase);
    while(getline(file,s)){
        if(regex_match(s.c_str(),reg) == true)
        {
            smatch match;
            string t;
            regex ree(R"(\'(.*)\')");
            if(regex_search(s,match,ree))
            {
                t = match[1];
                size_t pos = t.find(":");
                string t1 = t.substr(0,pos);
                string t2 = t.substr(pos+2, t.size()-1);
                string test_command;
                if(os=="freebsd") test_command = "sudo pkg install p5-" + t1 + "-" + t2;
                else if(os=="opensuse") test_command = "sudo zypper install perl-" + t1 + "-" + t2;
                else if(os=="ubuntu") //Не придумала, как провернуть такое же на ubuntu ибо там пакет называется совсем иначе
                {
                    transform(t1.begin(),t1.end(),t1.begin(), ::tolower);
                    transform(t2.begin(),t2.end(),t2.begin(), ::tolower);
                    test_command = "sudo apt install lib" + t1 + "-" + t2 + "-" + "perl";
                }
                system(test_command.c_str());
            }
        
        }
    }

    file.close();

}


int assembly_cmake (string arch_name)
{   
    installation("cmake");
    //Все в одной команде,так как вызовы system независимы и каждый раз при вызове будет получаться новая среда
    //rm -r пока без ключа -f, который позволяет совершать удаление без запроса подтверждения
    //-r рекурсивно
    string assembly;
    if(os=="opensuse") 
        assembly = "cd " + build_dir + " && rm -r ./*" + " && cmake " + unpack_dir + " && make";
    else if(os=="ubuntu") 
    {
        doCommand("sudo apt install -y libncurses-dev libreadline-dev libbsd-dev");
        assembly = "cd " + build_dir + " && rm -r ./*" + " && cmake " + unpack_dir + " && make";
    }
    else if(os=="freebsd")
    {
        installation("bash");
        assembly = "cd " + build_dir + " && cmake " + unpack_dir + " && make";
    }
    return returnCode(assembly);
}

int assembly_autotools (string arch_name)
{
    installation("autoconf");
    installation("automake");
    return returnCode("cd " + unpack_dir + " && ./configure" + " && make" + " && cp -r ./* " + build_dir);
}

void install_gems (string arch_name, string path)
{
    doCommand("cp -r " + path + " " + build_dir);
    doCommand("sudo gem install " + path);    
}

int assembly_gem (string arch_name, string path)
{
    installation("git");
    return returnCode("cd " + path + " && gem build ./*.gemspec && cp -r ./*.gem " + build_dir);
}

int assembly_php (string arch_name)
{
    int rc;
    if(os=="opensuse") 
    {
        doCommand("sudo zypper install -y php7 php7-devel php7-pecl php7-pear");
        rc = returnCode("cd " + unpack_dir + " && /usr/bin/phpize" + " && ./configure" + " && make");

    }
    else if(os=="ubuntu")
    {
        doCommand("sudo apt install -y php php-dev php-pear");
        rc = returnCode("cd " + unpack_dir + " && /usr/bin/phpize" + " && ./configure" + " && make");
    } 
    else if(os=="freebsd")
    {
        doCommand("sudo pkg install-y php83 php83-pear php83-session php83-gd");
        rc = returnCode("cd " + unpack_dir + " && /usr/local/bin/phpize" + " && ./configure" + " && make");
    } 
    doCommand("cp -r " + unpack_dir + "/*" + " " + build_dir);
    return rc;
}

int assembly_perl_build (string arch_name,string path)
{
    check_dependencies(arch_name,path);
    doCommand("sudo cpan Module::Build");
    if(os=="opensuse")
    {
        installation("perl");
        doCommand("sudo zypper install -y perl-App-cpanminus");
    } 
    else if(os=="ubuntu")
    {
        installation("perl");
        doCommand("sudo apt install -y cpanminus");
    } 
    //string build = "cd " + unpack_path + " && perl Build.PL" + " && sudo ./Build installdeps" + " && ./Build";
    else if(os=="freebsd")
    {
        installation("perl5");
        doCommand("sudo pkg install -y p5-App-cpanminus");
    } 
    int rc = returnCode("cd " + unpack_dir + " && sudo cpanm --installdeps . && sudo perl Build.PL && sudo ./Build");
    doCommand("sudo cp -r " + unpack_dir + "/*" + " " + build_dir);
    return rc;

}

int assembly_perl_make (string arch_name)
{
    if(os=="opensuse")
    {
        installation("perl");
        doCommand("sudo zypper install -y libtirpc-devel");
    } 
    else if(os=="ubuntu") 
    {
        installation("perl");
        doCommand("sudo apt install -y libtirpc-dev");
    }
    else if(os=="freebsd") installation("perl5");
    int rc = returnCode("cd " + unpack_dir + " && perl ./Makefile.PL" + " && make");
    doCommand("sudo cp -r " + unpack_dir + "/*" + " " + build_dir);
    return rc;

}

int empty_gemspec (string path)
{
    string command("find " + path + " -type f -name \"*.gemspec\" 2>&1");
    array<char, 128> buffer;
    FILE* pipe = popen(command.c_str(), "r");
    int rc;
    
    if (!pipe)
    {
        cerr << "Не удалось выполнить команду" << endl;
        return 0;
    }
    if(fgets(buffer.data(), 128, pipe) != NULL) //Если найден файл ./gemspec
    {
        rc = 0;             
    }
    else //Если нет файла ./gemspec
    {
        rc = 1;
    }
    
    pclose(pipe);
    return rc;
}


int main(int argc, char *argv[])
{
    // cout<<argv[1]; //Первый переданный аргумент программе
    if (argc == 1)
    {
        cout << "Не указан путь к архиву\n";
        exit(0);
    }

    #if defined(__linux__)
        //linux code 
        ifstream in("/etc/os-release");
        string os_name;
        if(in.is_open())
        {
            regex r("^(NAME=).*");
        
            while(getline(in,os_name))
            {
                if(regex_match(os_name.c_str(),r) == true)
                {
                    break;
                }
            }
        }

        in.close();

        os = os_name.substr(5, os_name.size()-5);
        regex reg1(".*(opensuse).*",regex_constants::icase);
        regex reg2(".*(ubuntu).*",regex_constants::icase);
        if(regex_match(os.c_str(),reg1) == true)
        {   
            os = "opensuse";
        }
        else if(regex_match(os.c_str(),reg2) == true)
        {
            os = "ubuntu";
        }
    #elif defined(__unix__)
        //unix code
        string command("uname -a");
        array<char, 200> buffer;
        string result;
        FILE* pipe = popen(command.c_str(),"r");
        while (fgets(buffer.data(), 200, pipe) != NULL)
        {
            result+= buffer.data();
        }
        pclose(pipe);
        regex reg3(".*(freebsd).*\n",regex_constants::icase);
        if(regex_match(result.c_str(),reg3) == true){os="freebsd";}
    #elif defined(__APPLE__)
        //apple code(other kinds of apple platforms)    
    #elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        //define something for Windows (32-bit and 64-bit, this part is common)
        /* _Win32 is usually defined by compilers targeting 32 or   64 bit Windows systems */
        #ifdef _WIN64
            //define something for Windows (64-bit only)
        #else
            //define something for Windows (32-bit only)
        #endif
    
    #endif    

    //это блок кода поместить в try-catch

    string path_arch = argv[1];
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
    cout << arch << "  " << type_arch << "\n";
    
    fs::create_directory("/tmp/archives");
    fs::path path3("/tmp/archives");
    path3 /= arch;
    fs::create_directory(path3);
    path3 /= "build-"+arch;
    fs::create_directory(path3);

    unpack_dir = "/tmp/archives/" + arch + "/" + arch;
    build_dir = "/tmp/archives/" + arch + "/build-" + arch;

    installation("tar");

    string unpack_cmd;
    int archiver;
    if(type_arch == ".tar")
    {
        unpack_cmd = "tar -xvf";  
        archiver = 1;      
    }
    else if(type_arch == ".tar.bz2" || type_arch == ".tar.bzip2" || type_arch == ".tb2" || type_arch == ".tbz")
    {
        unpack_cmd = "tar -xvjf";
        archiver = 1;  
    }
    else if(type_arch == ".tar.xz" || type_arch == ".txz")
    {
        unpack_cmd = "tar -xvJf";
        archiver = 1;  

    }
    else if(type_arch == ".tar.gz" || type_arch == ".tgz")
    {
        unpack_cmd = "tar -xvzf";
        archiver = 1;  

    }
    else if(type_arch == ".gem")
    {
        installation("ruby");
        if(os=="opensuse") doCommand("sudo zypper install -y ruby-devel");
        else if(os=="ubuntu") doCommand("sudo apt install -y ruby-dev");
	    else if(os=="freebsd") doCommand("sudo pkg install-y rubygem-grpc");
        install_gems(arch,path_arch);
        archiver = 2;  

    }

    if(archiver == 1)
    {   
        string cmd = "tar -tf " + path_arch + " | egrep -q '^(.*/)?\\.\\./'";
        if(returnCode(cmd.c_str()) == 0)
        {
            cout<<"Подозрительные файлы в архиве"<<"\n";
        }
        else
        {   
            doCommand(unpack_cmd + " " + path_arch + " -C " + "/tmp/archives/" + arch);
            cout<<"Архив разархивирован"<<"\n";
        }

    }

    if(os=="ubuntu" || os=="opensuse") installation("make");

    if(archiver!=2)
    {
        if(fs::exists(unpack_dir + "/CMakeLists.txt") && assembly_cmake(arch) == 0) //CMake
        {
            cout<<"Собрано с помощью CMake"<<"\n";
            doCommand("cd " + build_dir + " && sudo make -n install"); 
        }
        else if(fs::exists(unpack_dir + "/configure") && fs::exists(unpack_dir + "/Makefile.in")  //GNU Autotools
                && fs::exists(unpack_dir + "/config.h.in")  && assembly_autotools(arch) == 0)
        {   
            cout<<"Собрано с помощью GNU Autotools"<<"\n";
            doCommand("cd " + unpack_dir + " && sudo make -n install"); 
        }
        else if(!empty_gemspec(unpack_dir) && assembly_gem(arch, unpack_dir) == 0) //Ruby
        {
            cout<<"Собрано с помощью gem"<<"\n";
            doCommand("sudo gem install " + unpack_dir + "/*.gem");
        }
        else if(fs::exists(unpack_dir + "/Rakefile") && empty_gemspec(unpack_dir)) //Ruby
        {
            doCommand("sudo gem install rake rake-compiler");
            if(returnCode("cd " + unpack_dir + " && rake --all -n") == 0) cout<<"Прогон всех шагов завершен успешно"<<"\n";    
        }
        else if(fs::exists(unpack_dir + "/Build.PL") && assembly_perl_build(arch,path_arch) == 0) //Perl
        {
            cout<<"Собрано с помощью Build и Build.PL"<<"\n";
            doCommand("cd " + unpack_dir + " && sudo ./Build install");
        }
        else if(fs::exists(unpack_dir + "/Makefile.PL") && assembly_perl_make(arch) == 0) //Perl
        {
            cout<<"Собрано с помощью make и Makefile.PL"<<"\n";
            doCommand("cd " + unpack_dir + " && sudo make -n install");
        }
        else if(fs::exists(unpack_dir + "/config.m4") && assembly_php(arch) == 0) //PHP
        {
            cout<<"Собрано с помощью phpize и GNU Autotools"<<"\n";
            doCommand("cd " + unpack_dir + " && sudo make -n install");

            string cmd_terminal;
            if(os=="opensuse")
                cmd_terminal = "cd " + unpack_dir + 
                "/modules && module=$(find *.so) && module_name=${module%.*} && echo 'extension=$module' | sudo tee /etc/php7/conf.d/$module_name.ini >/dev/null";
            else if(os=="ubuntu")
                cmd_terminal = "cd " + unpack_dir + 
                "/modules && module=$(find *.so) && module_name=${module%.*} && echo 'extension=$module' | sudo tee /etc/php/8.1/mods-available/$module_name.ini >/dev/null";
            doCommand(cmd_terminal);

        }
        else cout<<"Не удалось собрать пакет"<<"\n";
    }
}
