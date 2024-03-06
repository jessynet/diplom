#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <experimental/filesystem>
using namespace std;
namespace fs = std::experimental::filesystem;

string os;

void installation (string package)
{
    string find = "command -v " + package + " >/dev/null";
    const char *command_find = find.c_str();
    if(system(command_find) != 0)
    {
        string install;
        if(os=="opensuse") install = "sudo zypper install -y " + package;
        else if(os=="ubuntu") install = "sudo apt install -y " + package;
        const char *command_install = install.c_str();
        cout<<package<<" : установлено"<<"\n";
    }
}

int assembly_cmake (string arch_name)
{   
    installation("cmake");
    string build_path = "/tmp/archives/" + arch_name + "/build-" + arch_name;
    string unpack_path = "/tmp/archives/" + arch_name + "/" + arch_name;
    //Все в одной команде,так как вызовы system независимы и каждый раз при вызове будет получаться новая среда
    //rm -r пока без ключа -f, который позволяет совершать удаление без запроса подтверждения
    //-r рекурсивно
    string assembly;
    if(os=="opensuse") 
        assembly = "cd " + build_path + " && rm -r ./*" + " && cmake " + unpack_path + " && make";
    else if(os=="ubuntu") 
    {
        system("sudo apt install -y libncurses-dev libreadline-dev libbsd-dev");
        assembly = "cd " + build_path + " && rm -r ./*" + " && cmake " + unpack_path + " && make";
    }
    const char *command_assembly = assembly.c_str();
    int returnCode = system(command_assembly);
    return returnCode;
}

int assembly_autotools (string arch_name)
{
    installation("autoconf");
    installation("automake");
    string build_path = "/tmp/archives/" + arch_name + "/build-" + arch_name;
    string unpack_path = "/tmp/archives/" + arch_name + "/" + arch_name;
    string assembly = "cd " + unpack_path + " && ./configure" + " && make" + " && cp -r ./* " + build_path;
    const char *command_assembly = assembly.c_str();
    int returnCode = system(command_assembly);
    return returnCode;
}

void install_gems (string arch_name, string path)
{
    string build_path = "/tmp/archives/" + arch_name + "/build-" + arch_name;
    string copy = "cp -r " + path + " " + build_path;
    string install = "sudo gem install " + path;
    const char *command_copy = copy.c_str();
    const char *command_install = install.c_str();
    system(command_copy);
    system(command_install);    
}

int assembly_gem (string arch_name, string path)
{
    string build_path = "/tmp/archives/" + arch_name + "/build-" + arch_name;
    string build = "cd " + path + " && gem build ./*.gemspec && cp -r ./*.gem " + build_path;
    const char *command_build = build.c_str();
    int returnCode = system(command_build);
    return returnCode;
}

int assembly_php (string arch_name)
{
    if(os=="opensuse") system("sudo zypper install -y php7 php7-devel php7-pecl php7-pear");
    else if(os=="ubuntu") system("sudo apt install -y php php-dev php-pear");
    string build_path = "/tmp/archives/" + arch_name + "/build-" + arch_name;
    string unpack_path = "/tmp/archives/" + arch_name + "/" + arch_name;
    string build = "cd " + unpack_path + " && /usr/bin/phpize" + " && ./configure" + " && make";
    string copy = "cp -r " + unpack_path + "/*" + " " + build_path;
    const char *command_build = build.c_str();
    const char *command_copy = copy.c_str();
    int returnCode = system(command_build);
    system(command_copy);
    return returnCode;
}

int assembly_perl_build (string arch_name)
{
    installation("perl");
    system("sudo cpan Module::Build");
    if(os=="opensuse") system("sudo zypper install -y perl-App-cpanminus");
    else if(os=="ubuntu") system("sudo apt install -y cpanminus");
    string unpack_path = "/tmp/archives/" + arch_name + "/" + arch_name;
    string build_path = "/tmp/archives/" + arch_name + "/build-" + arch_name;
    string build = "cd " + unpack_path + " && sudo cpanm --installdeps . && sudo perl Build.PL && sudo ./Build";
    //string build = "cd " + unpack_path + " && perl Build.PL" + " && sudo ./Build installdeps" + " && ./Build";
    string copy = "sudo cp -r " + unpack_path + "/*" + " " + build_path;
    const char *command_build = build.c_str();
    const char *command_copy = copy.c_str();
    int returnCode = system(command_build);
    system(command_copy);
    return returnCode;

}

int assembly_perl_make (string arch_name)
{
    installation("perl");
    if(os=="opensuse") system("sudo zypper install -y libtirpc-devel");
    else if(os=="ubuntu") system("sudo apt install -y libtirpc-dev");
    string unpack_path = "/tmp/archives/" + arch_name + "/" + arch_name;
    string build_path = "/tmp/archives/" + arch_name + "/build-" + arch_name;
    string build = "cd " + unpack_path + " && perl ./Makefile.PL" + " && make";
    string copy = "sudo cp -r " + unpack_path + "/*" + " " + build_path;
    const char *command_build = build.c_str();
    const char *command_copy = copy.c_str();
    int returnCode = system(command_build);
    system(command_copy);
    return returnCode;

}

int empty_gemspec (string path)
{
    string find_gem = "cd " + path + " && find . -type f -name \"*.gemspec\" > tmp.txt";
    const char *command_gem = find_gem.c_str();
    system(command_gem);
    int returnCode = fs::is_empty(path + "/tmp.txt");
    string remove_path = path + "/tmp.txt";
    const char *rm_path = remove_path.c_str();
    remove(rm_path);
    return returnCode;
}


int main(int argc, char *argv[])
{
    // cout<<argv[1]; //Первый переданный аргумент программе
    if (argc == 1)
    {
        cout << "Не указан путь к архиву\n";
        exit(0);
    }

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
    regex reg3(".*(freebsd).*",regex_constants::icase);
    if(regex_match(os.c_str(),reg1) == true)
    {
        os = "opensuse";
    }
    else if(regex_match(os.c_str(),reg2) == true)
    {
        os = "ubuntu";
    }
    else if(regex_match(os.c_str(),reg3) == true)
    {
        os = "freebsd";
    }

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
        if(os=="opensuse") system("sudo zypper install -y ruby-devel");
        else if(os=="ubuntu") system("sudo apt install -y ruby-dev");
        install_gems(arch,path_arch);
        archiver = 2;  

    }

    if(archiver == 1)
    {   
        string cmd = "tar -tf " + path_arch + " | egrep -q '^(.*/)?\\.\\./'";
        const char *command_check = cmd.c_str();
        int returnCode = system(command_check);
        if(returnCode == 0)
        {
            cout<<"Подозрительные файлы в архиве"<<"\n";
        }
        else
        {   
            string unpack = unpack_cmd + " " + path_arch + " -C " + "/tmp/archives/" + arch;
            const char *command_unpack = unpack.c_str();
            system(command_unpack);
            cout<<"Архив разархивирован"<<"\n";
        }

    }

    installation("make");

    string unpack_dir = "/tmp/archives/" + arch + "/" + arch;
    string build_dir = "/tmp/archives/" + arch + "/build-" + arch;
    if(archiver!=2)
    {
        if(fs::exists(unpack_dir + "/CMakeLists.txt") && assembly_cmake(arch) == 0) //CMake
        {
            cout<<"Собрано с помощью CMake"<<"\n";
            string cmd_cmake = "cd " + build_dir + " && sudo make -n install";
            const char *command_cmake = cmd_cmake.c_str();
            system(command_cmake); 
        }
        else if(fs::exists(unpack_dir + "/configure") && fs::exists(unpack_dir + "/Makefile.in")  //GNU Autotools
                && fs::exists(unpack_dir + "/config.h.in")  && assembly_autotools(arch) == 0)
        {   
            cout<<"Собрано с помощью GNU Autotools"<<"\n";
            string cmd_gnu = "cd " + unpack_dir + " && sudo make -n install";
            const char *command_gnu = cmd_gnu.c_str();
            system(command_gnu); 
        }
        else if(!empty_gemspec(unpack_dir) && assembly_gem(arch, unpack_dir) == 0) //Ruby
        {
            cout<<"Собрано с помощью gem"<<"\n";
            string cmd_gem = "sudo gem install " + unpack_dir + "/*.gem";
            const char *command_gem = cmd_gem.c_str();
            system(command_gem);
        }
        else if(fs::exists(unpack_dir + "/Rakefile") && empty_gemspec(unpack_dir)) //Ruby
        {
            system("sudo gem install rake rake-compiler");
            string cmd_rake = "cd " + unpack_dir + " && rake --all -n";
            const char *command_rake = cmd_rake.c_str();
            if(system(command_rake) == 0) cout<<"Прогон всех шагов завершен успешно"<<"\n";    
        }
        else if(fs::exists(unpack_dir + "/Build.PL") && assembly_perl_build(arch) == 0) //Perl
        {
            cout<<"Собрано с помощью Build и Build.PL"<<"\n";
            string cmd_perl= "cd " + unpack_dir + " && sudo ./Build install";
            const char *command_perl = cmd_perl.c_str();
            system(command_perl);
        }
        else if(fs::exists(unpack_dir + "/Makefile.PL") && assembly_perl_make(arch) == 0) //Perl
        {
            cout<<"Собрано с помощью make и Makefile.PL"<<"\n";
            string cmd_perl= "cd " + unpack_dir + " && sudo make -n install";
            const char *command_perl = cmd_perl.c_str();
            system(command_perl);
        }
        else if(fs::exists(unpack_dir + "/config.m4") && assembly_php(arch) == 0) //PHP
        {
            cout<<"Собрано с помощью phpize и GNU Autotools"<<"\n";
            string cmd_php= "cd " + unpack_dir + " && sudo make -n install";
            const char *command_php = cmd_php.c_str();
            system(command_php);

            string cmd_terminal;
            if(os=="opensuse")
                cmd_terminal = "cd " + unpack_dir + 
                "/modules && module=$(find *.so) && module_name=${module%.*} && echo 'extension=$module' | sudo tee /etc/php7/conf.d/$module_name.ini >/dev/null";
            else if(os=="ubuntu")
                cmd_terminal = "cd " + unpack_dir + 
                "/modules && module=$(find *.so) && module_name=${module%.*} && echo 'extension=$module' | sudo tee /etc/php/8.1/mods-available/$module_name.ini >/dev/null";
            const char *command_terminal = cmd_terminal.c_str();
            system(command_terminal);

        }
        else cout<<"Не удалось собрать пакет"<<"\n";
    }
}
