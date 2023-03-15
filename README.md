# APD--Tema1
 Procesare paralel˘a folosind paradigma Map-Reduce
 
 
Pentru rezolvarea temei am folosit crearea thread-urilor din primul laborator.
Initial am rezolvat proplema secvential, urmand sa folosesc o bariera si doua
mutex-uri pentru a o paraleliza.

*In functia main
    Din fisierul de test citesc fisierele de input si le adaug intr-o stiva.
    Fiecare thread va avea asociat o structura care contine informatii 
    corespunzatoare. Retin toate structurile intr-un array.
    Creez thread-urile. Acestea vor rula functia *open_files*. Functia 
    primeste ca argument structura din array corespunzatoare thread-ului.

    Structura contine:
    - id-ul thread-ului
    - numarul de fisiere de input
    - numarul de mapers
    - numarul de reducers
    - stiva cu toate fisierele de input
    - un vector de mape. Mapa are cheia de tip int ( ce va reprezenta o putere)
        si valoarea mapata  de tip lista de int (pastreaza numerele care se pot 
        scrie ca ceva la puterea data de cheie)

*Functia open_files  
    Verific id-ul thread-ului. 
        Daca este maper:
        Cat timp mai sunt fisiere de input in stiva, scot primul fisier, dupa
         care il prelucrez.
        Scoaterea unui element din lista este o regiune critica. Folosesc un 
        mutex. Thread-ul curent face lock, retine numele fisierului, il scoate
        din stiva, dupa da unlock.

        Thread-ul ruleaza functia de prelucrare a fisierului *read_from_file*.
        Functia returneaza un map care trebuie adaugat intr-un vector.
        Adaugarea in vector reprezinta o regiune critica. Folosesc un alt 
        mutex. Thread-ul face lock , adauga in vector, face unlock.

        Dupa ce mapperii au terminat de citit toate elementele din stiva,
        Reducerii prelucreaza vectorul de mape.
        Folosesc un semafor, cand toate thread-urile au ajuns la semafor 
        (adica si mapperii au terminat), atunci reducerii incep treaba
        si ruleaza functia *reduce*.

*Functia read_from_file
    Functia este executata de mapperi si returneaza un map.
    Citesc pe rand fiecare numar din fisier si verific daca se poate scrie ca
     baza^putere (unde putere ia valori de la 2 -> reducers+1).
    Daca numarul este 1, se poate scrie ca 1 la orice putere si il adaug in map-ul
    curent in lista corespunzatoare fiecarei puteri.
    Pentru a vedea daca numarul x poate fi scris ca baza^p  folosesc un bsearch
    Baza ia valori de la left = 2 la right = sqrt(x). 

*Functia reduce
    Functia este executata de reduceri si afiseaza in fisierele de out rezultatele
    Cati reduceri am, atatea fisiere de out vor exista.
    Un reducer, ia vectorul de mape si uneste toate listele din toate map-urile
    care au ca si cheie puterea data de reducer-ul curent.
    Inainte de a face merge pe liste, le sortez, dupa care elimin toate duplicatele
    Rezultatul este dimensiunea listei.

