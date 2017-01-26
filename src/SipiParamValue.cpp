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

#include <iostream>
#include <sstream>
#include <cerrno>


#include "SipiParamValue.h"
#include "SipiError.h"

using namespace std;


namespace Sipi {

  //
  // constructors
  //
  SipiParamValue::SipiParamValue (void) {
    dataType = SipiUndefined;
    val = nullptr;
  }
  //============================================================================


  SipiParamValue::SipiParamValue (const SipiParamValue &p) {
    dataType = p.dataType;
    if (dataType == SipiString) {
      sval = new std::string (*(p.sval));
    }
    else {
      val = p.val;
    }
  }
  //============================================================================


  SipiParamValue::SipiParamValue (const SipiParamValue *p) {
    dataType = p->dataType;
    if (dataType == SipiString) {
      sval = new std::string (*(p->sval));
    }
    else {
      val = p->val;
    }
  }
  //============================================================================


  SipiParamValue::SipiParamValue (const int i) {
    dataType = SipiInteger;
    ival = i;
  }

  SipiParamValue::SipiParamValue (const float f) {
    dataType = SipiFloat;
    fval = f;
  }
  //============================================================================

  SipiParamValue::SipiParamValue (const char *str) {
    dataType = SipiString;
    sval = new std::string (str);
  }
  //============================================================================

  SipiParamValue::SipiParamValue (const string &s) {
    dataType = SipiString;
    sval = new std::string (s);
  }
  //============================================================================

  SipiParamValue::~SipiParamValue (void) {
    if (dataType == SipiString) {
      delete sval;
    }
    val = nullptr;
  }
  //============================================================================


  SipiParamValue &SipiParamValue::operator= (const SipiParamValue &rhs) {
    dataType = rhs.dataType;
    if (dataType == SipiString) {
      sval = new std::string (*(rhs.sval));
    }
    else {
      val = rhs.val;
    }
    return *this;
  }
  //============================================================================

  //
  // operator overloads
  //
  SipiParamValue &SipiParamValue::operator= (const int rhs) {
    switch (dataType) {
      case SipiInteger: {
	ival = rhs;
	break;
      }
      case SipiFloat: {
	fval = (float) rhs;
	break;
      }
      case SipiString: {
	std::ostringstream ostr;
	ostr << rhs;
	sval = new std::string (ostr.str());
	break;
      }
      default: {
	dataType = SipiInteger;
	ival = rhs;
      }
    }
    return *this;
  }
  //============================================================================


  SipiParamValue &SipiParamValue::operator= (const float rhs) {
    switch (dataType) {
      case SipiInteger: {
	ival = round (rhs);
	break;
      }
      case SipiFloat: {
	fval = rhs;
	break;
      }
      case SipiString: {
	std::ostringstream ostr;
	ostr << rhs;
	sval = new std::string (ostr.str());
	break;
      }
      default: {
	dataType = SipiFloat;
	fval = rhs;
      }
    }
    return *this;
  }
  //============================================================================


  SipiParamValue &SipiParamValue::operator= (const char *rhs) {
    std::istringstream instr (rhs);
    switch (dataType) {
      case SipiInteger: {
	instr >> ival;
	break;
      }
      case SipiFloat: {
	instr >> fval;
	break;
      }
      case SipiString: {
	delete sval;
	sval = new std::string (rhs);
	break;
      }
      default: {
	dataType = SipiString;
	sval = new std::string (rhs);
      }
    }
    return *this;
  }
  //============================================================================


 SipiParamValue & SipiParamValue::operator= (const std::string &rhs) {
    std::istringstream instr (rhs);
    switch (dataType) {
      case SipiInteger: {
	instr >> ival;
	break;
      }
      case SipiFloat: {
	instr >> fval;
	break;
      }
      case SipiString: {
	delete sval;
	sval = new std::string (rhs);
	break;
      }
      default: {
	dataType = SipiString;
	sval = new std::string (rhs);
      }
    }
    return *this;
  }
  //============================================================================


  int SipiParamValue::getValue (const int dummy) {
    switch (dataType) {
      case SipiInteger: {
	return ival;
      }
      case SipiFloat: {
	return (round (fval));
      }
      case SipiString: {
	int itmp = (int) strtol((*sval).c_str(), (char **) nullptr, 10);
	if (errno == EINVAL) {
	  SipiError sipi_error (__FILE__, __LINE__,
				"invalid datatype: conversion \"SipiString\" to  \"SipiInteger\" impossible!");
	  throw (sipi_error);
	}
	return itmp;
      }
      default: {
	SipiError sipi_error (__FILE__, __LINE__,  "Datatype is \"SipiUndefined\"!");
	throw (sipi_error);
      }
    } // switch
  }
  //============================================================================

  float SipiParamValue::getValue (const float dummy) {
    switch (dataType) {
      case SipiInteger: {
	return ((float) ival);
      }
      case SipiFloat: {
	return fval;
      }
      case SipiString: {
	SipiError sipi_error (__FILE__, __LINE__,  "Datatype is \"SipiString\": cannot convert to float!");
	throw (sipi_error);
      }
      default: {
	SipiError sipi_error (__FILE__, __LINE__,  "Datatype is \"SipiUndefined\"!");
	throw (sipi_error);
      }
    }
  }
  //============================================================================

  string SipiParamValue::getValue (const string &dummy) {
    switch (dataType) {
      case SipiInteger: {
	std::ostringstream ostr;
	ostr << ival;
	return ostr.str();
      }
      case SipiFloat: {
	std::ostringstream ostr;
	ostr << fval;
	return ostr.str();
      }
      case SipiString: {
	return *sval;
      }
      default: {
	SipiError sipi_error (__FILE__, __LINE__,  "invalid datatype:  \"SipiUndefined\"!");
	throw (sipi_error);
      }
    }
  }
  //============================================================================


  std::ostream &operator<< (std::ostream &outstr, const SipiParamValue &rhs) {
    switch (rhs.dataType) {
      case SipiInteger:
	outstr << rhs.ival;
	break;
      case SipiFloat:
	outstr << rhs.fval;
	break;
      case  SipiString:
	outstr << "'" << *(rhs.sval) << "'";
	break;
      default:
	outstr << "SipiParamValue: undefined value!";
    }
    return outstr;
  }
  //============================================================================

}
