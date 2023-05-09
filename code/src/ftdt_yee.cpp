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


// Taille de la matrice de travail 
static const int N = 100;
static const int M = 100;
static const int K = 100;
static const int L = 3;

// static const int N = 2;
// static const int M = 2;
// static const int K = 2;
// static const int L = 2;

// Tampon générique à utiliser pour créer le fichier
static const int BUFFER_SIZE = N * M * K * L * sizeof(double); 
static const int DATA_SIZE = N * M * K;


// static const int BUFFER_SIZE = 2 * 2 * 2 * 2 * sizeof(double); // test
// static const int MATRIX_SIZE = 2;


// std::vector<char> buffer_[BUFFER_SIZE];

char buffer_[BUFFER_SIZE];


struct slice_range {
    int start_index;
    int end_index;
};


struct field_component {
    double x;
    double y;
    double z;

    field_component() {}
    field_component(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
};

int get_mtx_index(int i, int j, int k) 
{
    return (M * N) * i + M * j + k; 
}

int get_mtx_index_4d(int i, int j, int k, int l)
{
    return get_mtx_index(i, j, k)*L + l;
}

class Matrix {
    private:
    std::vector<field_component> data;

    public:
    Matrix() {
        for(int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                for(int k = 0; k < K; k++) {
                    data.push_back(field_component(0, 0, 0));
                }
            }
        }
    }

    Matrix(double* mtx) {
        for(int i = 0; i < N; i++) {
            for (int j = 0; j < M; j++) {
                for(int k = 0; k < K; k++) {
                    data.push_back(field_component(
                        mtx[get_mtx_index_4d(i, j, k, 0)], 
                        mtx[get_mtx_index_4d(i, j, k, 1)], 
                        mtx[get_mtx_index_4d(i, j, k, 2)]));
                }
            }
        }
    }

    field_component& operator()(int i, int j, int k)  { 
        return data[get_mtx_index(i, j, k)]; 
    }    

    double& operator()(int i, int j, int k, int l)  { 
        int idx = get_mtx_index(i, j, k);
        if(l == 0)      
            return data[idx].x;
        else if(l == 1)      
            return data[idx].y;
        else if(l == 2)      
            return data[idx].z;
        else
            return data[idx].x;
    }

    int size() { return data.size(); }

    void replace_slice(std::vector<field_component> tab, slice_range ranges[3]) {
        int cpt = 0;
        for (int i = ranges[0].start_index; i < ranges[0].end_index; i++) {
            for (int j = ranges[1].start_index; j < ranges[1].end_index; j++) {
                for(int k = ranges[2].start_index; k < ranges[2].end_index; k++) {     
                    data[get_mtx_index(i, j, k)] = tab[cpt++];
                }
            }
        }
    }
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


std::vector<double> slice(Matrix mtx, slice_range ranges[3], int axe) 
{
    std::vector<double> tmp;    
    for (int i = ranges[0].start_index; i < ranges[0].end_index; i++) {
        for (int j = ranges[1].start_index; j < ranges[1].end_index; j++) {
            for(int k = ranges[2].start_index; k < ranges[2].end_index; k++) {          
                if(axe == 0)      
                    tmp.push_back(mtx(i, j, k).x);
                else if(axe == 1)      
                    tmp.push_back(mtx(i, j, k).y);
                else if(axe == 2)      
                    tmp.push_back(mtx(i, j, k).z);
            }
        }
    }

    return tmp;
}



Matrix curl(Matrix mtx_E)
{
    // make a copy of actual E matrix
    // std::vector<double> mtx_E (mtx, mtx + N * M * K);

    std::cerr << "E\n";
    Matrix res;

    // curl_E[:, :-1, :, 0] += E[:, 1:, :, 2] - E[:, :-1, :, 2]
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M - 1; j++) {
            for (int k = 0; k < K; k++) {
                res(i, j, k, 0) += mtx_E(i, j + 1, k, 2) - mtx_E(i, j, k, 2);
            }
        }
    }

    // curl_E[:, :, :-1, 0] -= E[:, :, 1:, 1] - E[:, :, :-1, 1]
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            for (int k = 0; k < K - 1; k++) {
                res(i, j, k, 0) -= mtx_E(i, j, k + 1, 1) - mtx_E(i, j, k, 1);
            }
        }
    }

    // curl_E[:, :, :-1, 1] += E[:, :, 1:, 0] - E[:, :, :-1, 0]
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            for (int k = 0; k < K - 1; k++) {
                res(i, j, k, 1) += mtx_E(i, j, k + 1, 0) - mtx_E(i, j, k, 0);
            }
        }
    }

    // curl_E[:-1, :, :, 1] -= E[1:, :, :, 2] - E[:-1, :, :, 2]
    for (int i = 0; i < N - 1; i++) {
        for (int j = 0; j < M; j++) {
            for (int k = 0; k < K; k++) {
                res(i, j, k, 1) -= mtx_E(i + 1, j, k, 2) - mtx_E(i, j, k, 2);
            }
        }
    }

    // curl_E[:-1, :, :, 2] += E[1:, :, :, 1] - E[:-1, :, :, 1]
    for (int i = 0; i < N - 1; i++) {
        for (int j = 0; j < M; j++) {
            for (int k = 0; k < K; k++) {
                res(i, j, k, 2) += mtx_E(i + 1, j, k, 1) - mtx_E(i, j, k, 1);
            }
        }
    }

    // curl_E[:, :-1, :, 2] -= E[:, 1:, :, 0] - E[:, :-1, :, 0]
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M - 1; j++) {
            for (int k = 0; k < K; k++) {
                res(i, j, k, 2) -= mtx_E(i, j + 1, k, 0) - mtx_E(i, j, k, 0);
            }
        }
    }

    return res;
}

int main(int argc, char const *argv[])
{

    if (argc < 2)
    {
        std::cerr << "Error : no shared file provided as argv[1]" << std::endl;
        return -1;
    }

    wait_signal();  // DECOMMENTER

    // Création d'un fichier "vide" (le fichier doit exister et être d'une
    // taille suffisante avant d'utiliser mmap).
    memset(buffer_, 0, BUFFER_SIZE);     // sets all values to zero


    FILE* shm_f = fopen(argv[1], "w");              
    fwrite(buffer_, sizeof(char), BUFFER_SIZE, shm_f); // writes file with zeros
    fclose(shm_f);

    // On signale que le fichier est prêt.
    std::cerr << "CPP:  File ready." << std::endl;
    ack_signal(); // DECOMMENTER

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

    wait_signal();  // DECOMMENTER

    Matrix mtx_E(mtx);
    Matrix res_curl = curl(mtx_E);

    for(int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            for(int k = 0; k < K; k++) {
                for(int l = 0; l < L; l++) {
                    mtx[get_mtx_index_4d(i, j, k, l)] = res_curl(i, j, k, l);
                }
            }
        }
    }

    std::cerr << "CPP: Work done." << std::endl;
    ack_signal();

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
