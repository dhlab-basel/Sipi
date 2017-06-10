//
// Created by Lukas Rosenthaler on 04.06.17.
//
#include <dirent.h>
#include <sys/stat.h>

#include "SipiFilenameHash.h"
#include "Error.h"

const static char __file__[] = __FILE__;

SipiFilenameHash::SipiFilenameHash(const std::string &name) {
    unsigned int hashval = 0;

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
    if ((index < 0) || (index >= hash_len)) {
        throw shttps::Error(__file__, __LINE__, "Invalid hash index!");
    }
    return (*hash)[index];
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
                    closedir(dirp);
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
    closedir(dirp);
    return (count == 26) ? level + 1 : level;
}

int SipiFilenameHash::check_levels(const std::string &imgdir) {
    bool ok;
    int levels = scanDir(imgdir, ok, 0);
    if (ok) return levels;
    throw shttps::Error(__file__, __LINE__, "inconsistency in directory tree!");
}


static void add_level(const std::string& path, int cur_level) {
    DIR *dirp = opendir(path.c_str());
    if (dirp == nullptr) {
        throw shttps::Error(__file__, __LINE__, std::string("Couldn't read directory content! Path: ") + dir, errno);
    }
    std::vector<std::string> filelist;
    struct dirent *dp;
    int n_dirs = 0;
    while ((dp = readdir(dirp)) != nullptr) {
        if (dp->d_type == DT_DIR) {
            if ((dp->d_namlen == 1) && (dp->d_name[0] >= 'A' && dp->d_name[0] <= 'Z')) {
                char tmp[3] = {'/', dp->d_name[0], '\0'};
                add_level(path + tmp, cur_level + 1);
                n_dirs++;
            }
        } else if (dp->d_type == DT_REG) {
            filelist.push_back(dp->d_name);
        }
    }
    closedir(dirp);
    if (n_dirs != 0) {
        throw shttps::Error(__file__, __LINE__, "inconsistency in directory tree!");
    }

    //
    // first create all new subdirs
    //
    for( char c = 'A'; c <= 'Z'; c++) {
        char tmp[3] = {'/', c, '\0'};
        std::string newdirname = path + tmp;
        if (mkdir(newdirname.c_str(), 0777)) {
            throw shttps::Error(__file__, __LINE__, "Creating subdir failed!", errno);
        }
    }

    for (auto fname: filelist) {
        SipiFilenameHash fhash(fname);
        char tmp[4] = {'/', fhash[cur_level], '/', '\0'};
        std::string newfname = path + tmp + fname;
        std::string oldfname = path + "/" + fname;
        if (rename(oldfname, newfname)) {
            throw shttps::Error(__file__, __LINE__, "Rename/move failed!", errno);
        }
    }
}

static void add_level(const std::string& dir, int levels) {
    DIR *dirp = opendir(dir.c_str());
    if (dirp == nullptr) {
        throw shttps::Error(__file__, __LINE__, std::string("Couldn't read directory content! Path: ") + dir, errno);
    }
    struct dirent *dp;
    std::vector<std::string> filelist;
    while ((dp = readdir(dirp)) != nullptr) {
        if (dp->d_type == DT_DIR) {
            if ((dp->d_namlen == 1) && (dp->d_name[0] >= 'A' && dp->d_name[0] <= 'Z')) {
                closedir(dirp);
                throw shttps::Error(__file__, __LINE__, "found invalid subdir!");
            }
        } else if (dp->d_type == DT_REG) {
            filelist.push_back(dp->d_name);
        }
    }
    closedir(dirp);

    for( char c = 'A'; c <= 'Z'; c++) {
        char tmp[4] = {'.', '/', c, '\0'};
        if (mkdir(tmp, 0777)) {
            throw shttps::Error(__file__, __LINE__, "Creating subdir failed!", errno);
        }
    }
    for (auto fname: filelist) {
        SipiFilenameHash fhash(fname,levels);
    }
}

void SipiFilenameHash::migrateToLevels(const std::string& imgdir, int levels) {
    int act_levels = check_levels(imgdir);
    if (levels > act_levels) {

    } else if (levels < act_levels) {

    } else {

    }

}
