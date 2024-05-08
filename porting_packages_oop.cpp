#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <array>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <experimental/filesystem>
using namespace std;
namespace fs = std::experimental::filesystem;


class Os
{
    public:

        string current_path = fs::current_path();
        fs::path unpack_dir;
        fs::path build_dir;
        string archive_name;
        fs::path path_to_archive;
        string type_archive;
        vector<string> install;
        string ruby_package;
        fs::path dirBuild;
        string perl_package;
        string perl_package_make;
        vector <string> phpize;
        string devnull_path = "^(.*/)?\\.\\./";
    
        virtual int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false) {return 1;}

        virtual void installation(const vector <string> packages) {cout << "???";}

        void installation(string package) {vector<string> pkgs = {package}; installation(pkgs); }

        virtual void options() {cout << "???";}

        virtual void set_ruby_package() {cout << "???";}

        virtual void set_install() {cout << "???";}

        virtual void install_gems () {cout << "???";}

        virtual void cd(string path) {cout << "???";}

        virtual void set_dirBuild() {cout << "???";}

        virtual int assembly_cmake(string arch_name) {return 1;}

        virtual int assembly_autotools (string arch_name) {return 1;}

        virtual int gemspec_exists (string path) {return 1;}

        virtual string find_file(regex mask, fs::path pathToDir, bool return_name_file = false) {return "???";}

        virtual int assembly_gem (string arch_name, string path) {return 1;}

        virtual int assembly_perl_build (string arch_name,string path) {return 1;}

        virtual int assembly_perl_make(string arch_name) {return 1;}

        virtual int assembly_php (string arch_name) {return 1;}

        int get_line(int* stdout_pipe)
        {
            int return_code = 0;
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
            regex reg(devnull_path, regex::extended);
            while((linelen = getline(&line, &linesize, f)) != -1)
            {
                        
                if(regex_match(line, reg))
                {
                    return_code = 1;
                    break;
                    }   
            }
            free(line);
            fclose(f);

            return return_code;

        }
        
};

class Unix : public Os
{ 
    public:
        string os_name(string command) 
        {
            array<char, 200> buffer;
            string result;
            FILE* pipe = popen(command.c_str(),"r");
            while (fgets(buffer.data(), 200, pipe) != NULL)
            {
                result+= buffer.data();
            }
            pclose(pipe);
            return result;
        }

        virtual void options()
        {
            //current_path = fs::current_path();

            string path_arch = path_to_archive;
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
            archive_name = arch;
            type_archive = type_arch;
            cout << arch << "  " << type_arch << "\n";
            
            fs::path path3("/tmp/archives");
            path3 /= arch;
            path3 /= "build-"+arch;
            fs::create_directories(path3);

            unpack_dir = "/tmp/archives/" + arch + "/" + arch;
            build_dir = "/tmp/archives/" + arch + "/build-" + arch;

            cout << unpack_dir << endl;
            cout << build_dir << endl;
        }  

        virtual void set_dirBuild() 
        {
            dirBuild = "/tmp/archives";
        }

        bool is_admin() {
        #ifdef WIN32
            // TODO
            return true;
        #else
            return getuid() == 0; //краткая запись if(getuid()==0) {return true;} else {return false;}
        #endif
        }

        virtual int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false) 
        {
        #ifdef WIN32
            // CreateProcess()
            // CreateProcessAsUser()
            // ShellExecute...
        #else
            if (need_admin_rights && !is_admin())
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
                        if(hide_stderr)
                        {
                            int devNull = open("/dev/null", O_WRONLY);
                            dup2(devNull, STDERR_FILENO);
                        }
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
                    
                    execvp(c,args); //если удачно, то не возвращает управления,если ошибка-то вернет управление
                    cerr << "Не удалось выполнить комманду exec\n"; 
                    _exit(42);
                       
                }
                default: //pid>0 родительский процесс
                {
                    int status;
                        
                    if (stdout_pipe) return_code = Os::get_line(stdout_pipe);
        
                    //Если порожденный процесс, заданный параметром pid, к моменту системного вызова находится в 
                    //состоянии закончил исполнение, системный вызов возвращается немедленно без блокирования текущего процесса.   
                    do {
                        waitpid(pid, &status, 0);
                    } while(!WIFEXITED(status) && !WIFSIGNALED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
                    //WIFSIGNALED возвращает истинное значение, если дочерний процесс завершился из-за необработанного сигнала.
                    //ожидание изменения состояния процесса-потомка 
                    //завершается либо если корректно завершлся дочерний процесс
                    //либо если он был прерван сигналом
                    int child_status;
                    if(WEXITSTATUS(status) == 0) child_status = 0;
                    else child_status = 1;
                    return_code = return_code || child_status; 
                }   
            }
        #endif
            return return_code;
        } 
        
        virtual void install_gems ()
        {
            run_command({"cp", "-r", path_to_archive ,build_dir});
            run_command({"gem", "install",  path_to_archive},true);    
        }

        virtual void installation(const vector <string> packages)
        {
            for(auto i : packages)
            {
                if(run_command({"command", "-v", i},false,nullptr,true) != 0)
                {
                    vector <string> in = install;
                    in.push_back(i);
                    if(run_command(in,true)==0)
                        cout << i << " : установлено" << "\n";
                    else
                    {
                        cerr << "Не удалось установить: " << i;
                        exit(1);
                    }
                }
            }
        } 

        virtual void cd(string path)
        {
            if(chdir(path.c_str()) != 0) //при успехе возвращается 0, при неудаче возвращается -1
            {
                cerr << "Не удалось перейти в каталог: " << path << endl;
                exit(1);
            }
        }

        virtual int assembly_cmake (string arch_name)
        {   
            installation({"make"});
            installation({"cmake"});
            int sum_code = 0;
            cd(build_dir);
            fs::path tmp{fs::current_path()};
            uintmax_t n{fs::remove_all(tmp / "*")};
            //uintmax_t = unsigned long long

            tmp /= ".cmake";
            tmp /= "api";
            tmp /= "v1";
            tmp /= "query";
            fs::create_directories(tmp);

            cd(tmp);
            run_command({"touch", "codemodel-v2"});
            run_command({"touch", "toolchains-v1"});

            cd(build_dir);
            
            sum_code = sum_code || run_command({"cmake", unpack_dir}); 
            sum_code = sum_code || run_command({"make"});
             
            cd(current_path);
            return sum_code;
        }

        virtual int assembly_autotools (string arch_name)
        {
            installation({"make"});
            int sum_code = 0;
            installation({"autoconf"});
            installation({"automake"});
            cd(unpack_dir);
            sum_code = sum_code || run_command({"./configure"});
            sum_code = sum_code || run_command({"make"});
            const auto copyOptions = fs::copy_options::recursive;
            fs::copy(unpack_dir, build_dir, copyOptions);
            cd(current_path);
            return sum_code;
        }

        virtual string find_file(regex mask, fs::path pathToDir, bool return_name_file = false)
        {
            string pathToFile = "";
            string namefile = "";

            fs::directory_iterator dirIterator(pathToDir);
            for(const auto& entry : dirIterator)
            {
                if(!is_directory(entry))
                {
                    if(regex_match(entry.path().c_str(),mask))
                    {
                        pathToFile = entry.path();
                        namefile = entry.path().filename();
                        break;
                    }

                }

            }

            if(return_name_file) return namefile;

            return pathToFile;

        }

        virtual int gemspec_exists (string path)
        {
            regex reg("(.*)\\.gemspec$", regex_constants::icase);
            return find_file(reg, path,true) != ""; //если 0, то нет gemspec, если 1, то есть gemspec
        }

        virtual int assembly_gem (string arch_name, string path)
        {
            int sum_code = 0;
            installation({"git"});
            cd(path);
            regex mask1(".*(.gemspec)",regex_constants::icase);
            regex mask2(".*(.gem)",regex_constants::icase);
            string gemspec_path = find_file(mask1, unpack_dir);
            sum_code = sum_code || run_command({"gem","build",gemspec_path});
            string gem_path = find_file(mask2, unpack_dir);
            const auto copyOptions = fs::copy_options::recursive;
            fs::copy(gem_path, build_dir, copyOptions);
            cd(current_path);
            return sum_code;
        }

        virtual int assembly_perl_build (string arch_name,string path)
        {
            int sum_code = 0;
            run_command({"cpan", "Module::Build"},true);
            installation({perl_package});
            cd(unpack_dir);
            sum_code = sum_code || run_command({"cpanm", "--installdeps", "."},true);
            sum_code = sum_code || run_command({"perl", "Build.PL"},true);
            sum_code = sum_code || run_command({"./Build"},true);
            const auto copyOptions = fs::copy_options::recursive;
            fs::copy(unpack_dir, build_dir, copyOptions);
            cd(current_path);
            return sum_code;

        }

        virtual int assembly_perl_make(string arch_name)
        {
            installation({"make"});
            int sum_code = 0;
            cd(unpack_dir);
            sum_code = sum_code || run_command({"perl", "./Makefile.PL"});
            sum_code = sum_code || run_command({"make"});

            const auto copyOptions = fs::copy_options::recursive;
            fs::copy(unpack_dir, build_dir, copyOptions);

            cd(current_path);
            return sum_code;
        }

        virtual int assembly_php (string arch_name)
        {
            installation({"make"});
            int rc;
            int sum_code = 0;
            cd(unpack_dir);;
            sum_code = sum_code || run_command(phpize);
            sum_code = sum_code || run_command({"./configure"});
            sum_code = sum_code || run_command({"make"});

            const auto copyOptions = fs::copy_options::recursive;
            fs::copy(unpack_dir, build_dir, copyOptions);
            cd(current_path);

            return sum_code;
        }

};

class FreeBsd : public Unix
{
    public:
        FreeBsd()
        {
            cout << "Это FreeBSD!\n";
        }

        virtual void set_ruby_package() 
        {
            ruby_package = "rubygem-grpc";
        }

        virtual void set_install() 
        {
            install = {"pkg", "install", "-y"};
        }

        virtual int assembly_cmake (string arch_name)
        {
            installation({"bash"});
            return Unix::assembly_cmake(arch_name);

        }

        virtual int assembly_perl_build (string arch_name,string path)
        {
            perl_package = "p5-App-cpanminus";
            Unix::installation({"perl5"});
            return Unix::assembly_perl_build(arch_name, path);
        }

        virtual int assembly_perl_make(string arch_name)
        {
            Unix::installation({"perl5"});
            return Unix::assembly_perl_make(arch_name);

        }

        virtual int assembly_php (string arch_name)
        {
            phpize = {"/usr/local/bin/phpize"};
            Unix::installation({"php83", "php83-pear", "php83-session", "php83-gd"});
            return Unix::assembly_php(arch_name);
        
        }

};

class Linux : public Unix
{
    public:
        string os_name(string path)
        {
            ifstream in(path);
            string os_name;
            if(in.is_open())
            {
                regex r("^(NAME=).*");
            
                while(getline(in,os_name))
                {
                    if(regex_match(os_name,r) == true)
                    {
                        break;
                    }
                }
            }

            in.close();

            return os_name;
        }

};

class OpenSuse : public Linux
{
    public:
        OpenSuse()
        {
            cout << "Это OpenSuse!\n";
        }

        virtual void set_ruby_package() 
        {
            ruby_package = "ruby-devel";
        }

        virtual void set_install() 
        {
            install = {"zypper", "install", "-y"};
        }

        virtual int assembly_perl_build (string arch_name,string path)
        {
            perl_package = "perl-App-cpanminus";
            Unix::installation({"perl"});
            return Unix::assembly_perl_build(arch_name, path);
        }

        virtual int assembly_perl_make(string arch_name)
        {
            Unix::installation({"perl"});
            perl_package_make = "libtirpc-devel";
            Unix::installation({perl_package_make});
            return Unix::assembly_perl_make(arch_name);

        }

        virtual int assembly_php (string arch_name)
        {
            Unix::installation({"php7", "php7-devel", "php7-pecl", "php7-pear"});
            phpize = {"/usr/bin/phpize"};
            return Unix::assembly_php(arch_name);;
        }


};

class Ubuntu : public Linux
{
    public:
        Ubuntu()
        {
            cout << "Это Ubuntu!\n";
        }

        virtual void set_ruby_package() 
        {
            ruby_package = "ruby-dev";
        }

        virtual void set_install() 
        {
            install = {"apt", "install", "-y"};
        }

        virtual int assembly_cmake (string arch_name)
        {
            installation({"libncurses-dev", "libreadline-dev", "libbsd-dev"});
            return Unix::assembly_cmake(arch_name);

        }

        virtual int assembly_perl_build (string arch_name,string path)
        {

            perl_package = "cpanminus";
            Unix::installation({"perl"});
            return Unix::assembly_perl_build(arch_name, path);
        }

        virtual int assembly_perl_make(string arch_name)
        {
            Unix::installation({"perl"});
            perl_package_make = "libtirpc-dev";
            Unix::installation({perl_package_make});
            return Unix::assembly_perl_make(arch_name);

        }

        virtual int assembly_php (string arch_name)
        {
            phpize = {"/usr/bin/phpize"};
            Unix::installation({"php", "php-dev", "php-pear"});
            return Unix::assembly_php(arch_name);

        }


};


void toDo(Os &os)
{
    os.options();
    
    os.set_install();

    vector<string> unpack_cmd;
    enum archiver
    {
        tar,
        gem
    };
    int archiver = tar;
    if(os.type_archive == ".tar")
    {
        unpack_cmd = {"tar", "-xvf"};     
    }
    else if(os.type_archive == ".tar.bz2" || os.type_archive == ".tar.bzip2" || os.type_archive == ".tb2" || os.type_archive == ".tbz")
    {
        unpack_cmd = {"tar", "-xvjf"}; 
    }
    else if(os.type_archive == ".tar.xz" || os.type_archive == ".txz")
    {
        unpack_cmd = {"tar", "-xvJf"};

    }
    else if(os.type_archive == ".tar.gz" || os.type_archive == ".tgz")
    {
        unpack_cmd = {"tar", "-xvzf"};

    }
    else if(os.type_archive == ".gem")
    {
        os.set_ruby_package();
        os.installation("ruby");
        os.installation(os.ruby_package);
        os.install_gems();
        archiver = gem;  
    }

    if(archiver == tar)
    {   
        os.set_dirBuild();
        os.installation("tar");
        int mypipe[2];
        if(pipe(mypipe))
        {
            perror("Ошибка канала");
            exit(1);
        }
        vector<string> cmd = {"tar", "-tf", os.path_to_archive};
        //флаг -c говорит о том, что команды считываются из строки
        if(os.run_command(cmd,false,mypipe) != 0)
        {
            cerr<<"Подозрительные файлы в архиве"<<"\n";
            exit(1);
        }
        else
        {   
            unpack_cmd.push_back(os.path_to_archive);
            unpack_cmd.push_back("-C");
            unpack_cmd.push_back(os.dirBuild/os.archive_name);
            os.run_command(unpack_cmd);
            cout<<"Архив разархивирован"<<"\n";
        }

        if(fs::exists(os.unpack_dir/"CMakeLists.txt") && os.assembly_cmake(os.archive_name) == 0) //CMake
        {
            cout<<"Собрано с помощью CMake"<<"\n";
            os.cd(os.build_dir);
            cout << os.current_path;
            os.run_command({"make", "-n", "install"},true); 
            os.cd(os.current_path);
        }
        else if(fs::exists(os.unpack_dir/"configure") && fs::exists(os.unpack_dir/"Makefile.in")  //GNU Autotools
                && fs::exists(os.unpack_dir/"config.h.in")  && os.assembly_autotools(os.archive_name) == 0)
        {   
            cout<<"Собрано с помощью GNU Autotools"<<"\n";
            os.cd(os.unpack_dir);
            os.run_command({"make", "-n", "install"},true); 
            os.cd(os.current_path);
        }
        else if(os.gemspec_exists(os.unpack_dir) && os.assembly_gem(os.archive_name, os.unpack_dir) == 0) //Ruby тут есть gemspec
        {
            cout<<"Собрано с помощью gem"<<"\n";
            regex r("(.*)\\.gem$", regex_constants::icase);
            string tmp_path = os.find_file(r,os.unpack_dir);
            os.run_command({"gem", "install", tmp_path},true);
        }
        else if(fs::exists(os.unpack_dir/"Rakefile") && !os.gemspec_exists(os.unpack_dir)) //Ruby тут нет gemspec
        {
            os.run_command({"gem", "install", "rake", "rake-compiler"},true);
            os.cd(os.unpack_dir);
            if(os.run_command({"rake", "--all", "-n"}) == 0) cout<<"Прогон всех шагов завершен успешно"<<"\n";  
            os.cd(os.current_path);  
        }
        else if(fs::exists(os.unpack_dir/"Build.PL") && os.assembly_perl_build(os.archive_name,os.path_to_archive) == 0) //Perl
        {
            cout<<"Собрано с помощью Build и Build.PL"<<"\n";
            os.cd(os.unpack_dir);
            os.run_command({"./Build", "install"},true);
            os.cd(os.current_path);
        }

        else if(fs::exists(os.unpack_dir/"Makefile.PL") && os.assembly_perl_make(os.archive_name) == 0) //Perl
        {
            cout<<"Собрано с помощью make и Makefile.PL"<<"\n";
            os.cd(os.unpack_dir);
            os.run_command({"make", "-n", "install"},true);
            os.cd(os.current_path);
        }
        else if(fs::exists(os.unpack_dir/"config.m4") && os.assembly_php(os.archive_name) == 0) //PHP
        {
            cout<<"Собрано с помощью phpize и GNU Autotools"<<"\n";
        
            os.cd(os.unpack_dir);
            os.run_command({"make", "-n", "install"},true);
            
            string tmp_dir = os.unpack_dir/"modules";

            string module;
            regex mask3("(.*)\\.so$", regex_constants::icase);
            module = os.find_file(mask3,tmp_dir,true);
            cout << "Для включения расширения добавьте в конфигурацию PHP строчку:" << endl
                 << "extension=" << module << endl;

        }
        else cout<<"Не удалось собрать пакет"<<"\n"; 
    }

}


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
        distrib.path_to_archive = argv[1];
        toDo(distrib);
        
    }
    else if(regex_match(os,reg2) == true)
    {
        Ubuntu distrib;
        distrib.path_to_archive = argv[1];
        toDo(distrib);
        
    }

#elif defined(__APPLE__)
    //apple code(other kinds of apple platforms)  

#elif defined(__unix__)
    Unix system;
    system.set_path_to_archive(argv[1]);
    os = system.os_name("uname -a");
    regex reg3(".*(freebsd).*\n",regex_constants::icase);
    if(regex_match(os,reg3) == true) 
    {
        FreeBsd distrib;
        distrib.path_to_archive = argv[1];
        toDo(distrib);
    }

#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    //define something for Windows (32-bit and 64-bit, this part is common)
    /* _Win32 is usually defined by compilers targeting 32 or   64 bit Windows systems */
    #ifdef _WIN64
        //define something for Windows (64-bit only)
    #else
        //define something for Windows (32-bit only)
    #endif
    
#endif

}



