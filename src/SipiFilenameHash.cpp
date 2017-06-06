//
// Created by Lukas Rosenthaler on 04.06.17.
//

#include "SipiFilenameHash.h"
#include "SipiError.h"

const static char __file__[] = __FILE__;

SipiFilenameHash::SipiFilenameHash(const std::string &name, int hash_len) {
    if ((hash_len < 1) || (hash_len > 6)) throw Sipi::SipiError(__file__, __LINE__, "invalid hash length!");
    unsigned int modval = 26;
    unsigned int hashval = 0;

    for (int i = 1; i < hash_len; i++) modval *= 26;

    for(auto c : name) {
        hashval = ((hashval * seed) + c) % modval;
    }

    for (int i = 0; i < hash_len; i++) {
        hash[i] = 'A' + hashval % modval;
        hashval /= 26;
    }
}

char &SipiFilenameHash::operator[] (int index) {
    char c;
    try {
        c = hash.at(index);
    } catch(const std::out_of_range& oor) {
        throw Sipi::SipiError(__file__, __LINE__, "Invalid hash index!");
    }
    return c;
}