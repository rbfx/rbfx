#include <random>
#include <cmath>

extern "C"
{

void randombytes(unsigned char *x,unsigned long long xlen)
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, 256);
    for (unsigned long long _ = 0; _ < xlen; ++_) {
        x[_] = dist(rng);
    }
}

}
