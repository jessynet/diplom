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
#include "cmake_trace_opensuse.h"
#include "cmake_trace_freebsd.h"
#include "cmake_trace_ubuntu.h"
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
    protected:
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
        string template_incorrect_path; //указатель на функцию из .h
        void (*ptr_cmake_depend)(fs::path,fs::path);
        void (*ptr_cmake_trace)(fs::path,fs::path, vector<string>,string);

    public:

        virtual int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false, string *ptr = nullptr)  = 0;

        virtual void installation(const vector <string> packages) = 0;

        void installation(string package) {vector<string> pkgs = {package}; installation(pkgs); }

        virtual void build_unpack_dir() = 0;

        virtual void install_gems() = 0;

        virtual void cd(string path) = 0;

        virtual int assembly_cmake() = 0;

        virtual int assembly_autotools() = 0;

        virtual int gemspec_exists(string path) = 0;

        virtual string find_file(regex mask, fs::path pathToDir, bool return_name_file = false) = 0;

        virtual int assembly_gem() = 0;

        virtual void cmake_libs() = 0;

        virtual void cmake_trace() = 0;

        virtual int assembly_perl_build() = 0;

        virtual int assembly_perl_make() = 0;

        virtual int assembly_php() = 0;

        virtual int return_code_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false) = 0;

        string get_archive_type(){return type_archive;}
        fs::path get_unpack_dir(){return unpack_dir;}
        fs::path get_build_dir(){return build_dir;}
        fs::path get_current_path(){return current_path;}
        fs::path get_path_to_archive(){return path_to_archive;}
        fs::path get_dirBuild(){return dirBuild;}
        fs::path get_archive_name(){return archive_name;}
        void set_path_to_archive(string path){path_to_archive = path;}
        void set_install_command(vector <string> cmd_install){install = cmd_install;}
        int check_tar(int* stdout_pipe)
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

        virtual void build_unpack_dir()
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
        
            return getuid() == 0; //краткая запись if(getuid()==0) {return true;} else {return false;}
        
        }

        virtual int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false, string *ptr = nullptr) 
        {
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
                    break;
                       
                }
                default: //pid>0 родительский процесс
                {
                    int status;
                        
                    if (stdout_pipe) 
                    {
                        if(ptr)
                        {
                    
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
                            regex reg("CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES");
                            while((linelen = getline(&line, &linesize, f)) != -1)
                            {
                                if(regex_search(line, reg))
                                {
                                    string temp_link_dirs = line;
                                    int find_sep = temp_link_dirs.find('"');
                                    temp_link_dirs = temp_link_dirs.substr(find_sep);
                                    temp_link_dirs.erase(0,1);
                                    temp_link_dirs.erase(temp_link_dirs.size() - 1);
                                    *ptr = temp_link_dirs;
                                    break;
                                    
                                }
                 
                            }
                            free(line);
                            fclose(f);

                        }
                        else return_code = Os::check_tar(stdout_pipe);

                    }    
                        

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
                    break;
                }   
            }
        
            return return_code;
        } 

        virtual int return_code_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool hide_stderr = false)
        {
            if(run_command(cmd, need_admin_rights, stdout_pipe, hide_stderr) != 0) 
            {
                cerr << "Ошибка выполнения команды\n";
                exit(1);
            }
            else return 0;

        }
 
        virtual void install_gems()
        {
            return_code_command({"cp", "-r", path_to_archive ,build_dir});
            return_code_command({"gem", "install",  path_to_archive},true);    
        }

        virtual void installation(const vector <string> packages) //программа ставится из одноименного пакета 
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

        virtual void cmake_libs()
        {
            fs::path path_to_reply = build_dir;
            path_to_reply /= ".cmake";
            path_to_reply /= "api";
            path_to_reply /= "v1";
            path_to_reply /= "reply";
            ptr_cmake_depend(path_to_reply, build_dir);
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
            return_code_command({"touch", "codemodel-v2"});
            return_code_command({"touch", "toolchains-v1"});

            cd(build_dir);
            
            sum_code = sum_code || return_code_command({"cmake", unpack_dir}); 
            cmake_libs();
            sum_code = sum_code || return_code_command({"make"});
             
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
            sum_code = sum_code || return_code_command({"./configure"});
            sum_code = sum_code || return_code_command({"make"});
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
            sum_code = sum_code || return_code_command({"gem","build",gemspec_path});
            string gem_path = find_file(mask2, unpack_dir);
            const auto copyOptions = fs::copy_options::recursive;
            fs::copy(gem_path, build_dir, copyOptions);
            cd(current_path);
            return sum_code;
        }

        virtual int assembly_perl_build()
        {
            int sum_code = 0;
            return_code_command({"cpan", "Module::Build"},true);
            installation({perl_package_build});
            cd(unpack_dir);
            sum_code = sum_code || return_code_command({"cpanm", "--installdeps", "."},true);
            sum_code = sum_code || return_code_command({"perl", "Build.PL"},true);
            sum_code = sum_code || return_code_command({"./Build"},true);
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
            sum_code = sum_code || return_code_command({"perl", "./Makefile.PL"});
            sum_code = sum_code || return_code_command({"make"});

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
            sum_code = sum_code || return_code_command(phpize);
            sum_code = sum_code || return_code_command({"./configure"});
            sum_code = sum_code || return_code_command({"make"});

            const auto copyOptions = fs::copy_options::recursive;
            fs::copy(unpack_dir, build_dir, copyOptions);
            cd(current_path);

            return sum_code;
        }

        virtual void cmake_trace()
        {
            fs::path path_to_reply = build_dir;
            string link_directories;
            vector <string> link_dirs;

            int mypipe_inf[2];
            pipe(mypipe_inf);
            cd(build_dir);
            run_command({"cmake", "--system-information"},false,mypipe_inf,false, &link_directories);
            char str_without_sep[link_directories.size()];
            strcpy(str_without_sep, link_directories.c_str());
            char *pch = strtok(str_without_sep, ";");

            while(pch != NULL)
            {
                link_dirs.push_back(pch);
                pch = strtok(NULL, ";");
            }

            for(auto i : link_dirs)
                cout << i << endl;
            //cd(current_path);

            fs::path path_to_package = "/tmp/archives/" + archive_name;


            ptr_cmake_trace(unpack_dir,path_to_package,link_dirs, archive_name);

        }


};

class FreeBsd : public Unix
{
    public:
        FreeBsd()
        {
            cerr << "FreeBSD!\n";
        }

        virtual int assembly_cmake()
        {
            installation({"bash"});
            ptr_cmake_trace = freebsd_trace;
            Unix::cmake_trace(); 
            ptr_cmake_depend = find_depend_freebsd;
            return Unix::assembly_cmake();
            
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


};

class OpenSuse : public Linux
{
    public:
        OpenSuse()
        {
            cerr << "OpenSuse!\n";
        }

        virtual int assembly_perl_build()
        {
            perl_package_build = "perl-App-cpanminus";
            Unix::installation({"perl"});
            return Unix::assembly_perl_build();
        }

        virtual int assembly_perl_make()
        {
            Unix::installation({"perl"});
            perl_package_make = "libtirpc-devel";
            Unix::installation({perl_package_make});
            return Unix::assembly_perl_make();

        }

        virtual int assembly_php()
        {
            Unix::installation({"php7", "php7-devel", "php7-pecl", "php7-pear"});
            phpize = {"/usr/bin/phpize"};
            return Unix::assembly_php();;
        }

        virtual int assembly_cmake()
        {
            ptr_cmake_trace = opensuse_trace;
            Unix::cmake_trace(); 
            ptr_cmake_depend = find_depend_opensuse;
            return Unix::assembly_cmake();
            
        }

        virtual void install_gems()
        {
            Unix::installation({"ruby-devel"});
            Unix::install_gems();
        }

};

class Ubuntu : public Linux
{
    public:
        Ubuntu()
        {
            cerr << "Ubuntu!\n";
        }

        virtual int assembly_cmake()
        {
            installation({"libncurses-dev", "libreadline-dev", "libbsd-dev"});
            ptr_cmake_trace = ubuntu_trace;
            Unix::cmake_trace(); 
            ptr_cmake_depend = find_depend_ubuntu;
            return Unix::assembly_cmake();
            
        }

        virtual int assembly_perl_build()
        {

            perl_package_build = "cpanminus";
            Unix::installation({"perl"});
            return Unix::assembly_perl_build();
        }

        virtual int assembly_perl_make()
        {
            Unix::installation({"perl"});
            perl_package_make = "libtirpc-dev";
            Unix::installation({perl_package_make});
            return Unix::assembly_perl_make();

        }

        virtual int assembly_php()
        {
            phpize = {"/usr/bin/phpize"};
            Unix::installation({"php", "php-dev", "php-pear"});
            return Unix::assembly_php();

        }

        virtual void install_gems()
        {
            Unix::installation({"ruby-dev"});
            Unix::install_gems();
        }

       


};


void toDo(Os &os)
{
    os.build_unpack_dir();
    vector<string> unpack_cmd;
    enum archiver
    {
        tar,
        gem
    };
    int archiver = tar;
    if(os.get_archive_type() == ".tar")
    {
        unpack_cmd = {"tar", "-xvf"};     
    }
    else if(os.get_archive_type() == ".tar.bz2" || os.get_archive_type() == ".tar.bzip2" || os.get_archive_type() == ".tb2" || os.get_archive_type() == ".tbz")
    {
        unpack_cmd = {"tar", "-xvjf"}; 
    }
    else if(os.get_archive_type() == ".tar.xz" || os.get_archive_type() == ".txz")
    {
        unpack_cmd = {"tar", "-xvJf"};

    }
    else if(os.get_archive_type() == ".tar.gz" || os.get_archive_type() == ".tgz")
    {
        unpack_cmd = {"tar", "-xvzf"};

    }
    else if(os.get_archive_type() == ".gem")
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
        vector<string> cmd = {"tar", "-tf", os.get_path_to_archive()};
        //флаг -c говорит о том, что команды считываются из строки
        if(os.run_command(cmd,false,mypipe) != 0)
        {
            cerr<<"Подозрительные файлы в архиве"<<"\n";
            exit(1);
        }
        else
        {   
            unpack_cmd.push_back(os.get_path_to_archive());
            unpack_cmd.push_back("-C");
            unpack_cmd.push_back(os.get_dirBuild()/os.get_archive_name());
            os.return_code_command(unpack_cmd);
            cout<<"Архив разархивирован"<<"\n";
        }

        if(fs::exists(os.get_unpack_dir()/"CMakeLists.txt") && os.assembly_cmake() == 0) //CMake
        {
            cout<<"Собрано с помощью CMake"<<"\n";
            os.cd(os.get_build_dir());
            cout << os.get_current_path();
            os.return_code_command({"make", "-n", "install"},true); 
            os.cd(os.get_current_path());
        }
        else if(fs::exists(os.get_unpack_dir()/"configure") && fs::exists(os.get_unpack_dir()/"Makefile.in")  //GNU Autotools
                && fs::exists(os.get_unpack_dir()/"config.h.in")  && os.assembly_autotools() == 0)
        {   
            cout<<"Собрано с помощью GNU Autotools"<<"\n";
            os.cd(os.get_unpack_dir());
            os.return_code_command({"make", "-n", "install"},true); 
            os.cd(os.get_unpack_dir());
        }
        else if(os.gemspec_exists(os.get_unpack_dir()) && os.assembly_gem() == 0) //Ruby тут есть gemspec
        {
            cout<<"Собрано с помощью gem"<<"\n";
            regex r("(.*)\\.gem$", regex_constants::icase);
            string tmp_path = os.find_file(r,os.get_unpack_dir());
            os.return_code_command({"gem", "install", tmp_path},true);
        }
        else if(fs::exists(os.get_unpack_dir()/"Rakefile") && !os.gemspec_exists(os.get_unpack_dir())) //Ruby тут нет gemspec
        {
            os.return_code_command({"gem", "install", "rake", "rake-compiler"},true);
            os.cd(os.get_unpack_dir());
            if(os.return_code_command({"rake", "--all", "-n"}) == 0) cout<<"Прогон всех шагов завершен успешно"<<"\n";  
            os.cd(os.get_current_path());  
        }
        else if(fs::exists(os.get_unpack_dir()/"Build.PL") && os.assembly_perl_build() == 0) //Perl
        {
            cout<<"Собрано с помощью Build и Build.PL"<<"\n";
            os.cd(os.get_unpack_dir());
            os.return_code_command({"./Build", "install"},true);
            os.cd(os.get_current_path());
        }

        else if(fs::exists(os.get_unpack_dir()/"Makefile.PL") && os.assembly_perl_make() == 0) //Perl
        {
            cout<<"Собрано с помощью make и Makefile.PL"<<"\n";
            os.cd(os.get_unpack_dir());
            os.return_code_command({"make", "-n", "install"},true);
            os.cd(os.get_current_path());
        }
        else if(fs::exists(os.get_unpack_dir()/"config.m4") && os.assembly_php() == 0) //PHP
        {
            cout<<"Собрано с помощью phpize и GNU Autotools"<<"\n";
        
            os.cd(os.get_unpack_dir());
            os.return_code_command({"make", "-n", "install"},true);
            
            string tmp_dir = os.get_unpack_dir()/"modules";

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
        //define something for Windows (64-bit only)
    #else
        //define something for Windows (32-bit only)
    #endif
    
#endif

}



