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
/*!
 * This file handles the reading and writing of JPEG 2000 files using libtiff.
 */
#ifndef __defined_sipiparam_h
#define __defined_sipiparam_h

#include <iostream>
#include <string>
#include <vector>
#include <cstring>


#include "SipiParamValue.h"

namespace Sipi {
 /*!
  * \class SipiParam
  * \author Lukas Rosenthaler and Peter Fornaro
  * \version 0.1
  *
  * A single command line parameter.
  *
  * The command line parameters have a name (which is used as command line option by prepending a "-" to
  * it. Each command line parameter can have one or more associated values. The values are in the range
  * given by min and max, and for each value a default has to be given.
  * \see SipiParamValue
  */
  class SipiParam {
  private:
    bool fromCmdline;
    std::string name;
    std::string description;
    SipiParamValue min;
    SipiParamValue max;
    std::vector<std::string> options;
    std::vector<SipiParamValue> vals;
  public:
   /*!
    * Default constructor
    */
    SipiParam (void);

   /*!
    * Copy constructor.
    *
    * \param[in] p Where to copy from.
    */
    SipiParam (const SipiParam &p);

   /*!
    * Constructor for simple flag like "-test".
    *
    * \param[in] name_p Name of parameter (without "-"!).
    * \param[in] description_p Description of parameter.
    */
    SipiParam (const char *name_p, const char *description_p);

   /*!
    * Constructor for integer parameter.
    *
    * \param name_p Name of parameter (without "-"!).
    * \param[in] description_p Description of parameter.
    * \param[in] min_p Minimal value accepted. If the value given is less, it will be clapmed to this min-value.
    * \param[in] max_p Maximal vlue accepted. If the vlue given is larger, it will be clamped to this max value.
    * \param[in] n_p Number of values accepted/required.
    * \param[in] ... Default values.
    */
    SipiParam (const char *name_p, const char *description_p, int min_p, int max_p, int n_p, ...);

   /*!
    * Constructor for float parameter.
    *
    * \param name_p Name of parameter (without "-"!).
    * \param description_p Description of parameter.
    * \param min_p Minimal value accepted. If the value given is less, it will be clapmed to this min-value.
    * \param max_p Maximal vlue accepted. If the vlue given is larger, it will be clamped to this max value.
    * \param n_p Number of values accepted/required.
    * \param ... Default values.
    */
    SipiParam (const char *name_p, const char *description_p, float min_p, float max_p, int n_p, ...);

   /*!
    * Constructor for string parameter.
    *
    * \param name_p Name of parameter (without "-"!).
    * \param description_p Description of parameter.
    * \param n_p Number of values accepted\required.
    * \param ... Default values.
    */
    SipiParam (const char *name_p, const char *description_p, int n_p, ...);

   /*!
    * Constructor for selection parameter.
    *
    * \param name_p Name of parameter (without "-"!).
    * \param description_p Description of parameter.
    * \param list_p List of selection optione, separated by ":".
    * \param n_p Number of values accepted/required.
    * \param ... Default values.
    */
    SipiParam (const char *name_p, const char *description_p, const char *list_p, int n_p, ...);

   /*!
    * Destructor
    */
    virtual ~SipiParam (void);

   /*!
    * "=" overload
    *
    * \param p SipiParam instance
    */
    SipiParam &operator= (const SipiParam &p);

   /*!
    * \return The name of the parameter
    */
    inline std::string &getName (void) { return name; };

   /*!
    * Parsing of the command line arguments.
    *
    * This method parses the command line arguments for the specified parameter. All Parameters have the
    * Form "-name val1 val2 ...".
    *
    * \param argv Command line arguments
    */
    void parseArgv (std::vector<std::string> &argv);

   /*!
    * Returns the number of values associated with this parameter
    *
    * \return Number of values
    */
    inline int numOfValues (void) const { return vals.size(); };

   /*!
    * returns true if the parameter has been set on the command line.
    *
    * \return true, if set from command line
    */
    inline bool isSet (void) { return fromCmdline; };

   /*!
    * Extract a value (Instance of SipiParamValue.
    *
    * This overload of the []-operator extracts a parameter value (Instance if SipiParamValue).
    * If the index is invalid a SipiError-exception is thrown.
    *
    * \param index Index of value
    */
    SipiParamValue &operator[] (int index);

   /*!
    * Write parameter as text to output stream
    *
    * \param lhs Output stream
    * \param rhs Reference to SipiParam instance
    * \return Output ostream
    */
    friend std::ostream &operator<< (std::ostream &lhs, SipiParam &rhs);
  };

}

#endif
