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
 * This file generic cmdline parameter values
 */
#ifndef __defined_sipiparamvalue_h
#define __defined_sipiparamvalue_h

#include <iostream>

/*! Integer constant initialized to 1 */
const int SipiIntType = 1;

/*! Float constant initialized to 1.0 */
const float SipiFloatType = 1.0;

/*! C++ string constant initialized to "*" */
const std::string SipiStringType = "*";


namespace Sipi {
 /*!
  * \typedef DataType_t
  *
  * Valid data types for sipi parameter values
  */
  typedef  enum {
    SipiUndefined,
    SipiInteger,
    SipiFloat,
    SipiString
  } DataType_t;

 /*!
  * \class SipiParamValue
  * \author Lukas Rosenthaler and Peter Fornaro
  * \version 0.1
  *
  * Class to hold a parameter value which supports integer, float and string datatypes
  */
  class SipiParamValue {
  private:
    DataType_t dataType;
    union {
      void *val;
      int ival;
      float fval;
      std::string *sval;
    };
    inline int round (const float f) { return (f >= 0.0) ? ((int) (f + 0.5)) : ((int) (f - 0.5)); }
  public:

   /*!
    * Default constructor.
    */
    SipiParamValue (void);

   /*!
    * Copy constructor (by reference).
    * \param[in] p Reference of SipiParamValue instance
    */
    SipiParamValue (const SipiParamValue &p);

   /*!
    * Copy constructor (by pointer).
    * \param[in] p Reference of SipiParamValue instance
    */
    SipiParamValue (const SipiParamValue *p);

   /*!
    * Constructor from integer.
    * \param[in] ival Integer value
    */
    SipiParamValue (const int ival);

   /*!
    * Constructor from float.
    * \param[in] fval Float value
    */
    SipiParamValue (const float fval);

   /*!
    * Constructor from C-string.
    * \param[in] str C-string
    */
    SipiParamValue (const char *str);

   /*!
    * Constructor from C++-string.
    * \param[in] str C++ string reference
    */
    SipiParamValue (const std::string &str);

   /*!
    * Destructor which free's all dynamically allocated memory.
    */
    virtual ~SipiParamValue (void);

    //
    // operator overloads
    //

   /*!
    * Overload of "=".
    * \param[in] rhs Reference to SipiParamValue instance
    * \return SipiParamValue instance
    */
    SipiParamValue &operator= (const SipiParamValue &rhs);

   /*!
    * Overload of "=".
    * \param[in] rhs Integer value
    * \return SipiParamValue instance
    */
    SipiParamValue &operator= (const int rhs);

   /*!
    * Overload of "=".
    * \param[in] rhs Float value
    * \return SipiParamValue instance
    */
    SipiParamValue &operator= (const float rhs);

   /*!
    * Overload of "=".
    * \param[in] rhs C-string
    * \return SipiParamValue instance
    */
    SipiParamValue &operator= (const char *rhs);

   /*!
    * Overload of "=".
    * \param[in] rhs Reference of C++ string
    * \return SipiParamValue instance
    */
    SipiParamValue &operator= (const std::string &rhs);

   /*!
    * Test, if SipiParamValue instance has a valid value
    * \return Boolean, if a valid value exists
    */
    inline bool isDefined (void) const { return (dataType != SipiUndefined); }

   /*!
    * Data type getter.
    * \return Datatype, either SipiParamValue::SipiInteger, SipiParamValue::SipiFloat or SipiParamValue::SipiString
    */
    inline int getType (void) const { return dataType; }

   /*!
    * Value getter for integer values.
    *
    * \param[in] dummy Used to determine the data type. Value is ignored!
    * \return value as integer
    */
    int getValue (const int dummy);

   /*!
    * Value getter for float values.
    *
    * \param[in] dummy Used to determine the data type. Value is ignored!
    * \return value as float
    */
    float getValue (const float dummy);

   /*!
    * Value getter for C++ string values.
    *
    * \param[in] dummy Used to determine the data type. Content is ignored!
    * \return value as C++ string
    */
    std::string getValue (const std::string &dummy);

   /*!
    * Output operator for iostream.
    *
    * This methods writes the formatted, commented content of the instance
    * to the output stream. This output is formatted for the human reader and
    * not intended for machine reading!
    *
    * \param[in] lhs Output stream
    * \param[in] rhs Reference to SipiParamValue instance
    * \return std:ostream object
    */
    friend std::ostream &operator<< (std::ostream &lhs, const SipiParamValue &rhs);
  };

} // namespace

#endif
