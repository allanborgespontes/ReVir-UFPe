/// ----------------------------------------*- mode: C++; -*--
/// @file configpar.h
/// Configuration parameter class
///
/// This class allows to globally register configuration parameters
/// of arbirtrary types for different programm components in so
/// called realms. Every configuration parameter can be accessed
/// by its realm id, its own parameter id and knowledge of its 
/// type.
/// ----------------------------------------------------------
/// $Id: configpar.h 4210 2009-08-06 11:22:29Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/include/configpar.h $
// ===========================================================
//                      
// Copyright (C) 2008, all rights reserved by
// - Institute of Telematics, Universitaet Karlsruhe (TH)
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ===========================================================
#ifndef _CONFIGPAR__H
#define _CONFIGPAR__H

#include "protlib_types.h"
#include "address.h"
#include <list>
#include <string>
#include <iostream>
#include <sstream>

namespace protlib {
  using namespace std;


typedef uint8 realm_id_t;      // note that 0 is a reserved value and means undefined/unset
typedef uint16 configpar_id_t; // note that 0 is a reserved value and means undefined/unset

/** Exceptions **/

class configParException : public ProtLibException {
public:
  configParException(const std::string& msg) : ProtLibException(msg) {};
};


class configParExceptionTypeMismatch : public configParException {
public:
  configParExceptionTypeMismatch(configpar_id_t configparid) : configParException("Type mismatch for a configuration parameter - this is a programming error"), configparid(configparid) {};
  virtual const char *what() const throw() { ostringstream more_detailed_error; more_detailed_error << error_msg << ", parid=" << configparid;
    return more_detailed_error.str().c_str(); }
private:
  configpar_id_t configparid;
};


class configParExceptionParIDInvalid : public configParException {
public:
  configParExceptionParIDInvalid(configpar_id_t configparid) : configParException("illegal parameter id - this is a programming error"), configparid(configparid) {};
  virtual const char *what() const throw() { ostringstream more_detailed_error; more_detailed_error << error_msg << ", parid=" << configparid;
    return more_detailed_error.str().c_str(); }
private:
  configpar_id_t configparid;
};


class configParExceptionParAlreadyRegistered : public configParException {
public:
  configParExceptionParAlreadyRegistered(configpar_id_t configparid) : configParException("tried to register parameter twice - this is a programming error"), configparid(configparid) {};
  virtual const char *what() const throw() { ostringstream more_detailed_error; more_detailed_error << error_msg << ", parid=" << configparid;
    return more_detailed_error.str().c_str(); }
private:
  configpar_id_t configparid;
};


class configParExceptionParNotRegistered : public configParException {
public:
  configParExceptionParNotRegistered(configpar_id_t configparid) : configParException("parameter not registered - this is a programming error"), configparid(configparid) {};
  virtual const char *what() const throw() { ostringstream more_detailed_error; more_detailed_error << error_msg << ", parid=" << configparid;
    return more_detailed_error.str().c_str(); }
private:
  configpar_id_t configparid;
};


class configParExceptionParseError : public configParException {
public:
  configParExceptionParseError(const std::string &msg) : configParException(msg) {};
};


/** confipar Base class (abstract)
 * this class only provides meta information about the configuration parameter:
 * its id, realm, name and description
 **/
class configparBase
{
public:

  // constructors
  configparBase() : realm_id(0), par_id(0), unit(NULL) {};
  configparBase(realm_id_t realm, configpar_id_t configparid, const char* name, const char* description, bool chg_at_runtime, const char* unitinfo= NULL);

  virtual ~configparBase() { }

  realm_id_t getRealmId() const throw() { return realm_id; }
  configpar_id_t getParId() const throw() { return par_id; }

  /// get the name of the parameter
  const string& getName() const { return name; }
  /// get a short description of the parameter
  const string& getDescription() const { return description; }
  /// get an informational description about the parameter's unit
  const char* getUnitInfo() const { return unit; }

  bool operator==(const configparBase& cfgpar) const;

  /// returns a copied object
  virtual configparBase* copy() const = 0;

  // these methods are the primitive versions of read and write
  virtual ostream& simpleWriteVal(ostream& outstream) const = 0;
  virtual istream& simpleReadVal(istream& inputstream) = 0;
  
  /// write the configuration parameter value (subclass should overwrite this method if anything special is needed)
  virtual ostream& writeVal(ostream& outstream) const = 0;
  /// read the configuration parameter value (subclass should overwrite this method if anything special is needed)
  virtual istream& readVal(istream& inputstream) = 0;

  /// write the configuration parameter value to config file (value will be quoted for some types, e.g. strings)
  virtual ostream& writeValToConfig(ostream& outstream) const = 0;
  /// read the configuration parameter value from config file (value will be quoted for some types, e.g. strings)
  virtual istream& readValFromConfig(istream& inputstream) = 0;

  /// return whether the parameter can be changed while the program is running
  bool isChangeableWhileRunning() const { return changeable_while_running; }

protected:
  ostream& appendUnitInfoComment(ostream& outstream) const;

public:
  static void write_quoted_string(std::ostream &out, const std::string &str);
  static std::string parse_quoted_string(std::istream &in);
  static void strip_leading_space(std::istream &in);
  static void skip_rest_of_line(std::istream &in);

private:
  realm_id_t     realm_id;
  configpar_id_t par_id;

  bool changeable_while_running;

  string name;
  string description;

  const char* unit; ///< informational text if the parameter has a unit 

};




/**
 * configpar<T> class
 * contains a configuration parameter of type T as private value
 *
 **/
template <typename T> class configpar : public configparBase
{
public:
  configpar<T>(const T& default_value) : configparBase(), value(default_value) {};
  configpar<T>(realm_id_t realm, configpar_id_t configparid, const char* name, const char* description, bool changeable_while_running, const T& default_value= T(), const char* unitinfo= NULL) : configparBase(realm, configparid, name, description, changeable_while_running, unitinfo), value(default_value) {};

  T getPar() const throw() { return value; }
  const T& getParRef() const throw() { return value; }
  T& getParRef() throw() { return value; }
  void setPar(const T& newval) throw() { value= newval; }

  operator T() const { return value; }

  bool operator==(const configpar<T>& cfgpar) const { return value == cfgpar.value && configparBase::operator==(cfgpar); }
  bool operator!=(const configpar<T>& cfgpar) const { return !operator==(cfgpar); }

  virtual configpar<T>* copy() const { return new configpar<T>(*this); }


  virtual ostream& simpleWriteVal(ostream& outstream) const;
  virtual istream& simpleReadVal(istream& inputstream);

  /// write the configuration parameter value (template specialization should overwrite this method if anything special is needed)
  virtual ostream& writeVal(ostream& outstream) const;
  /// read the configuration parameter value (template specialization should overwrite this method if anything special is needed)
  virtual istream& readVal(istream& inputstream);

  /// write the configuration parameter value to config file (value will be quoted for some types, e.g. strings)
  virtual ostream& writeValToConfig(ostream& outstream) const;
  /// read the configuration parameter value from config file (value will be quoted for some types, e.g. strings)
  virtual istream& readValFromConfig(istream& inputstream);

private:
  T value;
};





/** general inline functions **/

// use the default output operator for this type
template<typename T>
inline
ostream& 
configpar<T>::simpleWriteVal(ostream& outstream) const 
{ 
	return outstream << value; 
}

// use the default input operator for this type
template<typename T>
inline
istream& 
configpar<T>::simpleReadVal(istream& inputstream) 
{ 
	return inputstream >> value; 
}


template<typename T>
inline
ostream& 
configpar<T>::writeVal(ostream& outstream) const 
{ 
	return simpleWriteVal(outstream); 
}


template<typename T>
inline
istream& 
configpar<T>::readVal(istream& inputstream) 
{ 
	return simpleReadVal(inputstream); 
}


template<typename T>
inline
ostream& 
configpar<T>::writeValToConfig(ostream& outstream) const 
{ 
	writeVal(outstream);
	return appendUnitInfoComment(outstream);
}


template<typename T>
inline
istream& 
configpar<T>::readValFromConfig(istream& inputstream) 
{ 
	return readVal(inputstream); 
}



/** output operator **/
inline
ostream& operator<<(ostream& outstream, const configpar<bool>& cfgboolpar)
{
  return outstream << (cfgboolpar.operator==(true) ? "on/yes/true" : "off/no/false");
}





} // end namespace protlib


#endif // defined _CONFIGPAR__H
