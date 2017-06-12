//
// Created by Lukas Rosenthaler on 04.06.17.
//
#include <iostream>
#include <fstream>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>


#include "SipiFilenameHash.h"
#include "Error.h"

const static char __file__[] = __FILE__;

SipiFilenameHash::SipiFilenameHash(const std::string &path_p) : path(path_p) {
    unsigned int hashval = 0;

    size_t pos = path.rfind("/");
    if (pos != std::string::npos) {
        name = path.substr(pos + 1);
    }
    else {
        name = path;
    }

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
    return *this;
}

SipiFilenameHash::~SipiFilenameHash() {
    delete [] hash;
}


char &SipiFilenameHash::operator[] (int index) {
    if ((index < 0) || (index >= hash_len)) {
        throw shttps::Error(__file__, __LINE__, "Invalid hash index!");
    }
    return (*hash)[index];
}

void SipiFilenameHash::copyFile(const std::string& imgdir, int levels) {
    std::ifstream source(path, std::ios::binary);
    if (source.fail()) {
        throw shttps::Error(__file__, __LINE__, std::string("Couldnt open file for reading: ") + path);
    }

    std::string outfname(imgdir);
    for (int i = 0; i < levels; i++) {
        char tmp[3] = {'/', (*hash)[i], '\0'};
        outfname += tmp;
    }
    outfname = "/" + name;
    std::ofstream dest(outfname, std::ios::binary);
    if (source.fail()) {
        throw shttps::Error(__file__, __LINE__, std::string("Couldnt open file for writing: ") + outfname);
    }

    dest << source.rdbuf();

    source.close();
    dest.close();
}


static int scanDir(const std::string &path, bool &ok, int level) {
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
        throw shttps::Error(__file__, __LINE__, std::string("Couldn't read directory content! Path: ") + path, errno);
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
            filelist.push_back(std::string(dp->d_name));
        }
    }
    closedir(dirp);

    if (filelist.size() > 0) {
        if (n_dirs != 0) {
            throw shttps::Error(__file__, __LINE__, "inconsistency in directory tree!");
        }
        //
        // first create all new subdirs
        //
        for (char c = 'A'; c <= 'Z'; c++) {
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
            if (rename(oldfname.c_str(), newfname.c_str())) {
                throw shttps::Error(__file__, __LINE__, "Rename/move failed!", errno);
            }
        }
    }
}

static bool remove_level(const std::string& path, int cur_level) {
    //
    // prepare scanning the directory
    //
    DIR *dirp = opendir(path.c_str());
    if (dirp == nullptr) {
        throw shttps::Error(__file__, __LINE__, std::string("Couldn't read directory content! Path: ") + path, errno);
    }
    std::vector<std::string> filelist;
    struct dirent *dp;
    int n_dirs = 0;
    int n_emptied_dirs = 0;
    //
    // start scanning the directory
    //
    while ((dp = readdir(dirp)) != nullptr) {
        if (dp->d_type == DT_DIR) {
            if ((dp->d_namlen == 1) && (dp->d_name[0] >= 'A' && dp->d_name[0] <= 'Z')) {
                //
                // we have subdirs – let's enter them recursively
                //
                char tmp[3] = {'/', dp->d_name[0], '\0'};
                if (remove_level(path + tmp, cur_level + 1)) {
                    // we have emptied the given path by moving files up....
                    n_emptied_dirs++;
                }
                n_dirs++;
            }
        } else if (dp->d_type == DT_REG) {
            //
            // we have a file - add it to the list of files to be moved up
            //
            filelist.push_back(std::string(dp->d_name));
        }
    }
    closedir(dirp);
    if (n_emptied_dirs == 26) {
        //
        // we have emptied all subdirs – let's remove them
        //
        size_t pos = path.rfind("/");
        for( char c = 'A'; c <= 'Z'; c++) {
            char tmp[3] = {'/', c, '\0'};
            std::string dirname = path.substr(0, pos) + tmp;
            if (rmdir(dirname.c_str())) {
                throw shttps::Error(__file__, __LINE__, "rmdir failed!", errno);
            }
        }
        return false;
    }

    if ((n_dirs == 0) && (cur_level > 0)) {
        //
        // we have no more subdirs, so we are down at the end. Let's move
        // all files one level up.
        //
        size_t pos = path.rfind("/");
        if (pos == std::string::npos) throw shttps::Error(__file__, __LINE__, "Inconsistency!");
        for (auto fname: filelist) {

            std::string newfname = path.substr(0, pos) + "/" + fname;
            std::string oldfname = path + "/" + fname;

            if (rename(oldfname.c_str(), newfname.c_str())) {
                throw shttps::Error(__file__, __LINE__, "Rename/move failed!", errno);
            }
        }
        return true;
    }
    return false;
}


void SipiFilenameHash::migrateToLevels(const std::string& imgdir, int levels) {
    int act_levels = check_levels(imgdir);
    if (levels > act_levels) {
        for (int i = 0; i < (levels - act_levels); i++) {
            add_level(imgdir, 0);
        }

    } else if (levels < act_levels) {
        for (int i = 0; i < (act_levels - levels); i++) {
            remove_level(imgdir, 0);
        }

    }
}

bool SipiFilenameHash::test__(void) {
    return true;
}