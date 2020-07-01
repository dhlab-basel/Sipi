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
#include <memory>


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
#include <syslog.h>


#include "SipiCache.h"
#include "shttps/Global.h"
#include "SipiError.h"

static const char __file__[] = __FILE__;

namespace Sipi {

    typedef struct _AListEle {
        std::string canonical;
        time_t access_time;
        off_t fsize;

        bool operator<(const _AListEle &str) const {
            return (difftime(access_time, str.access_time) < 0.);
        }

        bool operator>(const _AListEle &str) const {
            return (difftime(access_time, str.access_time) < 0.);
        }

        bool operator==(const _AListEle &str) const {
            return (difftime(access_time, str.access_time) == 0.);
        }
    } AListEle;


    SipiCache::SipiCache(const std::string &cachedir_p, long long max_cachesize_p, unsigned max_nfiles_p,
                         float cache_hysteresis_p) : _cachedir(cachedir_p), max_cachesize(max_cachesize_p),
                                                     max_nfiles(max_nfiles_p), cache_hysteresis(cache_hysteresis_p) {

        if (access(_cachedir.c_str(), R_OK | W_OK | X_OK) != 0) {
            throw SipiError(__file__, __LINE__, "Cache directory not available", errno);
        }

        std::string cachefilename = _cachedir + "/.sipicache";
        cachesize = 0;
        nfiles = 0;

        syslog(LOG_INFO, "Cache at \"%s\" cachesize=%lld nfiles=%d hysteresis=%f", _cachedir.c_str(), max_cachesize,
               max_nfiles, cache_hysteresis);
        std::ifstream cachefile(cachefilename, std::ofstream::in | std::ofstream::binary);

        struct dirent **namelist;
        int n;

        if (!cachefile.fail()) {
            cachefile.seekg(0, cachefile.end);
            std::streampos length = cachefile.tellg();
            cachefile.seekg(0, cachefile.beg);
            int n = length / sizeof(SipiCache::FileCacheRecord);
            syslog(LOG_INFO, "Reading cache file...");

            for (int i = 0; i < n; i++) {
                SipiCache::FileCacheRecord fr;
                cachefile.read((char *) &fr, sizeof(SipiCache::FileCacheRecord));
                std::string accesspath = _cachedir + "/" + fr.cachepath;

                if (access(accesspath.c_str(), R_OK) != 0) {
                    //
                    // we cannot find the file – probably it has been deleted => skip it
                    //
                    syslog(LOG_DEBUG, "Cache could'nt find file \"%s\" on disk!", fr.cachepath);
                    continue;
                }

                CacheRecord cr;
                cr.img_w = fr.img_w;
                cr.img_h = fr.img_h;
                cr.tile_w = fr.img_w;
                cr.tile_h = fr.tile_h;
                cr.clevels = fr.clevels;
                cr.numpages = fr.numpages;
                cr.origpath = fr.origpath;
                cr.cachepath = fr.cachepath;
                cr.mtime = fr.mtime;
                cr.access_time = fr.access_time;
                cr.fsize = fr.fsize;
                cachesize += fr.fsize;
                nfiles++;
                cachetable[fr.canonical] = cr;
                syslog(LOG_INFO, "File \"%s\" adding to cache", cr.cachepath.c_str());
            }
        }

        //
        // now we looking for files that are not in the list of cached files
        // and we delete them
        //

        n = scandir(_cachedir.c_str(), &namelist, nullptr, alphasort);

        if (n < 0) {
            perror("scandir");
        } else {
            while (n--) {
                if (namelist[n]->d_name[0] == '.') continue; // files beginning with "." are not removed
                std::string file_on_disk = namelist[n]->d_name;
                bool found = false;

                for (const auto &ele : cachetable) {
                    if (ele.second.cachepath == file_on_disk) found = true;
                }

                if (!found) {
                    std::string ff = _cachedir + "/" + file_on_disk;
                    syslog(LOG_INFO, "File \"%s\" not in cache file! Deleting...", file_on_disk.c_str());
                    remove(ff.c_str());
                }

                free(namelist[n]);
            }

            free(namelist);
        }

        for (const auto &ele : cachetable) {
            try {
                (void) sizetable.at(ele.second.origpath);
            } catch (const std::out_of_range &oor) {
                SipiCache::SizeRecord tmp_cr = {ele.second.img_w, ele.second.img_h, ele.second.tile_w, ele.second.tile_h, ele.second.clevels,  ele.second.numpages, ele.second.mtime};
                sizetable[ele.second.origpath] = tmp_cr;
            }
        }
    }
    //============================================================================

    SipiCache::~SipiCache() {
        syslog(LOG_DEBUG, "Closing cache...");
        std::string cachefilename = _cachedir + "/.sipicache";
        std::ofstream cachefile(cachefilename, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);

        if (!cachefile.fail()) {
            for (const auto &ele : cachetable) {
                SipiCache::FileCacheRecord fr;
                fr.img_w = ele.second.img_w;
                fr.img_h = ele.second.img_h;
                fr.tile_w = ele.second.tile_w;
                fr.tile_h = ele.second.tile_h;
                fr.clevels = ele.second.clevels;
                fr.numpages = ele.second.numpages;
                (void) snprintf(fr.canonical, 256, "%s", ele.first.c_str());
                (void) snprintf(fr.origpath, 256, "%s", ele.second.origpath.c_str());
                (void) snprintf(fr.cachepath, 256, "%s", ele.second.cachepath.c_str());
                fr.mtime = ele.second.mtime;
                fr.fsize = ele.second.fsize;
                fr.access_time = ele.second.access_time;
                cachefile.write((char *) &fr, sizeof(SipiCache::FileCacheRecord));
                syslog(LOG_DEBUG, "Writing \"%s\" to cache file...", ele.second.cachepath.c_str());
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
        } else if (t1.tv_sec < t2.tv_sec) {
            return -1;
        } else {
            if (t1.tv_sec == t2.tv_sec) {
                if (t1.tv_nsec > t2.tv_nsec) {
                    return 1;
                } else if (t1.tv_nsec < t2.tv_nsec) {
                    return -1;
                } else {
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

    static bool _compare_access_time_asc(const AListEle &e1, const AListEle &e2) {
        double d = difftime(e1.access_time, e2.access_time);
        return (d < 0.0);
    }
    //============================================================================

    static bool _compare_access_time_desc(const AListEle &e1, const AListEle &e2) {
        double d = difftime(e1.access_time, e2.access_time);
        return (d > 0.0);
    }
    //============================================================================

    static bool _compare_fsize_asc(const AListEle &e1, const AListEle &e2) {
        return (e1.fsize < e2.fsize);
    }
    //============================================================================

    static bool _compare_fsize_desc(const AListEle &e1, const AListEle &e2) {
        return (e1.fsize > e2.fsize);
    }
    //============================================================================

    int SipiCache::purge(bool use_lock) {
        if ((max_cachesize == 0) && (max_nfiles == 0)) return 0; // allow cache to grow indefinitely! dangerous!!
        int n = 0;

        if (((max_cachesize > 0) && (cachesize >= max_cachesize)) || ((max_nfiles > 0) && (nfiles >= max_nfiles))) {
            std::vector<AListEle> alist;
            std::unique_lock<std::mutex> locking_mutex_guard(locking, std::defer_lock);
            if (use_lock) locking_mutex_guard.lock();

            for (const auto &ele : cachetable) {
                AListEle al = {ele.first, ele.second.access_time, ele.second.fsize};
                alist.push_back(al);
            }

            sort(alist.begin(), alist.end(), _compare_access_time_asc);

            long long cachesize_goal = max_cachesize * cache_hysteresis;
            int nfiles_goal = max_nfiles * cache_hysteresis;

            for (const auto &ele : alist) {
                syslog(LOG_DEBUG, "Purging from cache \"%s\"...", cachetable[ele.canonical].cachepath.c_str());
                std::string delpath = _cachedir + "/" + cachetable[ele.canonical].cachepath;
                ::remove(delpath.c_str());
                cachesize -= cachetable[ele.canonical].fsize;
                --nfiles;
                ++n;
                (void) cachetable.erase(ele.canonical);
                if ((max_cachesize > 0) && (cachesize < cachesize_goal)) break;
                if ((max_nfiles > 0) && (nfiles < nfiles_goal)) break;
            }
        }

        return n;
    }
    //============================================================================

    std::string SipiCache::check(const std::string &origpath_p, const std::string &canonical_p) {
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

        std::string res;

        {
            std::lock_guard<std::mutex> locking_mutex_guard(locking);

            try {
                fr = cachetable.at(canonical_p);
            } catch (const std::out_of_range &oor) {
                return res; // return empty string, because we didn't find the file in cache
            }

            //
            // get the current time (seconds since Epoch)
            //
            time_t at;
            time(&at);
            fr.access_time = at; // update the access time!
        }

        if (tcompare(mtime, fr.mtime) > 0) { // original file is newer than cache, we have to replace it..
            return res; // return empty string, means "replace the file in the cache!"
        } else {
            return _cachedir + "/" + fr.cachepath;
        }
    }
    //============================================================================

    /*!
     * Creates a new cache file with a unique name.
     *
     * \return the name of the file.
     */
    std::string SipiCache::getNewCacheFileName(void) {
        std::string filename = _cachedir + "/cache_XXXXXXXXXX";
        char *c_filename = &filename[0];
        int tmp_fd = mkstemp(c_filename);

        if (tmp_fd == -1) {
            throw SipiError(__file__, __LINE__, std::string("Couldn't create cache file ") + filename, errno);
        }

        close(tmp_fd);
        return filename;
    }
    //============================================================================

    void SipiCache::add(
            const std::string &origpath_p,
            const std::string &canonical_p,
            const std::string &cachepath_p,
            size_t img_w_p,
            size_t img_h_p,
            size_t tile_w_p,
            size_t tile_h_p,
            int clevels_p,
            int numpages_p) {
        size_t pos = cachepath_p.rfind('/');
        std::string cachepath;

        if (pos != std::string::npos) {
            cachepath = cachepath_p.substr(pos + 1);
        } else {
            cachepath = cachepath_p;
        }

        struct stat fileinfo;
        SipiCache::CacheRecord fr;
        SipiCache::SizeRecord sr;

        fr.img_w = sr.img_w = img_w_p;
        fr.img_h = sr.img_h = img_h_p;
        fr.tile_w = sr.tile_w = tile_w_p;
        fr.tile_h = sr.tile_h = tile_h_p;
        fr.clevels = sr.clevels = clevels_p;
        fr.numpages = sr.numpages = numpages_p;
        fr.origpath = origpath_p;
        fr.cachepath = cachepath;

        std::lock_guard<std::mutex> locking_mutex_guard(locking);

        if (stat(cachepath_p.c_str(), &fileinfo) != 0) {
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
            std::string toremove = _cachedir + "/" + tmp_fr.cachepath;
            ::remove(toremove.c_str());
            cachesize -= tmp_fr.fsize;
            --nfiles;
        } catch (const std::out_of_range &oor) {
            // do nothing...
        }

        purge(false);

        cachetable[canonical_p] = fr;
        cachesize += fr.fsize;

        /*
        try {
            (void) sizetable.at(origpath_p);
        }
        catch(const std::out_of_range& oor) {
         */
        SipiCache::SizeRecord tmp_cr = {img_w_p, img_h_p, tile_w_p, tile_h_p, clevels_p, numpages_p};
        sizetable[origpath_p] = tmp_cr;
        //}

        ++nfiles;
    }
    //============================================================================

    bool SipiCache::remove(const std::string &canonical_p) {
        SipiCache::CacheRecord fr;
        std::lock_guard<std::mutex> locking_mutex_guard(locking);

        try {
            fr = cachetable.at(canonical_p);
        } catch (const std::out_of_range &oor) {
            return false; // return empty string, because we didn't find the file in cache
        }

        syslog(LOG_DEBUG, "Delete from cache \"%s\"...", cachetable[canonical_p].cachepath.c_str());
        std::string delpath = _cachedir + "/" + cachetable[canonical_p].cachepath;
        ::remove(delpath.c_str());
        cachesize -= cachetable[canonical_p].fsize;
        cachetable.erase(canonical_p);
        --nfiles;

        return true;
    }
    //============================================================================

    void SipiCache::loop(ProcessOneCacheFile worker, void *userdata, SortMethod sm) {
        std::vector<AListEle> alist;

        for (const auto &ele : cachetable) {
            AListEle al = {ele.first, ele.second.access_time, ele.second.fsize};
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

        for (const auto &ele : alist) {
            worker(i, ele.canonical, cachetable[ele.canonical], userdata);
            i++;
        }
    }
    //============================================================================

    bool SipiCache::getSize(
            const std::string &origname_p,
            size_t &img_w,
            size_t &img_h,
            size_t &tile_w,
            size_t &tile_h,
            int &clevels,
            int &numpages) {
        struct stat fileinfo;
        if (stat(origname_p.c_str(), &fileinfo) != 0) {
            throw SipiError(__file__, __LINE__, "Couldn't stat file \"" + origname_p + "\"!", errno);
        }
#if defined(HAVE_ST_ATIMESPEC)
        struct timespec mtime = fileinfo.st_mtimespec;
#else
        time_t mtime = fileinfo.st_mtime;
#endif

        try {
            SipiCache::SizeRecord sr = sizetable.at(origname_p);
            if (tcompare(mtime, sr.mtime) > 0) { // original file is newer than cache, we have to replace it..
                std::lock_guard<std::mutex> locking_mutex_guard(locking);

                sizetable.erase(origname_p);
                return false; // means "replace the file in the cache"
            }

            img_w = sr.img_w;
            img_h = sr.img_h;
            tile_w = sr.tile_w;
            tile_h = sr.tile_h;
            clevels = sr.clevels;
            numpages = sr.numpages;
        } catch (const std::out_of_range &oor) {
            return false;
        }

        return true;
    }
    //============================================================================
}
