#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <array>
#include <list>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>
#include <fcntl.h>
#include <experimental/filesystem>
#include <nlohmann/json.hpp>
#include <set> //множество(set) представляет такой контейнер, который может хранить только уникальные значения, применяется для
//создания коллекций, которые не должны иметь дубликатов
using namespace std;
namespace fs = std::experimental::filesystem;
using json = nlohmann::json;

vector<string> linkDirs;


//fd[0] – открыт на чтение, fd[1] – на запись (вспомните STDIN == 0, STDOUT == 1)

bool is_admin() {
#ifdef WIN32
    // TODO
    return true;
#else
    return getuid() == 0; //краткая запись if(getuid()==0) {return true;} else {return false;}
#endif
}

void analysis_pkgconfig(vector<string> req_libs, string path);

int run_command(vector<string> cmd, bool need_admin_rights = false, int *stdout_pipe = nullptr, int recurs = 0, string PATH = "",bool found = true) {
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
                dup2(stdout_pipe[1], STDOUT_FILENO);
                //dup2() — Дублирует дескриптор открытого файла в другой
                //int dup2(int fd1, int fd2);
                //Возвращает дескриптор файла со значением fd2 , который теперь ссылается на тот же файл, что и fd1 . 
                //Файл, на который ранее ссылался fd2, закрывается

                //int dup2(int oldhandle, int newhandle);
                //Функция dup2() дублирует oldhandle как newhandle. Если имеется файл, который был связан с new_handle до 
                //вызова dup2(), то он будет закрыт. В случае успеха возвращается 0, а в случае ошибки —1. При ошибке errno 
                //устанавливается в одно из следующих значений

                //dup2 создает    новый   дескриптор   со   значением
                //newhandle  Если  файл  связанный   с   дескриптором
                //newhandle   открыт,   то   при   вызове   dup2   он
                //закрывается.

                //Переменные newhandle и oldhandle - это  дескрипторы файлов


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
                //FILE *fdopen(int handle, char *type);
                //Функция fdopen() связывает входной или выходной поток с файлом, который идентифицируется дескриптором handle. 
                //Переменная type представляет собой строку символов, определяющую тип доступа, запрашиваемый для потока. 
                //Тип данных FILE.
                // Этот тип данных определяет поток и содержит информацию, необходимую для управления потоком, в том числе указатель на буфер  потока, и его показатели состояния
                FILE *f = fdopen(stdout_pipe[0], "r");
                if (!f) 
                {
                    //Функция strerror() возвращает указатель на сообщение об ошибке, связанное с номером ошибки.
                    auto es = strerror(errno);
                    cerr << "fdopen failure: " << es << endl;
                    exit(1);
                }
                //std::size_t— это беззнаковый целочисленный тип результата оператора sizeof
                //size_t может хранить максимальный размер теоретически возможного объекта любого типа (включая массив).
                //ssize_t - тот же size_t, но только знаковый
                /*возможные ситуации:
                1)Прочитали ровно то количество байт, сколько было в файле и тогда nread = 0 - не ошибка
                2)Прочитали меньшее число байт, чем планировалось и больше читать нечего, 0 - тоже не ошибка
                3)Когда ничего не было изначально для чтения - будет 0 и это уже ошибка в рамках данной программы и этой логики(не найден файл,например)
                */

               //Указатели представляют собой объекты, значением которых служат адреса других объектов 
               //(переменных, констант, указателей) или функций. Как и ссылки, указатели применяются для косвенного доступа 
               //к объекту. 

                //ssize_t getline(char **lineptr, size_t *n, FILE *stream);
                //Функция getline() считывает целую строку из stream, сохраняет адрес буфера с текстом в *lineptr. 
                //Буфер завершается null и включает символ новой строки, если был найден разделитель для новой строки.
                //Если *lineptr равно NULL и *n(разыменование указателя) равно 0 перед вызовом, то getline() выделит буфер для хранения строки. 
                //Этот буфер должен быть высвобожден программой пользователя, даже если getline() завершилась с ошибкой.

                //При успешном выполнении getline() возвращает количество считанных символов, включая символ разделителя, 
                //но не включая завершающий байт null ('\0'). Это значение может использоваться для обработки 
                //встроенных байтов null при чтении строки.

                //Функция возвращает -1 при ошибках чтения строки (включая условие достижения конца файла). 
                //При возникновении ошибки в errno сохраняется её значение.
                
                //Указатель на указатель — это именно то, что вы подумали: указатель, который содержит адрес другого указателя.
                //Но так как указатель хранит адрес, то мы можем по этому адресу получить хранящееся там значение. 
                //Для этого применяется операция * или операция разыменования.Результатом этой операции всегда является объект, на который указывает указатель.
                
                //char** lineptr = &line; size_t* n = &linesize
                char *line= NULL;
                size_t linesize = 0;
                ssize_t linelen;
                bool not_found_lib = false;
                
                //int* ptr;
                //int** ptr1 = &ptr;
                //*ptr1 == ptr
                //**ptr1 == *ptr
                if(recurs == 0)
                {
                    bool flag = false;
                    string name_package_for_lib = "";
                    string spare_package_for_lib = "";
                    while((linelen = getline(&line, &linesize, f)) != -1)
                    {
                        name_package_for_lib = "";
                        if(line[0] != 'S' && count(line, line + linelen, '|') == 3)
                        {
                            flag = true;
                            int c = 0;
                            int ind1 = 0;
                            int ind2 = 0;
                            
                            for(int i = 0; i < linelen; i++)
                            {
                                if(line[i] == '|' && (ind1 == 0 || ind2 == 0))
                                {
                                    c++;
                                    if(c==1) ind1 = i;
                                    else if(c==2) ind2 = i;
                                }
                            }

                            for(int j = ind1 + 1; j < ind2; j++)
                                name_package_for_lib+=line[j];
                            if(name_package_for_lib[0] == ' ')
                                name_package_for_lib.erase(0,1);
                            int size = name_package_for_lib.size();
                            while(name_package_for_lib[size - 1] == ' ')
                            {
                                name_package_for_lib.erase(size - 1);
                                size = name_package_for_lib.size();
                            }
                            if(found)
                            {
                                cout << name_package_for_lib << endl;
                                int mypipe[2];
                                if(pipe(mypipe))
                                {
                                    perror("Ошибка канала");
                                    exit(1);
                                }
                                run_command({"zypper","info", "--requires",name_package_for_lib},false,mypipe,1); //точно одно выводится
                            }
                            else
                            {
                                regex reg1("^(lib).*");
                                regex reg2("-");
                                regex reg3("32bit");
                                regex reg4("64bit");
                                if(regex_match(name_package_for_lib, reg1) && !regex_search(name_package_for_lib,reg2))
                                {
                                    not_found_lib = true;
                                    cout << name_package_for_lib << endl;
                                    //нужно найти последнюю версию пакета (идут в порядке возрастания)
                                    //run_command({"zypper", "install", "-y", name_package_for_lib},true);
                                    //break;
                                }
                                else if(!regex_search(name_package_for_lib,reg3) && !regex_search(name_package_for_lib,reg4))
                                    spare_package_for_lib = name_package_for_lib;
                            }   
                        }
               
                    }

                    //высвобождение буфера
                    free(line);
                    if(!flag)
                    {
                        cerr << "Не удалось найти пакет, из которого поставляется библиотека\n";
                        exit(1);

                    }

                    if(!found && !not_found_lib) //если библиотека не установлена и если не найден пакет lib.....
                        run_command({"zypper", "install", "-y", spare_package_for_lib},true);
                    else if(!found && not_found_lib) 
                        run_command({"zypper", "install", "-y", name_package_for_lib},true);  //установка последней версии пакета
                }
                else if(recurs == 1) //считывается вывод команды zypper info --requires 
                {
                    bool requires = false;
                    regex r1("Requires");
                    regex r2("Требует");
                    regex r3("pkgconfig");
                    regex r4("=");
                    regex r5("-devel");
                    
                    string install_package_name = "";
                    while(linelen = getline(&line, &linesize, f) != -1)
                    {
                        if(regex_search(line,r2) || regex_search(line,r1))
                            requires = true;
                        if(requires)
                        {   
                            if(regex_search(line,r3)) //pkgconfig(name)
                            {
                                install_package_name = line;
                                while(install_package_name[0] == ' ')
                                    install_package_name.erase(0,1); 
                                int pos1 = install_package_name.find("(");
                                int pos2 = install_package_name.find(")");
                                install_package_name = install_package_name.substr(pos1 + 1, pos2 - pos1 -1);
                                install_package_name = install_package_name + ".pc"; 
                                cout << install_package_name;   
                                int mypipe[2];
                                if(pipe(mypipe))
                                {
                                    perror("Ошибка канала");
                                    exit(1);
                                }
                                fs::path pkgconf("/usr/lib64/pkgconfig");
                                if(fs::exists(pkgconf/install_package_name))
                                    run_command({"pkg-config", "--libs", pkgconf/install_package_name}, false, mypipe, 2, pkgconf/install_package_name);                     
                                else
                                {
                                    cerr << "Не найден файл " << install_package_name << endl;
                                    exit(1);
                                }
                            }
                            else if(regex_search(line,r4) || regex_search(line,r5)) //name = version || name-devel
                            {
                                install_package_name = line;
                                while(install_package_name[0] == ' ')
                                    install_package_name.erase(0,1); 
                                cout << install_package_name;
                                run_command({"zypper", "install", "-y", install_package_name},true);
                                
                            }
                        }
                    }

                    free(line);
                }
                else if(recurs == 2)
                {
                    bool dependencies = false;
                    vector<string> req_libs;
                    while(linelen = getline(&line, &linesize, f) != -1) 
                    {
                        string tmp_line;
                        string tmp_lib;
                        tmp_line = line;
                        int start_ind = 0;
                        int curr_ind;
                        int count_probel = 0;
                        for(int i = 0; i < tmp_line.size(); i++)
                        {
                            if(tmp_line[i] == ' ')
                            {
                                count_probel++;
                            }
        
                        }

                        if(count_probel == 0)
                            req_libs.push_back(line);
                        else
                        {
                            for(int i = 0; i < tmp_line.size(); i++)
                            {
                                if(tmp_line[i] == ' ')
                                {
                                    curr_ind = i;
                                    tmp_lib = tmp_line.substr(start_ind,curr_ind - start_ind);
                                    start_ind = i + 1; //индекс первого после пробела элемента
                                    req_libs.push_back(tmp_lib);
                                }
                            }

                            tmp_lib = tmp_line.substr(start_ind);
                            req_libs.push_back(tmp_lib);

                        }
                        //for(auto i: req_libs)
                        //    cout << i << endl;
                        dependencies = true;

                    }
                    if(!dependencies)
                        cout << "Нет зависимостей\n";
                    else
                        //analysis_pkgconfig(req_libs, PATH); //анализ путей из pkg-config с ключом -L
                    {
                        int mypipe[2];
                        for(auto l: req_libs)
                        {
                            if(pipe(mypipe)) 
                            {
                                perror("Ошибка канала");
                                exit(1);
                            }
                            string so_lib = "";
                            string a_lib = "";
                            string tmp_lib_name = "";
                    
                            if(l[0] == '-' && l[1] == 'l') //-lname = libname.so || libname.a
                            {
                                tmp_lib_name = l.substr(2); //убираем -l
                                so_lib = "lib" + tmp_lib_name + ".so";
                                a_lib = "lib" + tmp_lib_name + ".a"; 

                            }    
                            else if(l[0] == '-' && l[1] == 'p') //-pthread=-lpthread=libpthread.so || libpthread.a
                            {
                                tmp_lib_name = l.substr(1); //убираем -
                                so_lib = "lib" + tmp_lib_name + ".so";
                                a_lib = "lib" + tmp_lib_name + ".a";
                            }    
                            else //прямое подключение name.a || name.so
                            {
                                regex r("(.*)\\.so");
                                if(l.back() == 'a') //.a
                                    a_lib = l;
                                else if(regex_match(l,r)) //.so
                                    so_lib = l;          
                            }

                            if(so_lib != "" || a_lib != "")
                            {
                                for(auto p : linkDirs) //ищем по всем путям указанным
                                {
                                    string path = p;
                                    
                                    int size = path.size();
                                    if(path[size - 1] == '/') //удаляется последний / в пути, если он был (нормализация пути)
                                    {
                                        path.erase(size - 1);
                                        size = path.size();
                                    }

                                    fs::path path_to_lib = path;


                                    string reg_lib_so = '/' + so_lib + ".*.*.*/";
                                    string reg_lib_a = '/' + a_lib + ".*.*.*/";

                                    if(so_lib != "" && a_lib != "") //для случая -lname -pname
                                    {
                                        if(!fs::exists(path_to_lib/so_lib) && !fs::exists(path_to_lib/a_lib))
                                        {
                                            if(run_command({"zypper","se","--provides", reg_lib_so}, false, mypipe,0,"",false) != 0)
                                                run_command({"zypper","se","--provides", reg_lib_a}, false, mypipe,0,"",false);    
                   
                                        }
                                    
                                    }
                                    else if(so_lib != "") //только .so
                                    {
                                        if(!fs::exists(path_to_lib/so_lib))
                                        {
                                            run_command({"zypper","se","--provides", reg_lib_so}, false, mypipe,0,"",false);
                                        
                                        }
                                    }
                                    else if(a_lib != "") //только .a
                                    {
                                        if(!fs::exists(path_to_lib/a_lib))
                                        {
                                            run_command({"zypper","se","--provides", reg_lib_a}, false, mypipe,0,"",false);
                                            
                                        }    

                                    }
                                }
                            }
                            else
                            {
                                cerr << "Некорректное имя библиотеки\n";
                                exit(1);
                            }

                        }
                    }


                }
                
            }
                
            do {
                waitpid(pid, &status, 0);
            } while(!WIFEXITED(status)); // WIFEXITED(status) возвращает истинное значение, если потомок нормально завершился, то есть вызвал exit или _exit, или вернулся из функции main().
            int child_status;
            if(WEXITSTATUS(status) == 0) child_status = 0;
            else child_status = 1;
            return_code = return_code || child_status; 
            /*WEXITSTATUS(status)
            возвращает восемь младших битов значения, которое вернул завершившийся дочерний процесс. Эти биты 
            могли быть установлены в аргументе функции exit() или в аргументе оператора return функции main(). Этот макрос можно 
            использовать, только если WIFEXITED вернул ненулевое значение.*/

        }   
    }
#endif
    return return_code;
}

void analysis_pkgconfig(vector<string> req_libs, string p)
{

}


void find_lib(vector<string> libs, vector<string> libsPath)
{

    string path;
    string check_l;
    int mypipe[2];
    string so_lib,a_lib;
    string tmp_lib_name; 
    bool found;

    for(auto l : libs)
    {
        found = false;
        if(pipe(mypipe)) 
        {
            perror("Ошибка канала");
            exit(1);
        }
        if(l[0] == '/') //если имя библиотеки начинается с /, то,значит, она написана с полным путем до нее
        {
            //cout << l << endl;
            fs::path path_tmp(l);
            if(fs::exists(path_tmp))
            {
                string lib_name = path_tmp.filename(); //получаем имя библиотеки
                
                run_command({"zypper", "wp", l}, false, mypipe);
                continue;
            }
            else
            {
                //Сообщение об ошибке или доставить библиотеку? Будет ли она расмолагаться по нужному пути,если доставлять?
            }
        
        }

        so_lib = "";
        a_lib = "";
        tmp_lib_name = "";
            
        if(l[0] == '-' && l[1] == 'l') //-lname = libname.so || libname.a
        {
            tmp_lib_name = l.substr(2); //убираем -l
            so_lib = "lib" + tmp_lib_name + ".so";
            a_lib = "lib" + tmp_lib_name + ".a"; 

        }    
        else if(l[0] == '-' && l[1] == 'p') //-pthread=-lpthread=libpthread.so || libpthread.a
        {
            tmp_lib_name = l.substr(1); //убираем -
            so_lib = "lib" + tmp_lib_name + ".so";
            a_lib = "lib" + tmp_lib_name + ".a";
        }    
        else //прямое подключение name.a || name.so
        {
            regex r("(.*)\\.so");
            if(l.back() == 'a') //.a
                a_lib = l;
            else if(regex_match(l,r)) //.so
                so_lib = l;          
        }

        if(so_lib != "" || a_lib != "")
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

                if(so_lib != "" && a_lib != "") //для случая -lname -pname
                {
                    if(fs::exists(path_to_lib/so_lib))
                    {
                        run_command({"zypper", "wp", path_to_lib/so_lib}, false, mypipe);
                        found = true;
                    }
                    else if(fs::exists(path_to_lib/a_lib))
                    {
                        run_command({"zypper", "wp", path_to_lib/a_lib}, false, mypipe);
                        found = true;
                      
                    }
                
                }
                else if(so_lib != "") //только .so
                {
                    if(fs::exists(path_to_lib/so_lib))
                    {
                        run_command({"zypper", "wp", path_to_lib/so_lib}, false, mypipe);
                        found = true;
                      
                    }
                }
                else if(a_lib != "") //только .a
                {
                    if(fs::exists(path_to_lib/a_lib))
                    {
                        run_command({"zypper", "wp", path_to_lib/a_lib}, false, mypipe);
                        found = true;
                        
                    }    

                }
            }
        }
        else
        {
            cerr << "Некорректное имя библиотеки\n";
            exit(1);
        }

        if(!found) //библиотека не нашлась ни по одному из путей
        {
            string reg_lib_so = '/' + so_lib + ".*.*.*/";
            string reg_lib_a = '/' + a_lib + ".*.*.*/";
            if(so_lib != "" && a_lib != "")
            {
                if(run_command({"zypper","se","--provides", reg_lib_so}, false, mypipe,0,"",false) != 0)
                    run_command({"zypper","se","--provides", reg_lib_a}, false, mypipe,0,"",false);

            }
            else if(so_lib != "")
                run_command({"zypper","se","--provides", reg_lib_so}, false, mypipe,0,"",false);
            else if(a_lib != "")
                run_command({"zypper","se","--provides", reg_lib_a}, false, mypipe,0,"",false);
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
            if(regex_match(entry.path().c_str(),mask1))
            {
                pathToFile = entry.path();
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
            if(regex_match(entry.path().c_str(),mask2))
            {
                pathToFile = entry.path();
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
            if(regex_match(entry.path().c_str(),mask))
            {
                pathToFile = entry.path();
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

void check_libs(vector<string> libs)
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
        string build_dir = "/tmp/archives/oicb-master/build-oicb-master";
        int build_dir_name_size = build_dir.size();

        while(build_dir[build_dir_name_size - 1] == '/')
        {
            build_dir.erase(build_dir_name_size - 1); //удаляется 1 символ с build_dir_name_size - 1 индекса
        //удаляется последний / в пути, если он был
            build_dir_name_size = build_dir.size();
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
                if(build_dir == path)
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
                if(tmp_path == (build_dir + "/")) //  Сранивается с добавлением /, так как возможнока ситуация /foo /foobar
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
            tmp = tmp.substr(0,3);
            if(tmp != "-Wl")
                libs.push_back(j["link"]["commandFragments"][ind]["fragment"]);

        }

    }

    return libs;

}

void libs(regex mask, fs::path pathToDir)
{
    //toolchain точно один
    regex mask1(".*(toolchains).*",regex_constants::icase);

    check_toolchain(mask1, mask, pathToDir);

    linkDirs = linkDirectories(mask1,"/tmp/archives/oicb-master/build-oicb-master/.cmake/api/v1/reply"); //Все каталоги, в которых стандартно ищутся библиотеки

    for(auto i: linkDirs)
        cout << i << " ";
    cout << endl;

    string pathToFile;
    fs::directory_iterator dirIterator(pathToDir);
    for(const auto& entry : dirIterator)
    {
        if(!is_directory(entry))
        {
            if(regex_match(entry.path().c_str(),mask))
            {
                //Берем бибилиотеки и пути с -L из каждого targeta, так как их может быть несколько
                vector<string> libs;//Библиотеки
                vector<string> libPath; //-L/...../
                pathToFile = entry.path();
                libs = lib_list(pathToFile);

                for(auto j: libs)
                    cout << j << " ";
                cout << endl;
                
                libPath = libpath(pathToFile); //Тут пути с -L

                if(libPath.size() != 0)
                {
                    for(auto j: libPath)
                        cout << j << " ";
                    cout << endl;

                    check_libs(libPath);

                    find_lib(libs, libPath);

                }

                else
                    find_lib(libs, linkDirs);
                       
            }

        }

    }


}

int main()
{
    regex mask1(".*(target).*",regex_constants::icase);
    //bool return_lib = check_libs(mask1,"/tmp/archives/oicb-master/build-oicb-master/.cmake/api/v1/reply");
    //libs(mask1,"/tmp/archives/xz-5.4.6/build-xz-5.4.6/.cmake/api/v1/reply");
    libs(mask1,"/tmp/archives/oicb-master/build-oicb-master/.cmake/api/v1/reply");
   
    return 0;

}