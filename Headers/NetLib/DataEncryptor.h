#ifndef NETLIB_DATAENCRYPTOR_H
#define NETLIB_DATAENCRYPTOR_H
#include <random>
#include <bitset>

typedef unsigned long long int bigInt;


class DataEncryptor
{
public:
    DataEncryptor() = default;
    ~DataEncryptor() = default;

    void InitialiseRSA();


private:
    bigInt GenerateRandomPrime();
    bool IsPrime(bigInt number);
    static bigInt ModExpo(bigInt base, bigInt exponent, bigInt modulus);
    static bigInt BigPow(bigInt base, bigInt expo);
    static int DataEncryptor::gcd(int a, int h);

    std::default_random_engine generator;
    std::uniform_int_distribution<bigInt> primeDist;

    bigInt privateRSAKey = 0;
    bigInt exponent = 3;
    bigInt publicRSAKey = 0;

    int k = 100; // Iterations of millar-rabin witness loop

};

#endif //NETLIB_DATAENCRYPTOR_H
