/*
 * Copyright © 2016 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
 * Ivan Subotic, Tobias Schweizer, André Kilchenmann, and André Fatton.
 * This file is part of Sipi.
 * Sipi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Sipi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Additional permission under GNU AGPL version 3 section 7:
 * If you modify this Program, or any covered work, by linking or combining
 * it with Kakadu (or a modified version of that library) or Adobe ICC Color
 * Profiles (or a modified version of that library) or both, containing parts
 * covered by the terms of the Kakadu Software Licence or Adobe Software Licence,
 * or both, the licensors of this Program grant you additional permission
 * to convey the resulting work.
 * See the GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public
 * License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "spdlog/spdlog.h"

#include "SipiCache.h"
#include "Global.h"
#include "SipiError.h"


using namespace std;

static const char __file__[] = __FILE__;

namespace Sipi {

    typedef struct _AListEle {
        string canonical;
        time_t access_time;
        off_t fsize;
        bool operator < (const _AListEle& str) const {
            return (difftime(access_time, str.access_time) < 0.);
        }
        bool operator > (const _AListEle& str) const {
            return (difftime(access_time, str.access_time) < 0.);
        }
        bool operator == (const _AListEle& str) const {
            return (difftime(access_time, str.access_time) == 0.);
        }
    } AListEle;


    SipiCache::SipiCache(const string &cachedir_p, long long max_cachesize_p, unsigned max_nfiles_p, float cache_hysteresis_p)
        : _cachedir(cachedir_p), max_cachesize(max_cachesize_p), max_nfiles(max_nfiles_p), cache_hysteresis(cache_hysteresis_p)
    {
        auto logger = spdlog::get(shttps::loggername);

        if (access(_cachedir.c_str(), R_OK | W_OK | X_OK) != 0) {
            throw SipiError(__file__, __LINE__, "Cache directory not available", errno);
        }

        string cachefilename = _cachedir + "/.sipicache";
        cachesize = 0;
        nfiles = 0;

        logger->info("Cache at '") << _cachedir << "' cachesize=" << max_cachesize << " nfiles=" << max_nfiles << " hysteresis=" << cache_hysteresis;
        ifstream cachefile(cachefilename, std::ofstream::in | std::ofstream::binary);

        struct dirent **namelist;
        int n;

        if (!cachefile.fail()) {
            cachefile.seekg (0, cachefile.end);
            streampos length = cachefile.tellg();
            cachefile.seekg (0, cachefile.beg);
            int n = length / sizeof (SipiCache::FileCacheRecord);
            logger->info("Reading cache file...");
            for (int i = 0; i < n; i++) {
                SipiCache::FileCacheRecord fr;
                cachefile.read((char *) &fr, sizeof (SipiCache::FileCacheRecord));
                string accesspath = _cachedir + "/" + fr.cachepath;
                if (access(accesspath.c_str(), R_OK) != 0) {
                    //
                    // we cannot find the file – probably it has been deleted => skip it
                    //
                    logger->debug("Cache could'nt find file '") << fr.cachepath << "' on disk!";
                    continue;
                }
                CacheRecord cr;
                cr.origpath = fr.origpath;
                cr.cachepath = fr.cachepath;
                cr.mtime = fr.mtime;
                cr.access_time = fr.access_time;
                cr.fsize = fr.fsize;
                cachesize += fr.fsize;
                nfiles++;
                cachetable[fr.canonical] = cr;
                logger->info("File '") << cr.cachepath << "' adding to cache file!";
            }
        }

        //
        // now we looking for files that are not in the list of cached files
        // and we delete them
        //
        n = scandir(_cachedir.c_str(), &namelist, NULL, alphasort);
        if (n < 0) {
            perror("scandir");
        }
        else {
            while (n--) {
                if (namelist[n]->d_name[0] == '.') continue; // files beginning with "." are not removed
                string file_on_disk = namelist[n]->d_name;
                bool found = false;
                for (const auto &ele : cachetable) {
                    if (ele.second.cachepath == file_on_disk) found = true;
                }
                if (!found) {
                    string ff = _cachedir + "/" + file_on_disk;
                    logger->info("File '") << file_on_disk << "' not in cache file! Deleting...";
                    remove(ff.c_str());
                }
                free(namelist[n]);
            }
            free(namelist);
        }
    }
    //============================================================================

    SipiCache::~SipiCache()
    {
        auto logger = spdlog::get(shttps::loggername);

        logger->debug("Closing cache...");
        string cachefilename = _cachedir + "/.sipicache";

        ofstream cachefile(cachefilename, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
        if (!cachefile.fail()) {
            for (const auto &ele : cachetable) {
                SipiCache::FileCacheRecord fr;
                (void) snprintf(fr.canonical, 256, "%s", ele.first.c_str());
                (void) snprintf(fr.origpath, 256, "%s", ele.second.origpath.c_str());
                (void) snprintf(fr.cachepath, 256, "%s", ele.second.cachepath.c_str());
                fr.mtime = ele.second.mtime;
                fr.fsize = ele.second.fsize;
                fr.access_time = ele.second.access_time;
                cachefile.write((char *) &fr, sizeof (SipiCache::FileCacheRecord));
                logger->debug("Writing '") << ele.second.cachepath << "' to cache file";
            }
        }
        cachefile.close();
    }
    //============================================================================

#if defined(HAVE_ST_ATIMESPEC)
    int SipiCache::tcompare(struct timespec &t1, struct timespec &t2)
#else
    int SipiCache::tcompare(time_t &t1, time_t &t2)
#endif
    {
#if defined(HAVE_ST_ATIMESPEC)
        if (t1.tv_sec > t2.tv_sec) {
            return 1;
        }
        else if (t1.tv_sec < t2.tv_sec) {
            return -1;
        }
        else {
            if (t1.tv_sec == t2.tv_sec) {
                if (t1.tv_nsec > t2.tv_nsec) {
                    return 1;
                }
                else if (t1.tv_nsec < t2.tv_nsec) {
                    return -1;
                }
                else {
                    return 0;
                }
            }
        }
#else
        if (t1 > t2) {
            return 1;
        }
        else if (t1 < t2) {
            return -1;
        }
        else {
            return 0;
        }

#endif
        return 0; // should never reached – just for the compiler to suppress a warning...
    }
    //============================================================================

    static bool _compare_access_time_asc(const AListEle &e1, const AListEle &e2)
    {
        double d = difftime(e1.access_time, e2.access_time);

        return (d < 0.0);
    }
    //============================================================================

    static bool _compare_access_time_desc(const AListEle &e1, const AListEle &e2)
    {
        double d = difftime(e1.access_time, e2.access_time);

        return (d > 0.0);
    }
    //============================================================================

    static bool _compare_fsize_asc(const AListEle &e1, const AListEle &e2)
    {
        return (e1.fsize < e2.fsize);
    }
    //============================================================================

    static bool _compare_fsize_desc(const AListEle &e1, const AListEle &e2)
    {
        return (e1.fsize > e2.fsize);
    }
    //============================================================================

    int SipiCache::purge() {
        auto logger = spdlog::get(shttps::loggername);

        if ((max_cachesize == 0) && (max_nfiles == 0)) return 0; // allow cache to grow indefinitely! dangerous!!
        int n = 0;
        if (((max_cachesize > 0) && (cachesize >= max_cachesize))
            || ((max_nfiles > 0) && (nfiles >= max_nfiles))) {
            vector<AListEle> alist;

            locking.lock();
            for (const auto &ele : cachetable) {
                AListEle al = { ele.first, ele.second.access_time, ele.second.fsize };
                alist.push_back(al);
            }
            sort(alist.begin(), alist.end(), _compare_access_time_asc);

            long long cachesize_goal = max_cachesize*cache_hysteresis;
            int nfiles_goal = max_nfiles*cache_hysteresis;
            for (const auto& ele : alist) {
                logger->debug("Purging from cache '") << cachetable[ele.canonical].cachepath << "'...";
                string delpath = _cachedir + "/" + cachetable[ele.canonical].cachepath;
                ::remove(delpath.c_str());
                cachetable.erase(ele.canonical);
                cachesize -= cachetable[ele.canonical].fsize;
                --nfiles;
                ++n;
                if ((max_cachesize > 0) && (cachesize < cachesize_goal)) break;
                if ((max_nfiles > 0) && (nfiles < nfiles_goal)) break;
            }
            locking.unlock();
        }
        return n;
    }
    //============================================================================

    std::string SipiCache::check(const string &origpath_p, const string &canonical_p)
    {
        struct stat fileinfo;
        SipiCache::CacheRecord fr;

        if (stat(origpath_p.c_str(), &fileinfo) != 0) {
            throw SipiError(__file__, __LINE__, "Couldn't stat file \"" + origpath_p + "\"!", errno);
        }
#if defined(HAVE_ST_ATIMESPEC)
        struct timespec mtime = fileinfo.st_mtimespec;
#else
        time_t mtime = fileinfo.st_mtime;
#endif

        string res;
        locking.lock();
        try {
            fr = cachetable.at(canonical_p);
        }
        catch(const std::out_of_range& oor) {
            locking.unlock();
            return res; // return empty string, because we didn't find the file in cache
        }

        //
        // get the current time (seconds since Epoch)
        //
        time_t at;
        time(&at);
        fr.access_time = at; // update the access time!
        locking.unlock();

        if (tcompare(mtime, fr.mtime) > 0) { // original file is newer than cache, we have to replace it..
            return res; // return empty string, means "replace the file in the cache!"
        }
        else {
            return _cachedir + "/" + fr.cachepath;
        }
    }
    //============================================================================

    string SipiCache::getNewCacheName(void)
    {
        //
        // create a unique temporary filename
        //
        string tmpname = _cachedir + "/cache_XXXXXXXXXX";
        char *writable = new char[tmpname.size() + 1];
        std::copy(tmpname.begin(), tmpname.end(), writable);
        writable[tmpname.size()] = '\0'; // don't forget the terminating 0

        if (mktemp(writable) == NULL) {
            throw SipiError(__file__, __LINE__, string("Couldn't create temporary file \"") + string(writable) + string("\"!"), errno);
        }
        tmpname = string(writable);
        delete[] writable;

        return tmpname;
    }
    //============================================================================

    void SipiCache::add(const string &origpath_p, const string &canonical_p, const string &cachepath_p)
    {
        size_t pos = cachepath_p.rfind('/');
        string cachepath;
        if (pos != string::npos) {
            cachepath = cachepath_p.substr(pos + 1);
        }
        else {
            cachepath = cachepath_p;
        }
        struct stat fileinfo;
        SipiCache::CacheRecord fr;

        fr.origpath = origpath_p;
        fr.cachepath = cachepath;

        locking.lock();
        if (stat(cachepath_p.c_str(), &fileinfo) != 0) {
            locking.unlock();
            throw SipiError(__file__, __LINE__, "Couldn't stat file \"" + origpath_p + "\"!", errno);
        }
#if defined(HAVE_ST_ATIMESPEC)
        fr.mtime = fileinfo.st_mtimespec;
#else
        fr.mtime = fileinfo.st_mtime;
#endif

        //
        // get the current time (seconds since Epoch)
        //
        time_t at;
        time(&at);
        fr.access_time = at;
        fr.fsize = fileinfo.st_size;

        //
        // we check if there is already a file with the same canonical name. If so,
        // we remove it
        //
        try {
            SipiCache::CacheRecord tmp_fr = cachetable.at(canonical_p);
            string toremove = _cachedir + "/" + tmp_fr.cachepath;
            ::remove(toremove.c_str());
            cachesize -= tmp_fr.fsize;
            --nfiles;
        }
        catch(const std::out_of_range& oor) {
            // do nothing...
        }

        purge();

        cachetable[canonical_p] = fr;
        cachesize += fr.fsize;
        ++nfiles;

        locking.unlock();
    }
    //============================================================================

    bool SipiCache::remove(const std::string &canonical_p) {
        auto logger = spdlog::get(shttps::loggername);
        SipiCache::CacheRecord fr;

        locking.lock();
        try {
            fr = cachetable.at(canonical_p);
        }
        catch(const std::out_of_range& oor) {
            locking.unlock();
            return false; // return empty string, because we didn't find the file in cache
        }

        logger->debug("Delete from cache '") << cachetable[canonical_p].cachepath << "'...";
        string delpath = _cachedir + "/" + cachetable[canonical_p].cachepath;
        ::remove(delpath.c_str());
        cachesize -= cachetable[canonical_p].fsize;
        cachetable.erase(canonical_p);
        --nfiles;
        locking.unlock();

        return true;
    }
    //============================================================================

    void SipiCache::loop(ProcessOneCacheFile worker, void *userdata, SortMethod sm) {
        vector<AListEle> alist;

        for (const auto &ele : cachetable) {
            AListEle al = { ele.first, ele.second.access_time, ele.second.fsize };
            alist.push_back(al);
        }

        switch (sm) {
            case SORT_ATIME_ASC: {
                sort(alist.begin(), alist.end(), _compare_access_time_asc);
                break;
            }
            case SORT_ATIME_DESC: {
                sort(alist.begin(), alist.end(), _compare_access_time_desc);
                break;
            }
            case SORT_FSIZE_ASC: {
                sort(alist.begin(), alist.end(), _compare_fsize_asc);
                break;
            }
            case SORT_FSIZE_DESC: {
                sort(alist.begin(), alist.end(), _compare_fsize_desc);
                break;
            }
        }

        int i = 1;
        for (const auto& ele : alist) {
            worker(i, ele.canonical, cachetable[ele.canonical], userdata);
            i++;
        }
    }
    //============================================================================

}
