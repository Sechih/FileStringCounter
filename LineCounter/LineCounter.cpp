#include <windows.h>
#include <iostream>
#include <thread>
#include <fstream> 
#include <vector>
#include <mutex>
using namespace std;


//C:\Users\Vladislav\Desktop\tst\test_dir\
//c:\Users\sechix\Desktop\tst\test_dir\

enum class Status:uint8_t {NoError=0, NoFiles};
enum class FileReadStatus :bool { Complete, Error };

namespace Fle{//отдельное пространство имен, куда можно дабавлять еще функций по работе с файлами
/**
 \brief  Найти количество строк
 \note
 \param [in] - указатель на строку
 \retval [out] - количество строк в файле
  */
int NumOfLinesInFile(char* path)
   {    
     ifstream fl(path, ios::in);
     if (!fl) return -1;
     int count = 0;
     char buffer[1000]; 
     while (!fl.eof())
     {
       count++;
       fl.getline(buffer, 1000);
     }
        fl.close();
        return count;
   }
}

/*
 * Структура для хранения сведений о файле 
 */
struct FoundFiles
{
    unsigned int num_of_string;//количество строк в файле
    string pathToFile; // путь к данному файлу
    string name; //имя файла
    FileReadStatus status;// статус на случай не открытия файла
};

/** Класс небольшой, поэтому методы не выносил за пределы
  * Сущность для определения количества строк в файле 
  */
class FindStringInDirectory{
public:
    FindStringInDirectory()
    {
        cores = Get_Cores(); //в задании сказано задействовать все ядра, не вижу смысла делать больше потоков чем ядер на ПК
    }
    /**
     \brief  Установить путь для поиска
      */
    void Set_Path(string path)
    {
        this->path = path;
    }
    uint32_t Get_NumOfFiles()
    {
        return num_of_files;
    }
    Status Get_Status()
    {
        return status;
    }
    /**
     \brief  Запустить обработку по указанному пути
      */
    void HandleStart()
    {
        status = Status::NoError;
        files.clear();
        curr_file = 0;
        FindTextFiles(); //определяем количество файлов директории        
        if (status == Status::NoFiles) {
            ConsoleShowError();
            return;//проверить статус
        }
        std::vector<std::thread> trds;
        for (std::size_t i = 0; i < cores; i++)//создаем оптимальное количество потоков исходя из ядер процессора  
            trds.push_back(std::thread([&]() {calculate(); }));     

            for (auto& th : trds)
                   th.join();    
    }
    /**
     \brief  Вывести в консоль (вывести только общее количество строк не позволила религия, ничто не мешает потом закомментить кусок)
      */
    void ConsoleDataShow() 
    {
        if (status == Status::NoFiles) return;
        unsigned int sum=0;
        for (auto& fl : files)
        {
            if (fl.status == FileReadStatus::Error)
            {
                cout << fl.name << "   ->   " << "File opening error\n";
            }
            else
            {
                cout << fl.name << "   ->   " << fl.num_of_string << "\n";
                sum += fl.num_of_string;
            }
        }
        cout << "\nThe total number of lines in all files is: " << sum << "\n";
    }
private:
    string path = "\0";//путь к директории с файлами
    unsigned int num_of_files = 0; //количество всех файлов
    unsigned int curr_file = 0;//текущая позиция для чтения
    std::vector<FoundFiles> files;//массив данных о файлах
    Status status;
    uint8_t cores;//количество ядер
    mutex mtx;
    /**
     \brief  определяем общее количество файлов в каталоге, имя файла, путь к файлу
      */
    void FindTextFiles()
    {
        WIN32_FIND_DATAA   FData;
        HANDLE hFind;
        string dir = path + "\*." + "txt";
        hFind = FindFirstFileA(dir.c_str(), &FData);
        if (hFind == INVALID_HANDLE_VALUE) {
            status = Status::NoFiles;
            num_of_files = 0;
        }
        else
        {
            do {
            ++num_of_files;
            FoundFiles fls;
            fls.pathToFile = path + (fls.name = FData.cFileName);
            files.push_back(fls);
            } while (FindNextFileA(hFind, &FData));
            FindClose(hFind);
        }
    }
    /**
    * Получить количество ядер процессора, можно было напрямую вызвать без метода, но будем надеятся на inline
    */
    inline uint8_t Get_Cores()
    {
        return thread::hardware_concurrency();
    }
    /*
    * Основной метод где рассчитываются параметры по заданию
    */
    void calculate()
    {
        unique_lock<mutex> ulk(mtx, defer_lock);
        while (curr_file != num_of_files)
        {
            ulk.lock();
            FoundFiles* fls = &files.at(curr_file);
            if (curr_file < num_of_files) ++curr_file;
            ulk.unlock();
        if (fls != nullptr) {
            int size = Fle::NumOfLinesInFile((char*)fls->pathToFile.c_str());
            if (!size)
            {
                fls->num_of_string = 0;
                fls->status = FileReadStatus::Error;
            }
            else
            {
                fls->status = FileReadStatus::Complete;
                fls->num_of_string = size;
            }
        }
        if (curr_file == num_of_files) break;
      }
    }
    void ConsoleShowError()
    {
        cout << "File No Found\n";
    }
};

class ExecutionDuration
{
public:
    ExecutionDuration()
    {
        tstart = std::chrono::high_resolution_clock::now();
    }
    ~ExecutionDuration()
    {
        tstop = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = tstop- tstart;
        cout << "Execution Duration: " << duration.count() << " s" << endl;
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> tstart, tstop;
};



int main()
{
    FindStringInDirectory txt_find;
    string path = "\0";
    std::cout << "Enter the path to the folder with .txt files:\n" ;
    std::cin >> path;
    txt_find.Set_Path(path);//добавляем путь к папке с файлами txt
    ExecutionDuration exd;//время выполнения программы
    txt_find.HandleStart();//запускаем обработку установленного пути
    txt_find.ConsoleDataShow();//выводим в консоль 



  //  system("pause");
    return 0;
}
