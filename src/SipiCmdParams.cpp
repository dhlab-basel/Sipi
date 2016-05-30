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

#include <string>
#include <iostream>

#include "SipiError.h"
#include "SipiCmdParams.h"

using namespace std;
//using namespace Magick;

namespace Sipi {

    SipiCmdParams::SipiCmdParams (int argc, char *argv[], const char *infostr) {
        if (infostr != NULL) {
            info = infostr;
        }
        SipiCmdParams::argv.resize (argc);
        for (int i = 0; i < argc; i++) {
            SipiCmdParams::argv[i] = argv[i];
        }
    }
    //============================================================================


    SipiCmdParams::~SipiCmdParams (void) {
        map<const string, SipiParam *, ltstr>::iterator indx = params.begin();
        while (indx != params.end()) {
            delete params[(*indx).first];
            indx++;
        }
    }
    //============================================================================

    void SipiCmdParams::print_usage(void) {
        map<const string, SipiParam *, ltstr>::iterator indx = params.begin();
        cout << endl << "usage: " << argv[0] << " ";
        while (indx != params.end()) {
            cout << " -" << params[(*indx).first]->getName ();
            for (int i = 0; i < params[(*indx).first]->numOfValues (); i++) {
                switch ((*(params[(*indx).first]))[i].getType()) {
                    case SipiInteger: {
                        cout << " i";
                        break;
                    }
                    case SipiFloat: {
                        cout << " f";
                        break;
                    }
                    case SipiString: {
                        cout << " s";
                        break;
                    }
                    default: {
                    }
                }
                if (params[(*indx).first]->numOfValues () > 1) {
                    cout << "_" << i;
                }
            }
            indx++;
        }
        cout << endl;
        cout << "Description: " << info << endl;
        cout << "Parameters:" << endl;
        indx = params.begin();
        while (indx !=  params.end()) {
            cout << *(params[(*indx).first]) << endl;
            indx++;
        }
    }
    //============================================================================


    void SipiCmdParams::parseArgv (int flags) {
        //if ((flags & withRoi) == withRoi) addParam (new SipiParam ("roi", "Region of interest", 0, 65535, 4, 0, 0, 65535, 65535));
        //if ((flags & withThreads) == withThreads) addParam (new SipiParam ("nthreads", "Number of parallel threads", 0, 16, 1, 1));
        if (argv.size() < 2) {
            print_usage();
            exit(0);
        }
        vector<string>::iterator iter = argv.begin();
        while (iter != argv.end()) {
            if (*iter == "--help") {
                print_usage();
                exit (0);
            }
            iter++;
        }

        map<const string, SipiParam *, ltstr>::iterator indx = params.begin();
        while (indx != params.end()) {
            SipiParam *p = params[(*indx).first];
            p->parseArgv (argv);
            indx++;
        }
        fnames = argv.begin();
        fnames++;
    }
    //============================================================================

    void SipiCmdParams::addParam (SipiParam *p) {
        params[p->getName()] = p;
    }
    //============================================================================

    string SipiCmdParams::getName (void) {
        if (fnames != argv.end()) {
            return *fnames++;
        }
        else {
            SipiError err (__FILE__, __LINE__, "Not enough cmdline file (input/output) parameters!");
            throw (err);
        }
    }

    SipiParam &SipiCmdParams::operator[] (const string &name) {
        return *(params[name]);
    }
    //============================================================================
}
