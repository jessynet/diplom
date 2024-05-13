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
#include "cmake_opensuse.h"
#include "cmake_ubuntu.h"
#include "cmake_freebsd.h"
#include <experimental/filesystem>
using namespace std;
namespace fs = std::experimental::filesystem;


/*Различие в том, где препроцессор будет начинать поиск файла some.h. Если использовать директиву #include "some.h", то 
сначала будут просмотрены локальные (по отношению к проекту) папки включения файлов. Если использовать #include <some.h>,
то сначала будут просматриваться глобальные (по отношению к проекту) папки включения файлов. Глобальные папки включения -
это папки, прописанные в настройке среды разработки, локальные - это те, которые прописаны в настройках проекта.*/

class Os
{
    public:

        string current_path = fs::current_path();
        fs::path unpack_dir;
        fs::path build_dir;
        string archive_name;
        fs::path path_to_archive;
        fs::path dirBuild;
        string type_archive;
        vector<string> install;
        string perl_package_build;
        string perl_package_make;
        vector <string> phpize;
        string template_incorrect_path;
    
        virtual int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false) = 0;

        virtual void installation(const vector <string> packages) = 0;

        void installation(string package) {vector<string> pkgs = {package}; installation(pkgs); }

        virtual void options() = 0;

        virtual void install_gems() = 0;

        virtual void cd(string path) = 0;

        virtual int assembly_cmake() = 0;

        virtual int assembly_autotools() = 0;

        virtual int gemspec_exists(string path) = 0;

        virtual string find_file(regex mask, fs::path pathToDir, bool return_name_file = false) = 0;

        virtual int assembly_gem() = 0;

        virtual int assembly_perl_build() = 0;

        virtual int assembly_perl_make() = 0;

        virtual int assembly_php() = 0;

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
            regex reg(template_incorrect_path, regex::extended);
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
            dirBuild = "/tmp/archives";
            template_incorrect_path = "^(.*/)?\\.\\./";
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
        
        virtual void install_gems()
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

        virtual int assembly_cmake()
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

        virtual int assembly_autotools()
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

        virtual int gemspec_exists(string path)
        {
            regex reg("(.*)\\.gemspec$", regex_constants::icase);
            return find_file(reg, path,true) != ""; //если 0, то нет gemspec, если 1, то есть gemspec
        }

        virtual int assembly_gem()
        {
            int sum_code = 0;
            installation({"git"});
            cd(unpack_dir);
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

        virtual int assembly_perl_build()
        {
            int sum_code = 0;
            run_command({"cpan", "Module::Build"},true);
            installation({perl_package_build});
            cd(unpack_dir);
            sum_code = sum_code || run_command({"cpanm", "--installdeps", "."},true);
            sum_code = sum_code || run_command({"perl", "Build.PL"},true);
            sum_code = sum_code || run_command({"./Build"},true);
            const auto copyOptions = fs::copy_options::recursive;
            fs::copy(unpack_dir, build_dir, copyOptions);
            cd(current_path);
            return sum_code;

        }

        virtual int assembly_perl_make()
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

        virtual int assembly_php()
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

        virtual int assembly_cmake()
        {
            installation({"bash"});

            fs::path path_to_reply = build_dir;
            path_to_reply /= ".cmake";
            path_to_reply /= "api";
            path_to_reply /= "v1";
            path_to_reply /= "reply";
            find_depend_freebsd(path_to_reply, build_dir);
            return Unix::assembly_cmake();

        }

        virtual int assembly_autotools()
        {
            return Unix::assembly_autotools();
        }

        virtual int assembly_perl_build()
        {
            perl_package_build = "p5-App-cpanminus";
            Unix::installation({"perl5"});
            return Unix::assembly_perl_build();
        }

        virtual int assembly_perl_make()
        {
            Unix::installation({"perl5"});
            return Unix::assembly_perl_make();

        }

        virtual int assembly_php()
        {
            phpize = {"/usr/local/bin/phpize"};
            Unix::installation({"php83", "php83-pear", "php83-session", "php83-gd"});
            return Unix::assembly_php();
        }

        virtual int assembly_gem()
        {
            return Unix::assembly_gem();
        }

        virtual int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false)
        {
            return Unix::run_command(cmd, need_admin_rights, stdout_pipe, hide_stderr);

        }

        virtual void installation(const vector <string> packages)
        {
            Unix::installation(packages);
        }

        virtual void options()
        {
            Unix::options();
        }

        virtual void cd(string path)
        {
            Unix::cd(path);
        }

        virtual string find_file(regex mask, fs::path pathToDir, bool return_name_file = false)
        {
            return Unix::find_file(mask,pathToDir, return_name_file);

        }

        virtual int gemspec_exists(string path)
        {
            return Unix::gemspec_exists(path);
        }

        virtual void install_gems()
        {
            Unix::installation({"rubygem-grpc"});
            Unix::install_gems();
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

        virtual int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false)
        {
            return Unix::run_command(cmd, need_admin_rights, stdout_pipe, hide_stderr);

        }

        virtual void installation(const vector <string> packages)
        {
            Unix::installation(packages);
        }

        virtual void options()
        {
            Unix::options();
        }

        virtual void cd(string path)
        {
            Unix::cd(path);
        }

        virtual string find_file(regex mask, fs::path pathToDir, bool return_name_file = false)
        {
            return Unix::find_file(mask,pathToDir, return_name_file);

        }

        virtual int assembly_cmake()
        {
            return Unix::assembly_cmake();
        }

        virtual int assembly_autotools()
        {
            return Unix::assembly_autotools();
        }

        virtual int assembly_gem()
        {
            return Unix::assembly_gem();
        }

        virtual int assembly_perl_build()
        {
            return Unix::assembly_perl_build();
        }

        virtual int assembly_perl_make()
        {
            return Unix::assembly_perl_make();
        }

        virtual int assembly_php()
        {
            return Unix::assembly_php();
        }

        virtual int gemspec_exists(string path)
        {
            return Unix::gemspec_exists(path);
        }

        virtual void install_gems()
        {
            Unix::install_gems();
        }

};

class OpenSuse : public Linux
{
    public:
        OpenSuse()
        {
            cout << "Это OpenSuse!\n";
        }

        virtual int assembly_perl_build()
        {
            perl_package_build = "perl-App-cpanminus";
            Linux::installation({"perl"});
            return Linux::assembly_perl_build();
        }

        virtual int assembly_perl_make()
        {
            Linux::installation({"perl"});
            perl_package_make = "libtirpc-devel";
            Linux::installation({perl_package_make});
            return Linux::assembly_perl_make();

        }

        virtual int assembly_php()
        {
            Linux::installation({"php7", "php7-devel", "php7-pecl", "php7-pear"});
            phpize = {"/usr/bin/phpize"};
            return Linux::assembly_php();;
        }

        virtual int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false)
        {
            return Linux::run_command(cmd, need_admin_rights, stdout_pipe, hide_stderr);

        }

        virtual void installation(const vector <string> packages)
        {
            Linux::installation(packages);
        }

        virtual void options()
        {
            Linux::options();
        }

        virtual void cd(string path)
        {
            Linux::cd(path);
        }

        virtual string find_file(regex mask, fs::path pathToDir, bool return_name_file = false)
        {
            return Linux::find_file(mask,pathToDir, return_name_file);

        }

        virtual int assembly_cmake()
        {
            fs::path path_to_reply = build_dir;
            path_to_reply /= ".cmake";
            path_to_reply /= "api";
            path_to_reply /= "v1";
            path_to_reply /= "reply";
            find_depend_opensuse(path_to_reply, build_dir);
            return Linux::assembly_cmake();
        }

        virtual int assembly_autotools()
        {
            return Linux::assembly_autotools();
        }

        virtual int assembly_gem()
        {
            return Linux::assembly_gem();
        }

        virtual int gemspec_exists (string path)
        {
            return Linux::gemspec_exists(path);
        }

        virtual void install_gems()
        {
            Linux::installation({"ruby-devel"});
            Linux::install_gems();
        }

};

class Ubuntu : public Linux
{
    public:
        Ubuntu()
        {
            cout << "Это Ubuntu!\n";
        }

        virtual int assembly_cmake()
        {
            fs::path path_to_reply = build_dir;
            path_to_reply /= ".cmake";
            path_to_reply /= "api";
            path_to_reply /= "v1";
            path_to_reply /= "reply";
            find_depend_ubuntu(path_to_reply, build_dir);
            
            //installation({"libncurses-dev", "libreadline-dev", "libbsd-dev"});
            return Linux::assembly_cmake();

        }

        virtual int assembly_perl_build()
        {

            perl_package_build = "cpanminus";
            Linux::installation({"perl"});
            return Linux::assembly_perl_build();
        }

        virtual int assembly_perl_make()
        {
            Linux::installation({"perl"});
            perl_package_make = "libtirpc-dev";
            Linux::installation({perl_package_make});
            return Linux::assembly_perl_make();

        }

        virtual int assembly_php()
        {
            phpize = {"/usr/bin/phpize"};
            Linux::installation({"php", "php-dev", "php-pear"});
            return Linux::assembly_php();

        }

        virtual int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false)
        {
            return Linux::run_command(cmd, need_admin_rights, stdout_pipe, hide_stderr);

        }

        virtual void installation(const vector <string> packages)
        {
            Linux::installation(packages);
        }

        virtual void options()
        {
            Linux::options();
        }

        virtual void cd(string path)
        {
            Linux::cd(path);
        }

        virtual string find_file(regex mask, fs::path pathToDir, bool return_name_file = false)
        {
            return Linux::find_file(mask,pathToDir, return_name_file);

        }

        virtual int assembly_autotools()
        {
            return Linux::assembly_autotools();
        }

        virtual int assembly_gem()
        {
            return Linux::assembly_gem();
        }

        virtual int gemspec_exists(string path)
        {
            return Linux::gemspec_exists(path);
        }

        virtual void install_gems()
        {
            Linux::installation({"ruby-dev"});
            Linux::install_gems();
        }

};


void toDo(Os &os)
{
    os.options();
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
        os.installation("ruby");
        os.install_gems();
        archiver = gem;  
    }

    if(archiver == tar)
    {   
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

        if(fs::exists(os.unpack_dir/"CMakeLists.txt") && os.assembly_cmake() == 0) //CMake
        {
            cout<<"Собрано с помощью CMake"<<"\n";
            os.cd(os.build_dir);
            cout << os.current_path;
            os.run_command({"make", "-n", "install"},true); 
            os.cd(os.current_path);
        }
        else if(fs::exists(os.unpack_dir/"configure") && fs::exists(os.unpack_dir/"Makefile.in")  //GNU Autotools
                && fs::exists(os.unpack_dir/"config.h.in")  && os.assembly_autotools() == 0)
        {   
            cout<<"Собрано с помощью GNU Autotools"<<"\n";
            os.cd(os.unpack_dir);
            os.run_command({"make", "-n", "install"},true); 
            os.cd(os.current_path);
        }
        else if(os.gemspec_exists(os.unpack_dir) && os.assembly_gem() == 0) //Ruby тут есть gemspec
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
        else if(fs::exists(os.unpack_dir/"Build.PL") && os.assembly_perl_build() == 0) //Perl
        {
            cout<<"Собрано с помощью Build и Build.PL"<<"\n";
            os.cd(os.unpack_dir);
            os.run_command({"./Build", "install"},true);
            os.cd(os.current_path);
        }

        else if(fs::exists(os.unpack_dir/"Makefile.PL") && os.assembly_perl_make() == 0) //Perl
        {
            cout<<"Собрано с помощью make и Makefile.PL"<<"\n";
            os.cd(os.unpack_dir);
            os.run_command({"make", "-n", "install"},true);
            os.cd(os.current_path);
        }
        else if(fs::exists(os.unpack_dir/"config.m4") && os.assembly_php() == 0) //PHP
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
        distrib.install = {"zypper", "install", "-y"};
        toDo(distrib);
        
    }
    else if(regex_match(os,reg2) == true)
    {
        Ubuntu distrib;
        distrib.path_to_archive = argv[1];
        distrib.install = {"apt", "install", "-y"};
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
        distrib.install = {"pkg", "install", "-y"};;
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



