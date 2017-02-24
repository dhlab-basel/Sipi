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

/**
 * @file
 */
#ifndef __defined_sipicmdparams_h
#define __defined_sipicmdparams_h

#include <iostream>
#include <string>
#include <map>
//#include <Magick++.h>


namespace Sipi {

    /*!
     * \class SipiCmdParams
     * \author Lukas Rosenthaler and peter Fornaro
     * \version 0.1
     *
     * This class performs the actual parsing of the commandline. First, all parameters and associated
     * values are parsed. A parameter consists of a SipiParam instance. On the commandline, the parameter is
     * given by a leading "-" and the name of the parameter followed by the value(s), each separated by blanks.
     * After processing all parameters, the remaining commandline arguments are considered to be filenames that
     * can be retrieved using the getName member method. Retrieving more filenames than given on the commandline
     * will throw an exception.
     *
     * An Example:
     * \verbatim
     SipiCmdParams params (argc, argv, "An adaptive median filter which uses the local variance for steering.");
     params.addParam (new SipiParam ("size", "Size of neigbourhood", 1, 5, 1, 2));
     params.addParam (new SipiParam ("noise", "Estimated noise level", 0, 65535, 1, 5000));
     params.addParam (new SipiParam ("var", "Method to be used to calculate variance", "classic:diffmed", 1, "classic"));
     params.addParam (new SipiParam ("smooth", "Method to be used for smoothing", "median:mean", 1, "median"));
     params.parseArgv (withRoi | withThreads);
     ...
     SipiImage inimg (infname);
     int nh_size_2 = (params["size"])[0].getValue (nh_size_2);
     float noise = (params["noise"])[0].getValue (noise);
     \endverbatim
     */
    class SipiCmdParams {
    private:
        struct ltstr {
            /**
             * Function used for associative container \<list\>
             */
            bool operator()(const std::string &s1, const std::string &s2) const { return s1 < s2; };
        };

        std::string info;
        std::vector <std::string> argv;
        std::map<const std::string, SipiParam *, ltstr> params;
        std::vector<std::string>::iterator fnames;
        void print_usage(void);

    public:
        /*!
         * Constructor which takes the command line arguments and an additional description
         *
         * \param[in] argc Number of commandline arguments (from main(argc, argv))
         * \param[in] argv Array with commandline arguments
         * \param[in] infostr Brief description of programm (optional)
         */
        SipiCmdParams(int argc, char *argv[], const char *infostr = NULL);

        /*!
         * Destructor
         */
        ~SipiCmdParams(void);

        /*!
         * Parsing the commandline arguments
         * \param[in] flags Add default cmdline arguments (logical "or"
         * of "withRoi" and "withThreads". Default = 0
         */
        void parseArgv(int flags = 0);

        /*!
         * Add a new parameter
         * \param[in] p Pointer to SipiParam object
         */
        void addParam(SipiParam *p);

        /*!
         * Get a file name from the commandline
         * \return String object with the filename
         */
        std::string getName(void);

        /*!
         * Number of remaining file names in commandline
         * \return Number of filename arguments
         */
        inline int numOfNames(void) { return argv.size(); };

        /*!
         * Index operator overload
         * \param[in] name Name of parameter to return
         * \return Reference to ParamValue instance
         */
        SipiParam &operator[](const std::string &name);
    };

}

#endif
