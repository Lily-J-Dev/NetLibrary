#include <cmath>
#include <memory>
#include <iostream>

#include "NetLib/DataEncryptor.h"

void DataEncryptor::InitialiseRSA() {
    // Get an initial non-deterministic random number
    std::random_device device;
    std::uniform_int_distribution<bigInt> initDist(0, (bigInt)-1);
    // Use the random device to create a unique random generator
    generator = std::default_random_engine(initDist(device));

    // Find the max and min value for the primes
    bigInt maxPrime = pow(2, (sizeof(bigInt)) / 2);
    bigInt minPrime = maxPrime / 2;
    primeDist = std::uniform_int_distribution<bigInt>(minPrime, maxPrime);

    // Generate two primes
    bigInt prime1 = GenerateRandomPrime(); // p
    bigInt prime2 = GenerateRandomPrime(); // q
    prime1 = 7;
    prime2 = 11;
    publicRSAKey = prime1*prime2; // n = pq;

    bigInt totient = (prime1 - 1) * (prime2 -1);
    exponent = 2;
    bigInt count = 0;
    //for checking co-prime which satisfies e>1
    while(exponent<totient){
        count = gcd(exponent,totient);
        if(count==1)
            break;
        else
            exponent++;
    }


    bigInt x = 1;
    while((1 + (x*totient)) % exponent != 0)
    {
        x++;
    }

    privateRSAKey = (1 + (x*totient)) / exponent;

    bigInt test = 3;
    bigInt cipher = BigPow(test, exponent) % publicRSAKey;
    bigInt testDecrypt =BigPow(cipher, privateRSAKey) % publicRSAKey;

}

bigInt DataEncryptor::GenerateRandomPrime()
{
    bigInt prime = primeDist(generator);
    while(!IsPrime(prime))
    {
        prime = primeDist(generator);
    }

    return prime;
}

// Using The Miller-Rabin primality test
bool DataEncryptor::IsPrime(bigInt number) {
    // Manually check edge cases
    if (number < 2 || number == 4 || number % 2 == 0) return false;
    if (number < 4) return true;

    // Solve n = 2^r·d + 1
    // Find the highest value for D, such that
    bigInt d = number - 1; // 2^r.d = n-1
    bigInt r = 0;

    while (d % 2 == 0) {
        d /= 2;
        r++;
    }
    auto dist = std::uniform_int_distribution<bigInt>(2, number - 2);

    for (int i = 0; i < k; i++) {
        bigInt a = dist(generator);
        bigInt x = ModExpo(a, d, number);

        if (x == 1 || x == number - 1)
            continue;

        bool composite = true;
        for (int j = 0; j < r - 1; j++) {
            x = (x * x) % number;
            if (x == number - 1)
            {
                composite = false;
                break;
            }
        }
        if (composite)
            return false;
    }
    return true;
}

// Finds the modular exponent for a given base, exponent and modulus
bigInt DataEncryptor::ModExpo(bigInt base, bigInt exponent, bigInt modulus)
{
    if(modulus == 0)
        return 0;

    bigInt result = 1;
    base = base % modulus;
    while(exponent > 0)
    {
        // If the exponent is odd
        if(exponent % 2 == 1)
        {
            result = (result*base) % modulus;
        }
        // Shift bits right
        exponent = exponent >> 1u;
        base = (base*base) % modulus;
    }
    return result;
}

// Pow using the Exponentiating by squaring algorithm
bigInt DataEncryptor::BigPow(bigInt base, bigInt expo)
{
    if(expo == 1)
        return base;
    else if(expo % 2 == 0)
    {
        base = base*base;
        bigInt mult = base;
        expo /= 2;
        for(bigInt i = 1; i < expo; i++)
            base *= mult;
        return base;
    }
    else
    {
        bigInt newBase = base*base;
        bigInt newExpo = (expo-1) /2;
        bigInt mult = newBase;
        for(bigInt i = 1; i < newExpo; i++)
            newBase *= mult;
        base *= newBase;
        return base;
    }
}

int DataEncryptor::gcd(int a, int h)
{
    int temp;
    while(1)
    {
        temp = a%h;
        if(temp==0)
            return h;
        a = h;
        h = temp;
    }
}