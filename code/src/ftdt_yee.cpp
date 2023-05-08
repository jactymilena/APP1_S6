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
static const int N = 100;
static const int M = 100;
static const int K = 100;
static const int L = 3;

// static const int N = 2;
// static const int M = 2;
// static const int K = 2;
// static const int L = 2;

// Tampon générique à utiliser pour créer le fichier
// static const int BUFFER_SIZE = N * M * K * L * sizeof(double); // vrai
static const int BUFFER_SIZE = N * M * K * L * sizeof(double); // vrai

// static const int BUFFER_SIZE = 2 * 2 * 2 * 2 * sizeof(double); // test
// static const int MATRIX_SIZE = 2;


// std::vector<char> buffer_[BUFFER_SIZE];

char buffer_[BUFFER_SIZE];


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

int get_mtx_index(int i, int j, int k) //int l) 
{
    // return (M*N*K) * i + (M*N) * j + M* k + l;
    return (M * N) * i + M * j + k;

}

std::vector<double> slice(std::vector<double>  mtx, slice_range ranges[3]) 
{
    std::vector<double> tmp;    
    for (int i = ranges[0].start_index; i < ranges[0].end_index; i++) {
        for (int j = ranges[1].start_index; j < ranges[1].end_index; j++) {
            for(int k = ranges[2].start_index; k < ranges[2].end_index; k++) {                
                tmp.push_back(mtx[get_mtx_index(i, j, k)]);
            }
        }
    }

    return tmp;
}

void diff_vectors(std::vector<double> &v_sum, std::vector<double> v) 
{
    for(int i = 0; i < N; i++) {
        v_sum[i] -= v[i];
    }
}

void replace_mtx(double* mtx, std::vector<double> v) 
{
    for(int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            for(int k = 0; k < K; k++) {
                mtx[get_mtx_index(i, j, k)] = v[get_mtx_index(i, j, k)];
            }
        }
    }
}

void curl(double* mtx)
{
    // TODO: make a copy of mtx into E, put zeros into mtx


    // make a copy of actual E matrix
    std::vector<double> mtx_E (mtx, mtx + N * M * K);

    std::cerr << "E\n";
    // std::cerr << mtx_E.size();    

    slice_range ranges[3] = {{0, N}, {1, M}, {0, K}};
    std::vector<double> m1 = slice(mtx_E, ranges);

    slice_range ranges2[3] = {{0, N}, {0, M-1}, {0, K}};
    std::vector<double> m2 = slice(mtx_E, ranges2);


    // std::cout << "tmp "; 
    // std::cout << m1.size() << std::endl; 
    // std::cout << m2.size() << std::endl; 


    // for (double i: tmp)
    //     std::cout << i << ' ';

    diff_vectors(m1, m2);
    replace_mtx(mtx, m1);
    
    /* TODO: 
        - put zeros into mtx before addition
        - idee pour le reste : for comme celui en haut pour l'addition avec les deux tableau deja slice
    */
    // TODO: (possibility--) multithreading with slice (call func with index and sign)


}

int main(int argc, char const *argv[])
{

    if (argc < 2)
    {
        std::cerr << "Error : no shared file provided as argv[1]" << std::endl;
        return -1;
    }

    wait_signal();

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
    ack_signal(); //DECOMMENTER

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


    // A ENLEVER : FEED MATRIX
    int cpt = 1;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            for(int k = 0; k < K; k++) {
                // for(int l = 0; l < L; l++) {
                    mtx[get_mtx_index(i, j, k)] += cpt;
                    // std::cout << mtx[get_mtx_index(i, j, k, l)];
                    cpt++;
                // }
            }
        }
    }
    
    wait_signal();
    std::cerr << "Before curl E\n";
    // ack_signal(); //DECOMMENTER
    // curl(mtx);

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

    // munmap(shm_mmap, BUFFER_SIZE);
    return 0;
}
