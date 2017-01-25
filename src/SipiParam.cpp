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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
# ifdef  __cplusplus
extern "C"
# endif
    void *alloca (size_t);
#endif

#include <stdarg.h>
#include <iostream>
#include <vector>

#include "SipiError.h"
#include "SipiParamValue.h"
#include "SipiParam.h"

namespace Sipi {

    SipiParam::SipiParam (void) {
        fromCmdline = false;
    }
    //============================================================================


    SipiParam::SipiParam (const SipiParam &p)
        : vals (p.vals.size())
    {
        fromCmdline = p.fromCmdline;
        name = p.name;
        description = p.description;
        min = p.min;
        max = p.max;
        vals = p.vals;
        options = p.options;
    }
    //============================================================================


    SipiParam::SipiParam (const char *name_p, const char *description_p)
        : name (name_p),
    description (description_p)
    {
        fromCmdline = false;
    }
    //============================================================================


    SipiParam::SipiParam (const char *name_p, const char *description_p, int min_p, int max_p, int n_p, ...)
        : name (name_p),
    description (description_p),
    min (min_p),
    max (max_p),
    vals (n_p)
    {
        va_list al;

        fromCmdline = false;
        va_start (al, n_p);
        for (int i = 0; i < n_p; i++) {
            SipiParamValue tmp (va_arg (al, int));
            vals[i] = tmp;
        }
        va_end (al);
    }
    //============================================================================


    SipiParam::SipiParam (const char *name_p, const char *description_p, float min_p, float max_p, int n_p, ...)
        : name (name_p),
    description (description_p),
    min (min_p),
    max (max_p),
    vals (n_p)
    {
        va_list al;

        fromCmdline = false;
        va_start (al, n_p);
        for (int i = 0; i < n_p; i++) {
            SipiParamValue tmp ((float) va_arg (al, double));
            vals[i] = tmp;
        }
        va_end (al);
    }
    //============================================================================


    SipiParam::SipiParam (const char *name_p, const char *description_p, int n_p, ...)
        : name (name_p),
    description (description_p),
    vals (n_p)
    {
        va_list al;

        fromCmdline = false;
        va_start (al, n_p);
        for (int i = 0; i < n_p; i++) {
            SipiParamValue tmp (va_arg (al, char*));
            vals[i] = tmp;
        }
        va_end (al);
    }
    //============================================================================


    SipiParam::SipiParam (const char *name_p, const char *description_p, const char *list_p, int n_p, ...)
        : name (name_p),
    description (description_p),
    vals (n_p)
    {
        va_list al;

        fromCmdline = false;

        char *tmpstr = (char *) alloca (strlen (list_p) + 1);
        strcpy (tmpstr, list_p);
        char *tok = nullptr;
        while ((tok = strsep (&tmpstr, ":")) != nullptr) {
            if (*tok != '\0') {
                std::string stmp = tok;
                options.push_back (stmp);
            }
        }
        va_start (al, n_p);
        for (int i = 0; i < n_p; i++) {
            SipiParamValue tmp (va_arg (al, char*));
            vals[i] = tmp;
        }
        va_end (al);
    }
    //============================================================================


    SipiParam::~SipiParam (void) {
    }
    //============================================================================


    void SipiParam::parseArgv (std::vector<std::string> &argv) {
        if (argv.empty()) return;
        std::string dash = "-";
        std::vector<std::string>::iterator iter = argv.begin();

        while (iter != argv.end()) {
            if ((*iter) == (dash + name)) {
                fromCmdline = true;
                iter = argv.erase (iter);
                for (int j = 0; (j < vals.size()) &&  (iter != argv.end()); j++) {
                    vals[j] = *iter;
                    iter = argv.erase (iter);
                }
            }
            else {
                iter++;
            }
        }

        return;
    }
    //============================================================================


    SipiParam &SipiParam::operator= (const SipiParam &p) {
        fromCmdline = p.fromCmdline;
        name = p.name;
        description = p.description;
        min = p.min;
        max = p.max;
        vals = p.vals;
        options = p.options;

        return *this;
    }
    //============================================================================


    SipiParamValue &SipiParam::operator[] (int index) {
        return vals[index];
    }
    //============================================================================

    std::ostream &operator<< (std::ostream &outstr, SipiParam &rhs) {
        outstr << "  -" << rhs.name << ": " << rhs.description << " (";
        if (rhs.min.isDefined() && rhs.max.isDefined()) {
            outstr  << "Range: [" << rhs.min << " - " << rhs.max << "] ";
        }
        if (rhs.options.size() > 0) {
            outstr << "Options: ";
            std::vector<std::string>::iterator iter = rhs.options.begin();
            while (iter != rhs.options.end()) {
                if (iter != rhs.options.begin()) outstr << "|";
                outstr << "'" << *iter << "'";
                iter++;
            }
            outstr << ", ";
        }
        if (rhs.vals.size() > 1) {
            outstr << "Defaults:";
        }
        else {
            outstr << "Default:";
        }
        for (int i = 0; i < rhs.vals.size(); i++) {
            outstr << " " << rhs.vals[i];
        }
        outstr << ")";

        return outstr;
    }
    //============================================================================

}
