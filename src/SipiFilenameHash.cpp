//
// Created by Lukas Rosenthaler on 04.06.17.
//
#include <dirent.h>

#include "SipiFilenameHash.h"
#include "Error.h"

const static char __file__[] = __FILE__;

SipiFilenameHash::SipiFilenameHash(const std::string &name, int hash_len_p) : hash_len(hash_len_p) {
    if ((hash_len < 1) || (hash_len > 6)) throw shttps::Error(__file__, __LINE__, "invalid hash length!");
    unsigned int modval = numchars;
    unsigned int hashval = 0;

    for (int i = 1; i < hash_len; i++) modval *= numchars;

    for(auto c : name) {
        hashval = ((hashval * seed) + c) % modval;
    }

    hash = new std::vector<char>(hash_len, 0);

    for (int i = 0; i < hash_len; i++) {
        (*hash)[i] = 'A' + hashval % modval;
        hashval /= numchars;
    }
}


SipiFilenameHash::SipiFilenameHash(const SipiFilenameHash& other) {
    hash = new std::vector<char>(*(other.hash));
}

SipiFilenameHash& SipiFilenameHash::operator=(const SipiFilenameHash& other) {
    hash = new std::vector<char>(*(other.hash));
}

SipiFilenameHash::~SipiFilenameHash() {
    delete [] hash;
}


char &SipiFilenameHash::operator[] (int index) {
    char c;
    try {
        c = (*hash).at(index);
    } catch(const std::out_of_range& oor) {
        throw shttps::Error(__file__, __LINE__, "Invalid hash index!");
    }
    return c;
}


static int scanDir(const std::string &path, bool &ok, int level) {
    int res = -1;
    DIR *dirp = opendir(path.c_str());
    if (dirp == nullptr) {
        throw shttps::Error(__file__, __LINE__, std::string("Couldn't read directory content! Path: ") + path, errno);
    }
    struct dirent *dp;
    int count = 0;
    int sublevel = -1;
    while ((dp = readdir(dirp)) != nullptr) {
        if (dp->d_type == DT_DIR) {
            if ((dp->d_namlen == 1) && (dp->d_name[0] >= 'A' && dp->d_name[0] <= 'Z')) {
                char tmp[3] = {'/', dp->d_name[0], '\0'};
                bool is_ok;
                int old_sublevel = sublevel;
                sublevel = scanDir(path + tmp, is_ok, level + 1);
                if ((old_sublevel >= 0) && (old_sublevel != sublevel)) {
                    throw shttps::Error(__file__, __LINE__, "inconsistency in directory tree!");
                }
                if (is_ok) { // we have either 0 or 26 subdirs
                    count++;
                }
            }
        }
    }
    if ((count == 0) || (count == 26)) { // no or 26 subdirs
        ok = true;
    } else {
        ok = false;
    }
    return (count == 26) ? level + 1 : level;
}

int SipiFilenameHash::check_levels(const std::string &path) {
    bool ok;
    int levels = scanDir(path, ok, 0);
    if (ok) return levels;
    throw shttps::Error(__file__, __LINE__, "inconsistency in directory tree!");
}