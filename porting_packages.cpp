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


string current_path = fs::current_path();
string os;
fs::path unpack_dir;
fs::path build_dir;


void cd(string path)
{
    if(chdir(path.c_str()) != 0) //при успехе возвращается 0, при неудаче возвращается -1
    {
        cerr << "Не удалось перейти в каталог: " << path << endl;
        exit(1);
    }
}

string find_file(regex mask, fs::path pathToDir = unpack_dir, bool return_name_file = false)
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

bool is_admin() {
#ifdef WIN32
    // TODO
    return true;
#else
    return getuid() == 0; //краткая запись if(getuid()==0) {return true;} else {return false;}
#endif
}

int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, bool devNull = false) {
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
                if(devNull)
                {
                    int devNull = open("/dev/null", O_WRONLY);
                    dup2(devNull, STDERR_FILENO);
                }
                dup2(stdout_pipe[1], STDOUT_FILENO); //делаем STDOUT_FILENO(стандартный вывод) копией stdout_pipe[1]
                //int dup2(int old_handle, int new_handle)
                //Функция dup2() дублирует old_handle как new_handle. Если имеется файл, который был связан с new_handle до вызова dup2(), 
                //то он будет закрыт. В случае успеха возвращается 0, а в случае ошибки —1.
                //close(stdout_pipe[1]);
            }

            char *args[cmd.size()+1];
            for(int i = 0; i < cmd.size(); i++)
            {
                char *cstr = &(cmd[i])[0];
                args[i] = cstr;

            }

            args[cmd.size()] = NULL;

            char* c = args[0];
               
            if(execvp(c,args)==-1)
            {
                cerr << "Не удалось выполнить комманду exec\n"; 
                _exit(42);
            }    
        }
        default: //pid>0 родительский процесс
        {
            int status; //цикл, так как дочерний процесс может быть прерван на какое-то время сигналов
            //например, заснуть, это будет расцененно как завершение дочернего процесса,но это не так
                
            if (stdout_pipe) 
            {
                close(stdout_pipe[1]); 
                //char buf[1024*64];
                //std::size_t— это беззнаковый целочисленный тип результата оператора sizeof
                //ssize_t - тот же size_t, но только знаковый
                //ssize_t nread;
                /*возможные ситуации:
                1)Прочитали ровно то количество байт, сколько было в файле и тогда nread = 0 - не ошибка
                2)Прочитали меньшее число байт, чем планировалось и больше читать нечего, 0 - тоже не ошибка
                3)Когда ничего не было изначально для чтения - будет 0 и это уже ошибка в рамках данной программы и этой логики(не найден файл,например)
                */
                //bool tread = false;
                //while((nread = read(stdout_pipe[0], buf, sizeof(buf))) > 0)
                //{
                //    cout << "Вывод дочернего процесса\n" << buf;
                //    tread = true;
                //}
                //ssize_t nread = read(stdout_pipe[0], buf, sizeof(buf));  //Сообщение дочернего процесса
                //if (nread < 0)
                //{
                //    return_code = 1;
                //}
                //else
                //    if(!tread)
                //        return_code = 1;

                FILE *f = fdopen(stdout_pipe[0], "r");
                if (!f) {
                    auto es = strerror(errno);
                    cerr << "fdopen failure: " << es << endl;
                    exit(1);
                }

                char *line= NULL;
                size_t linesize = 0;
                ssize_t linelen;
                regex reg("^(.*/)?\\.\\./", regex::extended);
                while((linelen = getline(&line, &linesize, f)) != -1)
                {
                   
                    if(regex_match(line, reg))
                    {
                        return_code = 1;
                        break;
                    }   
                }
                free(line);
            }
                
            do {
                waitpid(pid, &status, 0);
            } while(!WIFEXITED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
            int child_status;
            if(WEXITSTATUS(status) == 0) child_status = 0;
            else child_status = 1;
            return_code = return_code || child_status; 
        }   
    }
#endif
    return return_code;
}

void installation (string package)
{
    if(run_command({"command", "-v", package},false,nullptr,true) != 0)
    {
        vector<string> install;
        if(os=="opensuse") install = {"zypper", "install", "-y", package};
        else if(os=="ubuntu") install = {"apt", "install", "-y", package};
        else if(os=="freebsd") install = {"pkg", "install", "-y", package};
        if(run_command(install,true)==0)
            cout<<package<<" : установлено"<<"\n";
        else
        {
            cerr<<"Не удалось установить: "<<package;
            exit(1);
        }
    }
}

int assembly_cmake (string arch_name)
{   
    installation("make");
    installation("cmake");
    //Все в одной команде,так как вызовы system независимы и каждый раз при вызове будет получаться новая среда
    //rm -r пока без ключа -f, который позволяет совершать удаление без запроса подтверждения
    //-r рекурсивно
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
    if(os=="opensuse")
    {
        //если выполнено первое условие,то последующие действия не выполняются
        
        sum_code = sum_code || run_command({"cmake", unpack_dir}); 
        //если sum_code будет 1, то второе действие (run_command) не будет выполняться
        //если будет равно 0, то будет выполняться следующее действие 
        sum_code = sum_code || run_command({"make"});
    } 
        
    else if(os=="ubuntu") 
    {
        run_command({"apt", "install", "-y", "libncurses-dev", "libreadline-dev", "libbsd-dev"},true);
        sum_code = sum_code || run_command({"cmake", unpack_dir});
        sum_code = sum_code || run_command({"make"});
    }
    else if(os=="freebsd")
    {
        installation("bash");
        sum_code = sum_code || run_command({"cmake", unpack_dir});
        sum_code = sum_code || run_command({"make"});
    }
    cd(current_path);
    return sum_code;
}

int assembly_autotools (string arch_name)
{
    installation("make");
    int sum_code = 0;
    installation("autoconf");
    installation("automake");
    cd(unpack_dir);
    sum_code = sum_code || run_command({"./configure"});
    sum_code = sum_code || run_command({"make"});
    const auto copyOptions = fs::copy_options::recursive;
    fs::copy(unpack_dir, build_dir, copyOptions);
    cd(current_path);
    return sum_code;
}

void install_gems (string arch_name, string path)
{
    run_command({"cp", "-r", path ,build_dir});
    run_command({"gem", "install", path},true);    
}

int assembly_gem (string arch_name, string path)
{
    int sum_code = 0;
    installation("git");
    cd(path);
    regex mask1(".*(.gemspec)",regex_constants::icase);
    regex mask2(".*(.gem)",regex_constants::icase);
    string gemspec_path = find_file(mask1); //Написать обертку поиска файла
    sum_code = sum_code || run_command({"gem","build",gemspec_path});
    string gem_path = find_file(mask2);//Написать обертку поиска файла
    const auto copyOptions = fs::copy_options::recursive;
    //cout << gemspec_path << endl << gem_path << endl;
    fs::copy(gem_path, build_dir, copyOptions);
    cd(current_path);
    return sum_code;
}

int assembly_php (string arch_name)
{
    installation("make");
    int rc;
    int sum_code = 0;
    cd(unpack_dir);
    if(os=="opensuse") 
    {
        run_command({"zypper", "install", "-y", "php7", "php7-devel", "php7-pecl", "php7-pear"},true);
        sum_code = sum_code || run_command({"/usr/bin/phpize"});

    }
    else if(os=="ubuntu")
    {
        run_command({"apt", "install", "-y", "php", "php-dev", "php-pear"},true);
        sum_code = sum_code || run_command({"/usr/bin/phpize"});
    } 
    else if(os=="freebsd")
    {
        run_command({"pkg", "install", "-y", "php83", "php83-pear", "php83-session", "php83-gd"},true);
        sum_code = sum_code || run_command({"/usr/local/bin/phpize"});
    }

    sum_code = sum_code || run_command({"./configure"});
    sum_code = sum_code || run_command({"make"});

    const auto copyOptions = fs::copy_options::recursive;
    fs::copy(unpack_dir, build_dir, copyOptions);
    cd(current_path);
    return sum_code;
}

int assembly_perl_build (string arch_name,string path)
{
    int sum_code = 0;
    run_command({"cpan", "Module::Build"},true);
    if(os=="opensuse")
    {
        installation("perl");
        run_command({"zypper", "install", "-y", "perl-App-cpanminus"},true);
    } 
    else if(os=="ubuntu")
    {
        installation("perl");
        run_command({"apt", "install", "-y", "cpanminus"},true);
    } 
    else if(os=="freebsd")
    {
        installation("perl5");
        run_command({"pkg", "install", "-y", "p5-App-cpanminus"},true);
    } 

    cd(unpack_dir);
    sum_code = sum_code || run_command({"cpanm", "--installdeps", "."},true);
    sum_code = sum_code || run_command({"perl", "Build.PL"},true);
    sum_code = sum_code || run_command({"./Build"},true);
    const auto copyOptions = fs::copy_options::recursive;
    fs::copy(unpack_dir, build_dir, copyOptions);
    cd(current_path);
    return sum_code;

}

int assembly_perl_make (string arch_name)
{
    installation("make");
    int sum_code = 0;
    if(os=="opensuse")
    {
        installation("perl");
        run_command({"zypper", "install", "-y", "libtirpc-devel"},true);
    } 
    else if(os=="ubuntu") 
    {
        installation("perl");
        run_command({"apt", "install", "-y", "libtirpc-dev"},true);
    }
    else if(os=="freebsd") installation("perl5");
    cd(unpack_dir);
    sum_code = sum_code || run_command({"perl", "./Makefile.PL"});
    sum_code = sum_code || run_command({"make"});

    const auto copyOptions = fs::copy_options::recursive;
    fs::copy(unpack_dir, build_dir, copyOptions);

    cd(current_path);
    return sum_code;

}

int empty_gemspec (string path)
{
    regex reg("(.*)\\.gemspec$", regex_constants::icase);
    return find_file(reg, path,true) != ""; //если 0, то нет gemspec, если 1, то есть gemspec
}


int main(int argc, char *argv[])
{
    // cout<<argv[1]; //Первый переданный аргумент программе
    if (argc == 1)
    {
        cerr << "Не указан путь к архиву\n";
        exit(1);
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

    regex reg1(".*(opensuse).*",regex_constants::icase);
    regex reg2(".*(ubuntu).*",regex_constants::icase);
    if(regex_match(os_name.c_str(),reg1) == true)
    {   
        os = "opensuse";
    }
    else if(regex_match(os_name.c_str(),reg2) == true)
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
    if(regex_match(result.c_str(),reg3) == true) {os="freebsd";}
    superuser = "sudo";
#elif defined(__APPLE__)
    //apple code(other kinds of apple platforms)    
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    //define something for Windows (32-bit and 64-bit, this part is common)
    /* _Win32 is usually defined by compilers targeting 32 or   64 bit Windows systems */
    //superuser = "runas......";
    #ifdef _WIN64
        //define something for Windows (64-bit only)
    #else
        //define something for Windows (32-bit only)
    #endif
    
#endif    

    current_path = fs::current_path();

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
    
    fs::path path3("/tmp/archives");
    path3 /= arch;
    path3 /= "build-"+arch;
    fs::create_directories(path3);

    unpack_dir = "/tmp/archives/" + arch + "/" + arch;
    build_dir = "/tmp/archives/" + arch + "/build-" + arch;

    vector<string> unpack_cmd;
    int archiver = 1;
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
        installation("ruby");
        if(os=="opensuse") run_command({"zypper", "install", "-y", "ruby-devel"},true);
        else if(os=="ubuntu") run_command({"apt", "install", "-y", "ruby-dev"},true);
	    else if(os=="freebsd") run_command({"pkg", "install", "-y", "rubygem-grpc"},true);
        install_gems(arch,path_arch);
        archiver = 2;  

    }

    if(archiver == 1)
    {   
        installation("tar");
        int mypipe[2];
        if(pipe(mypipe))
        {
            perror("Ошибка канала");
            exit(1);
        }
        vector<string> cmd = {"tar", "-tf", path_arch};
        //флаг -c говорит о том, что команды считываются из строки
        if(run_command(cmd,false,mypipe) != 0)
        {
            cerr<<"Подозрительные файлы в архиве"<<"\n";
            exit(1);
        }
        else
        {   
            unpack_cmd.push_back(path_arch);
            unpack_cmd.push_back("-C");
            unpack_cmd.push_back("/tmp/archives/" + arch);
            run_command(unpack_cmd);
            cout<<"Архив разархивирован"<<"\n";
        }

        if(fs::exists(unpack_dir/"CMakeLists.txt") && assembly_cmake(arch) == 0) //CMake
        {
            cout<<"Собрано с помощью CMake"<<"\n";
            cd(build_dir);
            run_command({"make", "-n", "install"},true); 
            cd(current_path);
        }
        else if(fs::exists(unpack_dir/"configure") && fs::exists(unpack_dir/"Makefile.in")  //GNU Autotools
                && fs::exists(unpack_dir/"config.h.in")  && assembly_autotools(arch) == 0)
        {   
            cout<<"Собрано с помощью GNU Autotools"<<"\n";
            cd(unpack_dir);
            run_command({"make", "-n", "install"},true); 
            cd(current_path);
        }
        else if(empty_gemspec(unpack_dir) && assembly_gem(arch, unpack_dir) == 0) //Ruby тут есть gemspec
        {
            cout<<"Собрано с помощью gem"<<"\n";
            regex r("(.*)\\.gem$", regex_constants::icase);
            string tmp_path = find_file(r,unpack_dir);
            run_command({"gem", "install", tmp_path},true);
        }
        else if(fs::exists(unpack_dir/"Rakefile") && !empty_gemspec(unpack_dir)) //Ruby тут нет gemspec
        {
            run_command({"gem", "install", "rake", "rake-compiler"},true);
            cd(unpack_dir);
            if(run_command({"rake", "--all", "-n"}) == 0) cout<<"Прогон всех шагов завершен успешно"<<"\n";  
            cd(current_path);  
        }
        else if(fs::exists(unpack_dir/"Build.PL") && assembly_perl_build(arch,path_arch) == 0) //Perl
        {
            cout<<"Собрано с помощью Build и Build.PL"<<"\n";
            cd(unpack_dir);
            run_command({"./Build", "install"},true);
            cd(current_path);
        }
        else if(fs::exists(unpack_dir/"Makefile.PL") && assembly_perl_make(arch) == 0) //Perl
        {
            cout<<"Собрано с помощью make и Makefile.PL"<<"\n";
            cd(unpack_dir);
            run_command({"make", "-n", "install"},true);
            cd(current_path);
        }
        else if(fs::exists(unpack_dir/"config.m4") && assembly_php(arch) == 0) //PHP
        {
            cout<<"Собрано с помощью phpize и GNU Autotools"<<"\n";
        
            cd(unpack_dir);
            run_command({"make", "-n", "install"},true);
            
            string tmp_dir = unpack_dir/"modules";

            string module;
            regex mask3("(.*)\\.so$", regex_constants::icase);
            module = find_file(mask3,tmp_dir,true);
            cout << "Для включения расширения добавьте в конфигурацию PHP строчку:" << endl
                 << "extension=" << module << endl;

        }
        else cout<<"Не удалось собрать пакет"<<"\n";
    }
}