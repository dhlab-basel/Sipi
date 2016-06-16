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

#ifndef __defined_sipi_cache_h
#define __defined_sipi_cache_h

#include <ctime>
#include <map>
#include <mutex>
#include <sys/time.h>

#include "SipiConfig.h"

namespace Sipi {

    class SipiCache {
    public:
        typedef struct {
            char canonical[256];
            char origpath[256];
            char cachepath[256];
#if defined(HAVE_ST_ATIMESPEC)
            struct timespec mtime; //!< entry time into cache
#else
            time_t mtime;
#endif
            time_t access_time;     //!< last access in seconds
            off_t fsize;
        } FileCacheRecord;

        typedef struct {
            std::string origpath;
            std::string cachepath;
#if defined(HAVE_ST_ATIMESPEC)
            struct timespec mtime; //!< entry time into cache
#else
            time_t mtime;
#endif
            time_t access_time;     //!< last access in seconds
            off_t fsize;
        } CacheRecord;

    private:
        std::mutex locking;
        std::string _cachedir;
        std::map<std::string,CacheRecord> cachetable;
        unsigned long long cachesize;
        unsigned long long max_cachesize;
        unsigned nfiles;
        unsigned max_nfiles;
        float cache_hysteresis;
    public:
        SipiCache(const std::string &cachedir_p, long long max_cachesize_p = 0, unsigned max_nfiles_p = 0, float cache_hysteresis_p = 0.1);
        ~SipiCache();
#if defined(HAVE_ST_ATIMESPEC)
        int tcompare(struct timespec &t1, struct timespec &t2);
#else
        int tcompare(time_t &t1, time_t &t2);
#endif
        int purge();
        std::string check(const std::string &origpath_p, const std::string &canonical_p);
        std::string getNewCacheName(void);
        void add(const std::string &origpath_p, const std::string &canonical_p, const std::string &cachepath_p);
        inline unsigned long long getCachesize(void) { return cachesize; }
        inline unsigned long long getMaxCachesize(void) { return max_cachesize; }
        inline unsigned getNfiles(void) { return nfiles; }
        inline unsigned getMaxNfiles(void) { return max_nfiles; }
    };
}

#endif
