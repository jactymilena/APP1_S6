#include <iostream>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <vector>

// Taille de la matrice de travail (un côté)
static const int N = 100;
static const int M = 100;
static const int K = 100;
static const int L = 3;

static const int BUFFER_SIZE = N * M * K * L * sizeof(double);
// Tampon générique à utiliser pour créer le fichier

// std::vector<char> buffer_[BUFFER_SIZE];

char buffer_[BUFFER_SIZE];


void wait_signal()
{
    // Attend une entrée (ligne complète avec \n) sur stdin.
    std::string msg;
    std::cin >> msg;
    std::cerr << "CPP: Got signal." << std::endl;
}

void ack_signal()
{
    // Répond avec un message vide.
    std::cout << "" << std::endl;
}

// char get_4dmatrix_element(int i, int j, int k, int l) 
// {
//     return buffer_[(M*N*K) * i + (M*N) * j + M* k + l];
// }

void slice() 
{


}

void curl(double* mtx)
{

}

int main(int argc, char const *argv[])
{

    if (argc < 2)
    {
        std::cerr << "Error : no shared file provided as argv[1]" << std::endl;
        return -1;
    }

    // wait_signal();

    // Création d'un fichier "vide" (le fichier doit exister et être d'une
    // taille suffisante avant d'utiliser mmap).
    memset(buffer_, 0, BUFFER_SIZE);     // sets all values to zero
    // std::fill(buffer_->begin(), buffer_->end(), 0);
    // v2 = std::vector<int>(v1.begin() + 1, v1.end()); slicing
    // auto v = std::vector<int>(buffer_->begin() + 1, buffer_->end()); ;

    FILE* shm_f = fopen(argv[1], "w");              
    fwrite(buffer_, sizeof(char), BUFFER_SIZE, shm_f); // writes file with zeros
    fclose(shm_f);

    // On signale que le fichier est prêt.
    std::cerr << "CPP:  File ready." << std::endl;
    ack_signal();

    // On ré-ouvre le fichier et le passe à mmap(...). Le fichier peut ensuite
    // être fermé sans problèmes (mmap y a toujours accès, jusqu'à munmap.)
    int shm_fd = open(argv[1], O_RDWR);
    void* shm_mmap = mmap(NULL, BUFFER_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    close(shm_fd);

    if (shm_mmap == MAP_FAILED) {
        std::cerr << "ERROR SHM\n";
        perror(NULL);
        return -1;
    }

    // Pointeur format double qui représente la matrice partagée:
    double* mtx = (double*)shm_mmap;

    while(true) {
        // On attend le signal du parent.
        wait_signal();
        std::cerr << "CPP: Will curl E" << std::endl;
        curl(mtx);
        // On signale que le travail est terminé.
        std::cerr << "CPP: Work done." << std::endl;
        ack_signal();
    }

    munmap(shm_mmap, BUFFER_SIZE);
    return 0;
}
