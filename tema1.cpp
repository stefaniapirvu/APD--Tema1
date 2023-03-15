#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <math.h>
#include <algorithm>
#include <pthread.h>
#include <filesystem>
#include <stack>


using namespace std;

// formatul structurii trimise ca argument de threaduri

struct info_t {
    
    long id; // id-ul threadului
    int number_of_files;
    int number_of_mapers; 
    int number_of_reducers;
    stack<string> *names; //stiva cu numele fisierelor de intrare
    vector<map< int, list<int>>> *vec_of_maps; 
    pthread_mutex_t *mutex;
    pthread_mutex_t *mutex2;
    pthread_barrier_t *barrier;
   

} ;

// functie executata de reduceri
// afiseaza in fisierele corespunzatoare output-urile
void reduce (vector< map <int, list<int>>> vec_of_maps , int number_of_files,int reducer) {
    list< int > final_list;
    
    // pentru fiecare map din vectorul de mape 
    // iau lista corespunzatoare puterii (datat de reducer)
    // sortez lista si fac merge intre ea si lista finala
    for (int i = 0; i < number_of_files; i++) {
        map<int , list<int>> mymap = vec_of_maps.at(i);
        mymap[reducer].sort();
        final_list.merge(mymap[reducer]);
    }

    // lista finala este sortata in urma merge-ului
    // elimin duplicatele
    final_list.unique();

    // creez si scriu in fisierele de out
    ofstream outfile;
    string number = to_string(reducer);
    string mystr ="out" + number +".txt";

    ofstream out_file(mystr);
    out_file << final_list.size();
    out_file.close();

}


// functie executata de mapperi ce returneaza o mapa
// mapa contine numerele din fisier ce sunt putere de 2, 3..reducer
map<int , list<int>>  read_from_file (string file_name, int number_of_reducers, int id) {
    ifstream file (file_name);
    int count;
    int x;
    map <int, list<int>> mymap;
   
    if ( file.is_open() ) { 
        file >> count;
    
        // se citeste fiecrae numar din fisier
        for (int  j = 0; j < count ; j++ ) {
            file >> x;
            // daca numarul este 1 nu se mai fac verificari 
            // se adauga in listele tuturor puterilor (cheilor din map)
            if (x == 1) {
                for (int i = 2; i <= number_of_reducers + 1; i++) {
                    mymap[i].push_back(x);
                }
            }

            // pentru fiecare putere aplic un bsearch
            for (int red = 2; red <= number_of_reducers + 1; red++) {
                // bazele posibile se afla in intervalul 2 , sqrt(x)
                long left = 2;
                long right = sqrt(x);
                long mid;
                long nr;
                while (left <= right ){
                    mid = left + (right - left) / 2;
                    nr = 1;
                    for (int i = 0; i < red; i++ ){
                        nr  *= mid;
                    }
                    if (nr == x) {
                        // am gasit un numar care ridicat la puterea 'red' este 'x'
                        // adaug 'x' in map, in lista cu cheia 'red'
                        mymap[red].push_back(x);
                        left = right + 1;

                    } else if (nr < x) {
                        left = mid + 1;
                    } else {
                        right = mid - 1;
                    }

                }
            }

        }

        
    }

    return mymap;

}

void *open_files( void *argv) {
    struct info_t *info ;
    info = (struct info_t *) argv;
    string mystr;
    map<int, list<int>> newmap;

    // verific daca thread ul este mapper
    if (info->id < info->number_of_mapers) {
        
        // atata timp cat mai am fisiere de citit din stiva
        // le citesc, le prelucrez, le elimin din stiva
        while (!info->names->empty()) {

            //pentru eliminarea din stiva folosesc un mutex
            pthread_mutex_lock(info->mutex);
            if (!info->names->empty()) {
                mystr = info->names->top();
                info->names->pop();
                
            }
            pthread_mutex_unlock(info->mutex);

            // la prelucrarea fiecarui fisier creez o mapa cu numerele cerute
            newmap = read_from_file(mystr,info->number_of_reducers, info->id);

            // fiecrae mapa o adaug intr-un vector
            // folosesc mutex la adaugarea in vector
            pthread_mutex_lock(info->mutex2);
            info->vec_of_maps->push_back(newmap);
            pthread_mutex_unlock(info->mutex2);
        }
    }

    // folosesc o bariera 
    // reducerii asteapta mapperii sa termine de prelucrat fisierele
    pthread_barrier_wait(info->barrier);  

    // verific daca threadul este reducer
    // reducerii prelucreaza simultan mapele din vector
    if (info->id >= info->number_of_mapers) {
       reduce (*(info->vec_of_maps), info->number_of_files, (info->id - info->number_of_mapers + 2 ));
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    ifstream file (argv[3]);
    
    string mystring;
    int number_of_files;
    int number_of_mapers = stoi(argv[1]);
    int number_of_reducers = stoi(argv[2]);
    stack <string> names;

    // citesc din fisierul de test 
    // adaug intr-o stiva numele fisierelor de intrare
    if (file.is_open()) { 
        file >> number_of_files;
        for (int i = 0; i < number_of_files; i++) {
            file >> mystring ;
            names.push(mystring);
        }
        
        // numarut total de threaduri este numarul de mapperi+ numarul de reduceri
        // creez un vector de structuri in care retin informatiile despre fiecare thread
        int num_threads = number_of_mapers + number_of_reducers;
        struct info_t thread_info[num_threads];
        pthread_t threads[num_threads];
        int r;
        long id;
        void *status;
        // creez o bariera si 2 mutex
        pthread_barrier_t barrier;
        pthread_barrier_init(&barrier, NULL, number_of_mapers + number_of_reducers);
        pthread_mutex_t mutex;
        pthread_mutex_init(&mutex, NULL);
        pthread_mutex_t mutex2;
        pthread_mutex_init(&mutex2, NULL);
        vector<map< int, list<int>>> vec;

    
        for (id = 0; id < num_threads; id++) {
            // pentru fiecare thread actualizez informatiile
            thread_info[id].names = &names;
            thread_info[id].vec_of_maps = &vec;
            thread_info[id].number_of_files = number_of_files;
            thread_info[id].number_of_mapers = number_of_mapers;
            thread_info[id].number_of_reducers =  number_of_reducers;
            thread_info[id].id = id;
            thread_info[id].mutex = &mutex;
            thread_info[id].mutex2 = &mutex2;
            thread_info[id].barrier = &barrier;

            // creez pe rand fiecare thread 
            // threadurile ruleaza functia open_files ce are ca argument o structura
            r = pthread_create(&threads[id], NULL, open_files, (void *) &thread_info[id] );
            
            if (r) {
                printf("Eroare la crearea thread-ului %ld\n", id);
                exit(-1);
            }
        }

        for (id = 0; id < num_threads; id++) {
            r = pthread_join(threads[id], &status);

            if (r) {
                printf("Eroare la asteptarea thread-ului %ld\n", id);
                exit(-1);
            }
        }

  
    	pthread_barrier_destroy(&barrier);
        pthread_mutex_destroy(&mutex);
        pthread_mutex_destroy(&mutex2);
        file.close();   
        pthread_exit(NULL);
    }

}