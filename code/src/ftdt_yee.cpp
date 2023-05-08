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
#include <array>


// Taille de la matrice de travail (un côté)
// static const int N = 100;
// static const int M = 100;
// static const int K = 100;
// static const int L = 3;

static const int N = 2;
static const int M = 2;
static const int K = 2;
static const int L = 2;

// Tampon générique à utiliser pour créer le fichier
static const int BUFFER_SIZE = N * M * K * L * sizeof(double); // vrai
// static const int BUFFER_SIZE = 2 * 2 * 2 * 2 * sizeof(double); // test
// static const int MATRIX_SIZE = 2;


// std::vector<char> buffer_[BUFFER_SIZE];

struct slice_range {
    int start_index;
    int end_index;
};



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

int get_mtx_index(int i, int j, int k, int l) 
{
    return (M*N*K) * i + (M*N) * j + M* k + l;
}

// void slice(std::vector<double>  mtx, int start_index, int end_index) 
void slice(std::vector<double>  mtx) 
{

}

void curl(double* mtx)
{
    // make a copy of mtx into E, put zeros into mtx

    // double* E;

    // make a copy of actual E matrix
    std::vector<double> mtx_E (mtx, mtx + N * M * K * L);

    // std::vector<double> mtx_E;
    // mtx_E.assign(mtx, mtx + sizeof(mtx) / sizeof(mtx[0]));

    std::cout << "E\n";    
    // for (int i = 0; i < MATRIX_SIZE; i++) {
    //     for (int j = 0; j < MATRIX_SIZE; j++) {
    //         std::cout << mtx_E;
    //     }
    //     std::cout << std::endl;
    // }

    // for (int i = 0; i < N; i++) { PRINT mtx_E
    //     for (int j = 0; j < M; j++) {
    //         for(int k = 0; k < K; k++) {
    //             for(int l = 0; l < L; l++) {
    //                 mtx_E[get_mtx_index(i, j, k, l)];
    //                 std::cout << mtx_E[get_mtx_index(i, j, k, l)];
    //             }
    //             std::cout << std::endl;
    //         }
    //         std::cout << std::endl;
    //     }
    //     std::cout << std::endl;
    // }


    // slice
    
    std::vector<double> tmp; 

    int l = 1;

    for (int i = 0; i < N; i++) {
        for (int j = 1; j < M; j++) {
            for(int k = 0; k < K; k++) {                
                tmp.push_back(mtx_E[get_mtx_index(i, j, k, l)]);
            }
        }
    }

    std::cout << "tmp "; 
    std::cout << tmp.size() << std::endl; 

    for (double i: tmp)
        std::cout << i << ' ';

    std::cout << std::endl;
         


    // slice_range ranges[4] = {{}}

    // std::copy(mtx.begin(), mtx.end(), )

    // TODO: (possibility--) multithreading with slice (call func with index and sign)


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
    char buffer_[BUFFER_SIZE];
    memset(buffer_, 0, BUFFER_SIZE);     // sets all values to zero
    // std::fill(buffer_->begin(), buffer_->end(), 0);
    // v2 = std::vector<int>(v1.begin() + 1, v1.end()); slicing
    // auto v = std::vector<int>(buffer_->begin() + 1, buffer_->end()); ;

    FILE* shm_f = fopen(argv[1], "w");              
    fwrite(buffer_, sizeof(char), BUFFER_SIZE, shm_f); // writes file with zeros
    fclose(shm_f);

    // On signale que le fichier est prêt.
    std::cerr << "CPP:  File ready." << std::endl;
    // ack_signal(); DECOMMENTER

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
    double* mtx = (double*)shm_mmap; // rergarder pour utiliser des vectors a la place 

    // for (int i = 0; i < MATRIX_SIZE; i++) {
    //     for (int j = 0; j < MATRIX_SIZE; j++) {
    //         // mtx[i*MATRIX_SIZE + j] += 1.0;
    //         std::cout << mtx[i*MATRIX_SIZE + j];

    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;
    int cpt = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            for(int k = 0; k < K; k++) {
                for(int l = 0; l < L; l++) {
                    mtx[get_mtx_index(i, j, k, l)] += cpt;
                    // std::cout << mtx[get_mtx_index(i, j, k, l)];
                    cpt++;
                }
            }
        }
        std::cout << std::endl;
    }

    curl(mtx);

    // std::array<double, BUFFER_SIZE> mtx = reinterpret_cast<double>(shm_mmap);
    

    // while(true) { // DECOMMENTER
    //     // On attend le signal du parent.
    //     wait_signal();
    //     std::cerr << "CPP: Will curl E" << std::endl;
    //     curl(mtx);
    //     // On signale que le travail est terminé.
    //     std::cerr << "CPP: Work done." << std::endl;
    //     ack_signal();
    // }

    munmap(shm_mmap, BUFFER_SIZE);
    return 0;
}
