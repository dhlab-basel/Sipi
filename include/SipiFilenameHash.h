//
// Created by Lukas Rosenthaler on 04.06.17.
//

#ifndef SIPI_SIPIFILENAMEHASH_H
#define SIPI_SIPIFILENAMEHASH_H

#include <string>
#include <vector>

class SipiFilenameHash {
private:
    std::vector<char> hash(2);
    const int seed = 137;

    SipiFilenameHash(const std::string &name, int hash_len);

    char &operator[] (int index);
};


#endif //SIPI_SIPIFILENAMEHASH_H
