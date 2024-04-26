/*
    nutconf.hpp - Nut configuration file manipulation API

    Copyright (C)
	2012	Emilien Kia <emilien.kia@gmail.com>
	2024	Jim Klimov <jimklimov+nut@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef NUTCONF_H_SEEN
#define NUTCONF_H_SEEN 1

#include "nutstream.hpp"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <stdexcept>

/* See include/common.h for details behind this */
#ifndef NUT_UNUSED_VARIABLE
#define NUT_UNUSED_VARIABLE(x) (void)(x)
#endif

#ifdef __cplusplus

namespace nut
{

class NutParser;
class NutConfigParser;
class DefaultConfigParser;
class GenericConfigParser;


/**
 * Helper to specify if a configuration variable is set or not.
 * In addition of its value.
 */
template<typename Type>
class Settable
{
protected:
	Type _value;
	bool _set;
public:
	Settable():_set(false){}
	Settable(const Settable<Type>& val):_value(val._value), _set(val._set){}
	Settable(const Type& val):_value(val), _set(true){}

	/* Avoid implicit copy/move operator declarations */
	Settable(Settable&&) = default;
	Settable& operator=(const Settable&) = default;
	Settable& operator=(Settable&&) = default;

	bool set()const{return _set;}
	void clear(){_set = false;}

	operator const Type&()const{return _value;}
	operator Type&(){return _value;}

	const Type& operator *()const{return _value;}
	Type& operator *(){return _value;}

	Settable<Type>& operator=(const Type& val){_value = val; _set = true; return *this;}

	bool operator==(const Settable<Type>& val)const
	{
		if(!set() && !val.set())
			return false;
		else
			return (set() && val.set() && _value==val._value);
	}

	bool operator==(const Type& val)const
	{
		if(!set())
			return false;
		else
			return _value == val;
	}

};


/**
 *  \brief  Serialisable interface
 *
 *  Classes that implement this interface provide way to serialize
 *  and deserialize instances to/from streams.
 */
class Serialisable
{
protected:

	/** Formal constructor */
	Serialisable() {}

public:

	/**
	 *  \brief  Deserializer
	 *
	 *  \param  istream  Input stream
	 *
	 *  \retval true  in case of success
	 *  \retval false in case of read error
	 */
	virtual bool parseFrom(NutStream & istream) = 0;

	/**
	 *  \brief  Serializer
	 *
	 *  \param  ostream  Output stream
	 *
	 *  \retval true  in case of success
	 *  \retval false in case of write error
	 */
	virtual bool writeTo(NutStream & ostream) const = 0;

	/** Destructor */
	virtual ~Serialisable();

};  // end of class Serialisable


/**
 * \brief	Mix a boolean (yes/no) or wider integer range
 */
class BoolInt
{
private:
	Settable<bool> b;
	Settable<int>  i;

public:
	/** If set, its value specifies if we want i==0/1 to mean false/true
	 *  in value-equality and type-cast operators for int and bool types.
	 *  NOTE: If true, assignment from "0" and "1" strings sets the int
	 *  not bool stored value representation!
	 *  NOTE: Survives the clear() call which applies to stored value,
	 *  but would be reset by assignment from another BoolInt object.
	 */
	Settable<bool> bool01;

	/** Leave all contents un-set */
	BoolInt() {}

	BoolInt(const bool val) {
		*this = val;
	}
	BoolInt(const bool val, bool newBool01) {
		this->bool01 = newBool01;
		*this = val;
	}

	BoolInt(const int val) {
		*this = val;
	}
	BoolInt(const int val, bool newBool01) {
		this->bool01 = newBool01;
		*this = val;
	}

	BoolInt(const char* val) {
		*this = val;
	}
	BoolInt(const char* val, bool newBool01) {
		this->bool01 = newBool01;
		*this = val;
	}

	BoolInt(const std::string &val) {
		*this = val;
	}
	BoolInt(const std::string &val, bool newBool01) {
		this->bool01 = newBool01;
		*this = val;
	}

	BoolInt(const BoolInt& other) {
		*this = other;
	}
	BoolInt(const BoolInt& other, bool newBool01) {
		this->bool01 = newBool01;
		*this = other;
		this->bool01 = newBool01;
	}

	inline void clear()
	{
		i.clear();
		b.clear();
	}

	inline BoolInt& operator=(const BoolInt& other)
	{
		clear();
		bool01.clear();

		if (other.b.set()) b = other.b;
		if (other.i.set()) i = other.i;

		if (other.bool01.set())
			bool01 = other.bool01;

		return *this;
	}

	inline BoolInt& operator=(BoolInt&& other)
	{
		clear();
		bool01.clear();

		if (other.b.set()) b = other.b;
		if (other.i.set()) i = other.i;

		if (other.bool01.set())
			bool01 = other.bool01;

		return *this;
	}

	inline BoolInt& operator=(int val)
	{
		clear();
		i = val;
		return *this;
	}

	inline BoolInt& operator=(bool val)
	{
		clear();
		b = val;
		return *this;
	}

	inline BoolInt& operator=(const char* s)
	{
		if (!s)
			throw std::invalid_argument(
				"BoolInt value from <null> string is is not supported");

		std::string src(s);
		return (*this = src);
	}

	inline BoolInt& operator=(std::string src)
	{
		static const Settable<bool> b0(false);
		static const Settable<bool> b1(true);

		// NOTE: Not a pointer, is not null at least
		if (src.empty())
			throw std::invalid_argument(
				"BoolInt value from <empty> string is is not supported");

		clear();

		if ("false" == src) { b = b0; return *this; }
		if ("off"   == src) { b = b0; return *this; }
		if ("0"     == src) {
			if (bool01.set() && bool01 == false) {
				i = 0;
			} else {
				b = b0;
			}
			return *this;
		}
		if ("no"    == src) { b = b0; return *this; }

		if ("true"  == src) { b = b1; return *this; }
		if ("on"    == src) { b = b1; return *this; }
		if ("1"     == src) {
			if (bool01.set() && bool01 == false) {
				i = 1;
			} else {
				b = b1;
			}
			return *this;
		}
		if ("yes"   == src) { b = b1; return *this; }
		if ("ok"    == src) { b = b1; return *this; }

		std::stringstream ss(src);
		int result;
		if (ss >> result && ss.rdbuf()->in_avail() == 0) {
			// Conversion succeeded and all chars were read
			// (e.g. not a decimal number)
			i = result;
#ifdef DEBUG
			std::cerr << "BoolInt assigned from '" << src
				<< "': got int '" << result << "'"
				<< " stream empty? " << ss.rdbuf()->in_avail()
				<< std::endl;
#endif
			return *this;
		}

		throw std::invalid_argument("BoolInt value from '" + src +
			"' string not understood as bool nor int");
	}

	inline BoolInt& operator<<(bool other)
	{
		*this = other;
		return *this;
	}

	inline BoolInt& operator<<(int other)
	{
		*this = other;
		return *this;
	}

	inline BoolInt& operator<<(std::string other)
	{
		*this = other;
		return *this;
	}

	inline BoolInt& operator<<(const char* other)
	{
		*this = other;
		return *this;
	}

	inline bool operator==(const BoolInt& other)const
	{
		// Either direct values are set and then equal; optionally
		// else numeric values of int and bool are cross-equal.
		if (b.set() && other.b.set()) return (b == other.b);
		if (i.set() && other.i.set()) return (i == other.i);

		if ((bool01.set() && bool01 == true)
		|| (!bool01.set() && other.bool01.set() && other.bool01 == true)
		) {
			// false if at least one object has neither i nor b
			// values "set()", or if their numeric values do not
			// match up as 0 or 1 exactly vs. boolean values.
			if (i.set() && other.b.set())
				return ( (other.b && i == 1) || (!other.b && i == 0) );
			if (b.set() && other.i.set())
				return ( (b && other.i == 1) || (!b && other.i == 0) );
		}

		return false;
	}

	inline bool operator==(const bool other)const
	{
		if (b.set()) return (b == other);
		if (bool01.set() && bool01 == true) {
			if (i.set())
				return ((other && i == 1) || (!other && i == 0));
		}
		return false;
	}

	inline bool operator==(const int other)const
	{
		if (i.set()) return (i == other);
		if (bool01.set() && bool01 == true) {
			if (b.set())
				return ((b && other == 1) || (!b && other == 0));
		}
		return false;
	}

	inline bool operator==(const std::string other)const
	{
		BoolInt tmp;
		if (bool01.set())
			tmp.bool01 = bool01;
		tmp = other;
		return (*this == tmp);
	}

	inline bool operator==(const char* s)const
	{
		if (!s)
			return false;

		std::string src(s);
		return (*this == src);
	}

	inline bool set()const
	{
		if (i.set() && b.set())
			throw std::invalid_argument(
				"BoolInt value somehow got both bool and int values set");

		return (i.set() || b.set());
	}

	operator int() {
		if (i.set()) return i;
		if (bool01.set() && bool01 == true) {
			if (b.set()) {
				if (b) return 1;
				return 0;
			}
		} else {
			throw std::invalid_argument(
				"BoolInt value not set to int");
		}

		throw std::invalid_argument(
			"BoolInt value not set, neither to bool nor to int");
	}

	operator bool() {
		if (b.set()) return b;
		if (bool01.set() && bool01 == true) {
			if (i.set()) {
				if (i == 0) return false;
				if (i == 1) return true;
			}
		} else {
			throw std::invalid_argument(
				"BoolInt value not set to bool");
		}

		throw std::invalid_argument(
			"BoolInt value not set, neither to bool nor to int");
	}

	inline std::string toString()const {
		if (b.set()) {
			if (b) return "yes";
			return "no";
		}

		if (i.set()) {
			if (bool01.set() && bool01 == true) {
				if (i == 0) return "no";
				if (i == 1) return "yes";
			}

			std::ostringstream ss;
			ss << i;
			return ss.str();
		}

		throw std::invalid_argument(
			"BoolInt value not set, neither to bool nor to int");
	}

	// FIXME: `std::string s = bi;` just won't work
	// but we can use `s = bi.toString()` or `cout << bi`
	operator std::string()const {
		return this->toString();
	}

	operator std::string&()const {
		return *(new std::string(this->operator std::string()));
	}
};

std::ostream& operator << (std::ostream &os, const BoolInt &bi);
inline std::ostream& operator << (std::ostream &os, const BoolInt &bi) {
	return (os << bi.toString());
}


/**
 * \brief	Certificate Identification structure for NUT
 *
 * Contains a certificate name and database password
 */
struct CertIdent
{
	Settable<std::string> certName, certDbPass;

	inline bool operator==(const CertIdent& ident)const
	{
		return certName == ident.certName && certDbPass == ident.certDbPass;
	}

	inline bool set()const
	{
		return certName.set() && certDbPass.set();
	}
};


/**
 * \brief	Certificate protected host structure for NUT
 *
 * Contains a host name, certificate name and option flags
 */
struct CertHost
{
	Settable<std::string> host, certName;
	nut::BoolInt certVerify, forceSsl;

	inline bool operator==(const CertHost& other)const
	{
		return certName == other.certName
			&& host == other.host
			&& certVerify == other.certVerify
			&& forceSsl == other.forceSsl;
	}

	inline bool set()const
	{
		return certName.set() && host.set()
			&& certVerify.set() && forceSsl.set();
	}
};


/**
 * NUT config parser.
 */
class NutParser
{
public:
	enum ParsingOption
	{
		OPTION_DEFAULT = 0,
		/** Colon character is considered as string character and not as specific token.
			Useful for IPv6 addresses */
		OPTION_IGNORE_COLON = 1
	};

	NutParser(const char* buffer = nullptr, unsigned int options = OPTION_DEFAULT);
	NutParser(const std::string& buffer, unsigned int options = OPTION_DEFAULT);

	virtual ~NutParser();

	/** Parsing configuration functions
	 * \{ */
	void setOptions(unsigned int options){_options = options;}
	unsigned int getOptions()const{return _options;}
	void setOptions(unsigned int options, bool set = true);
	void unsetOptions(unsigned int options){setOptions(options, false);}
	bool hasOptions(unsigned int options)const{return (_options&options) == options;}
	/** \} */

	struct Token
	{
		enum TokenType {
			TOKEN_UNKNOWN = -1,
			TOKEN_NONE    = 0,
			TOKEN_STRING  = 1,
			TOKEN_QUOTED_STRING,
			TOKEN_COMMENT,
			TOKEN_BRACKET_OPEN,
			TOKEN_BRACKET_CLOSE,
			TOKEN_EQUAL,
			TOKEN_COLON,
			TOKEN_EOL
		} type;
		std::string str;

		Token():type(TOKEN_NONE),str(){}
		Token(TokenType type_arg, const std::string& str_arg=""):type(type_arg),str(str_arg){}
		Token(TokenType type_arg, char c):type(type_arg),str(1, c){}
		Token(const Token& tok):type(tok.type),str(tok.str){}

		/* Avoid implicit copy/move operator declarations */
		Token(Token&&) = default;
		Token& operator=(const Token&) = default;
		Token& operator=(Token&&) = default;

		bool is(TokenType type_arg)const{return this->type==type_arg;}

		bool operator==(const Token& tok)const{return tok.type==type && tok.str==str;}

		operator bool()const{return type!=TOKEN_UNKNOWN && type!=TOKEN_NONE;}
	};

	/** Parsing functions
	* \{ */
	std::string parseCHARS();
	std::string parseSTRCHARS();
	Token parseToken();
	std::list<Token> parseLine();
	/** \} */

#ifndef UNITEST_MODE
protected:
#endif /* UNITEST_MODE */
	size_t getPos()const;
	void setPos(size_t pos);
	char charAt(size_t pos)const;

	void pushPos();
	size_t popPos();
	void rewind();

	void back();

	char get();
	char peek();

private:
	unsigned int _options;

	std::string _buffer;
	size_t _pos;
	std::vector<size_t> _stack;
};


typedef std::list<std::string> ConfigParamList;

struct GenericConfigSectionEntry
{
	std::string     name;
	ConfigParamList values;
	// std::string  comment;

};

struct GenericConfigSection
{
	/** Section entries map */
	typedef std::map<std::string, GenericConfigSectionEntry> EntryMap;

	std::string name;
	// std::string comment;
	EntryMap entries;

	const GenericConfigSectionEntry& operator [] (const std::string& varname)const{return entries.find(varname)->second;}
	GenericConfigSectionEntry& operator [] (const std::string& varname){return entries[varname];}

	bool empty()const;
	void clear();
};

class BaseConfiguration
{
	friend class GenericConfigParser;
public:
	virtual ~BaseConfiguration();
protected:
	virtual void setGenericConfigSection(const GenericConfigSection& section) = 0;
};

class NutConfigParser : public NutParser
{
public:
	virtual void parseConfig();

	/* Declared for cleaner overrides; arg ignored in current class */
	virtual void parseConfig(BaseConfiguration* config);

protected:
	NutConfigParser(const char* buffer = nullptr, unsigned int options = OPTION_DEFAULT);
	NutConfigParser(const std::string& buffer, unsigned int options = OPTION_DEFAULT);

	virtual void onParseBegin()=0;
	virtual void onParseComment(const std::string& comment)=0;
	virtual void onParseSectionName(const std::string& sectionName, const std::string& comment = "")=0;
	virtual void onParseDirective(const std::string& directiveName, char sep = 0, const ConfigParamList& values = ConfigParamList(), const std::string& comment = "")=0;
	virtual void onParseEnd()=0;
};

class DefaultConfigParser : public NutConfigParser
{
public:
	DefaultConfigParser(const char* buffer = nullptr);
	DefaultConfigParser(const std::string& buffer);

protected:
	virtual void onParseSection(const GenericConfigSection& section)=0;

	virtual void onParseBegin() override;
	virtual void onParseComment(const std::string& comment) override;
	virtual void onParseSectionName(const std::string& sectionName, const std::string& comment = "") override;
	virtual void onParseDirective(const std::string& directiveName, char sep = 0, const ConfigParamList& values = ConfigParamList(), const std::string& comment = "") override;
	virtual void onParseEnd() override;

	GenericConfigSection _section; ///> Currently parsed section
};


class GenericConfigParser : public DefaultConfigParser
{
public:
	GenericConfigParser(const char* buffer = nullptr);
	GenericConfigParser(const std::string& buffer);

	virtual void parseConfig(BaseConfiguration* config) override;

protected:
	virtual void onParseSection(const GenericConfigSection& section) override;

	BaseConfiguration* _config;
};


class GenericConfiguration : public BaseConfiguration, public Serialisable
{
public:
	/** Sections map */
	typedef std::map<std::string, GenericConfigSection> SectionMap;

	GenericConfiguration(){}

	virtual ~GenericConfiguration() override;

	void parseFromString(const std::string& str);

	/** Serialisable interface implementation \{ */
	bool parseFrom(NutStream & istream) override;
	bool writeTo(NutStream & ostream) const override;
	/** \} */

	// FIXME Let be public or set it as protected with public accessors ?
	SectionMap sections;

	const GenericConfigSection& operator[](const std::string& secname)const{return sections.find(secname)->second;}
	GenericConfigSection& operator[](const std::string& secname){return sections[secname];}


protected:
	virtual void setGenericConfigSection(const GenericConfigSection& section) override;

	/**
	 *  \brief  Configuration parameters getter
	 *
	 *  \param[in]   section  Section name
	 *  \param[in]   entry    Entry name
	 *  \param[out]  params   Configuration parameters
	 *
	 *  \retval true  if the entry was found
	 *  \retval false otherwise
	 */
	bool get(const std::string & section, const std::string & entry, ConfigParamList & params) const;

	/**
	 *  \brief  Global scope configuration parameters getter
	 *
	 *  \param[in]   entry    Entry name
	 *  \param[out]  params   Configuration parameters
	 *
	 *  \retval true  if the entry was found
	 *  \retval false otherwise
	 */
	inline bool get(const std::string & entry, ConfigParamList & params) const
	{
		return get("", entry, params);
	}

	/**
	 *  \brief  Configuration parameters setter
	 *
	 *  The section and entry are created unless they already exist.
	 *
	 *  \param[in]  section  Section name
	 *  \param[in]  entry    Entry name
	 *  \param[in]  params   Configuration parameters
	 */
	void set(const std::string & section, const std::string & entry, const ConfigParamList & params);

	/**
	 *  \brief  Global scope configuration parameters setter
	 *
	 *  The entry is created unless it already exists.
	 *
	 *  \param[in]  entry    Entry name
	 *  \param[in]  params   Configuration parameters
	 */
	inline void set(const std::string & entry, const ConfigParamList & params)
	{
		set("", entry, params);
	}

	/**
	 *  \brief  Add configuration parameters
	 *
	 *  The section and entry are created unless they already exist.
	 *  Current parameters are kept, the provided are added to the list end.
	 *
	 *  \param[in]  section  Section name
	 *  \param[in]  entry    Entry name
	 *  \param[in]  params   Configuration parameters
	 */
	void add(const std::string & section, const std::string & entry, const ConfigParamList & params);

	/**
	 *  \brief  Add global scope configuration parameters
	 *
	 *  The entry is created unless they already exists.
	 *  Current parameters are kept, the provided are added to the list end.
	 *
	 *  \param[in]  entry    Entry name
	 *  \param[in]  params   Configuration parameters
	 */
	inline void add(const std::string & entry, const ConfigParamList & params)
	{
		add("", entry, params);
	}

	/**
	 *  \brief  Configuration parameters removal
	 *
	 *  Removes the entry, only.
	 *  Does nothing if the section or the entry don't exist.
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 */
	void remove(const std::string & section, const std::string & entry);

	/**
	 *  \brief  Global scope configuration parameters removal
	 *
	 *  Removes the entry, only.
	 *  Does nothing if the entry don't exist.
	 *
	 *  \param  entry    Entry name
	 */
	inline void remove(const std::string & entry)
	{
		remove("", entry);
	}

	/**
	 *  \brief  Configuration section removal
	 *
	 *  Removes entire section (if exists).
	 *
	 *  \param  section  Section name
	 */
	void removeSection(const std::string & section);

	/** Global scope configuration removal */
	inline void removeGlobal()
	{
		removeSection("");
	}

	/**
	 *  \brief  Configuration string getter
	 *
	 *  Empty string is returned if the section or entry doesn't exist.
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *
	 *  \return Configuration parameter as string
	 */
	std::string getStr(
		const std::string & section,
		const std::string & entry) const;

	/**
	 *  \brief  Global scope configuration string getter
	 *
	 *  Empty string is returned if the entry doesn't exist.
	 *
	 *  \param  entry  Entry name
	 *
	 *  \return Configuration parameter as string
	 */
	inline std::string getStr(const std::string & entry) const
	{
		return getStr("", entry);
	}

	/**
	 *  \brief  Configuration string setter
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  value    Parameter value
	 */
	void setStr(
		const std::string & section,
		const std::string & entry,
		const std::string & value);

	/**
	 *  \brief  Global scope configuration string setter
	 *
	 *  \param  entry    Entry name
	 *  \param  value    Parameter value
	 */
	inline void setStr(
		const std::string & entry,
		const std::string & value)
	{
		setStr("", entry, value);
	}

	/**
	 *  \brief  Configuration flag getter
	 *
	 *  False is returned if the section or entry doesn't exist.
	 *  If a flag exists in configuration (any value is ignored),
	 *  it is effectively True.
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *
	 *  \return Configuration parameter as boolean
	 */
	bool getFlag(
		const std::string & section,
		const std::string & entry) const;

	/**
	 *  \brief  Global scope configuration flag getter
	 *
	 *  False is returned if the entry doesn't exist.
	 *  If a flag exists in configuration (any value is ignored),
	 *  it is effectively True.
	 *
	 *  \param  entry  Entry name
	 *
	 *  \return Configuration parameter as boolean
	 */
	inline bool getFlag(const std::string & entry) const
	{
		return getFlag("", entry);
	}

	/**
	 *  \brief  Configuration flag setter (mentioned == true)
	 *
	 *  Note: to unset a flag, just use remove() method.
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 */
	void setFlag(
		const std::string & section,
		const std::string & entry);

	/**
	 *  \brief  Global scope configuration flag setter (mentioned == true)
	 *
	 *  Note: to unset a flag, just use remove() method.
	 *
	 *  \param  entry    Entry name
	 */
	inline void setFlag(
		const std::string & entry)
	{
		setFlag("", entry);
	}

	/**
	 *  \brief  Configuration boolean option getter
	 *
	 *  Value depends on original string representation of a setting.
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 *
	 *  \return Configuration parameter as boolean (or the default if not defined)
	 */
	bool getBool(
		const std::string & section,
		const std::string & entry,
		bool                val = false) const;

	/**
	 *  \brief  Configuration boolean option getter
	 *
	 *  Value depends on original string representation of a setting.
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 *
	 *  \return Configuration parameter as boolean (or the default if not defined)
	 */
	// Avoid error: implicit conversion turns string literal
	//       into bool: 'const char[7]' to 'bool'
	bool getBool(
		const std::string & section,
		const char        * entry,
		bool                val = false) const
	{
		return getBool(section, std::string(entry), val);
	}

	/**
	 *  \brief  Global scope configuration boolean option getter
	 *
	 *  Value depends on original string representation of a setting.
	 *
	 *  \param  entry  Entry name
	 *  \param  val      Default value
	 *
	 *  \return Configuration parameter as boolean (or the default if not defined)
	 */
	inline bool getBool(const std::string & entry, bool val = false) const
	{
		return getBool("", entry, val);
	}

	/**
	 *  \brief  Configuration boolean option setter
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 */
	inline void setBool(
		const std::string & section,
		const std::string & entry,
		bool                val = true)
	{
		setStr(section, entry, bool2str(val));
	}

	/**
	 *  \brief  Global scope configuration boolean option setter
	 *
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 */
	inline void setBool(
		const std::string & entry,
		bool                val = true)
	{
		setBool("", entry, val);
	}

	/**
	 *  \brief  Configuration boolean option setter from a string
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 */
	inline void setBool(
		const std::string & section,
		const std::string & entry,
		const std::string & val = "true")
	{
		// Normalize:
		bool b = str2bool(val);
		setStr(section, entry, bool2str(b));
	}

	/**
	 *  \brief  Global scope configuration boolean option setter from a string
	 *
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 */
	inline void setBool(
		const std::string & entry,
		const std::string & val = "true")
	{
		setBool("", entry, val);
	}

	/**
	 *  \brief  Configuration number getter
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 *
	 *  \return Configuration parameter as number (or the default if not defined)
	 */
	long long int getInt(
		const std::string & section,
		const std::string & entry,
		long long int       val = 0) const;

	/**
	 *  \brief  Global scope configuration number getter
	 *
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 *
	 *  \return Configuration parameter as number (or the default if not defined)
	 */
	inline long long int getInt(const std::string & entry, long long int val = 0) const
	{
		return getInt("", entry, val);
	}

	/**
	 *  \brief  Configuration number setter
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 */
	void setInt(
		const std::string & section,
		const std::string & entry,
		long long int       val);

	/**
	 *  \brief  Global scope configuration number setter
	 *
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 */
	inline void setInt(
		const std::string & entry,
		long long int       val)
	{
		setInt("", entry, val);
	}

	/**
	 *  \brief  Configuration number getter (hex value even if without leading "0x")
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 *
	 *  \return Configuration parameter as number (or the default if not defined)
	 */
	long long int getIntHex(
		const std::string & section,
		const std::string & entry,
		long long int       val = 0) const;

	/**
	 *  \brief  Global scope configuration number getter (hex value even if without leading "0x")
	 *
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 *
	 *  \return Configuration parameter as number (or the default if not defined)
	 */
	inline long long int getIntHex(const std::string & entry, long long int val = 0) const
	{
		return getIntHex("", entry, val);
	}

	/**
	 *  \brief  Configuration number setter (hex value even if without leading "0x")
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 */
	void setIntHex(
		const std::string & section,
		const std::string & entry,
		long long int       val);

	/**
	 *  \brief  Global scope configuration number setter (hex value even if without leading "0x")
	 *
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 */
	inline void setIntHex(
		const std::string & entry,
		long long int       val)
	{
		setIntHex("", entry, val);
	}

	/**
	 *  \brief  Configuration floating-point number getter
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 *
	 *  \return Configuration parameter as number (or the default if not defined)
	 */
	double getDouble(
		const std::string & section,
		const std::string & entry,
		double              val = 0.0) const;

	/**
	 *  \brief  Global scope configuration floating-point number getter
	 *
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 *
	 *  \return Configuration parameter as number (or the default if not defined)
	 */
	inline double getDouble(const std::string & entry, double val = 0.0) const
	{
		return getDouble("", entry, val);
	}

	/**
	 *  \brief  Configuration floating-point number setter
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 */
	void setDouble(
		const std::string & section,
		const std::string & entry,
		double              val);

	/**
	 *  \brief  Global scope configuration floating-point number setter
	 *
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 */
	inline void setDouble(
		const std::string & entry,
		double              val)
	{
		setDouble("", entry, val);
	}

	/**
	 *  \brief  Cast numeric type with range check
	 *
	 *  Throws an exception on cast error.
	 *
	 *  \param  number  Number
	 *  \param  min     Minimum
	 *  \param  max     Maximum
	 *
	 *  \return \c number which was cast to target type
	 */
	template <typename T>
	static T range_cast(long long int number, long long int min, long long int max)
#if (defined __cplusplus) && (__cplusplus < 201100)
		throw(std::range_error)
#endif
	{
		if (number < min)
		{
			std::stringstream e;
			e << "Failed to range-cast " << number << " (underflows " << min << ')';

			throw std::range_error(e.str());
		}

		if (number > max)
		{
			std::stringstream e;
			e << "Failed to range-cast " << number << " (overflows " << max << ')';

			throw std::range_error(e.str());
		}

		return static_cast<T>(number);
	}

	/**
	 *  \brief  Configuration mixed boolean/int option getter
	 *
	 *  Value depends on original string representation of a setting.
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 *
	 *  \return Configuration parameter as BoolInt type for original
	 *          values which have a boolean or integer-numeric meaning
	 *          (or the default if not defined)
	 */
	nut::BoolInt getBoolInt(
		const std::string & section,
		const std::string & entry,
		nut::BoolInt        val = false) const;

	/**
	 *  \brief  Global scope configuration mixed boolean/int option getter
	 *
	 *  Value depends on original string representation of a setting.
	 *
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 *
	 *  \return Configuration parameter as BoolInt type for original
	 *          values which have a boolean or integer-numeric meaning
	 *          (or the default if not defined)
	 */
	inline nut::BoolInt getBoolInt(const std::string & entry, nut::BoolInt val = false) const
	{
		return getBoolInt("", entry, val);
	}

	/**
	 *  \brief  Configuration mixed boolean/int option setter
	 *
	 *  Input value types are auto-converted through BoolInt
	 *  type for sanity checks (e.g. throw exceptions for
	 *  invalid string contents) and are stored as strings
	 *  internally.
	 *
	 *  \param  section  Section name
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 */
	inline void setBoolInt(
		const std::string & section,
		const std::string & entry,
		nut::BoolInt        val = true)
	{
		setStr(section, entry, val);
	}

	/**
	 *  \brief  Global scope configuration mixed boolean/int option setter
	 *
	 *  \param  entry    Entry name
	 *  \param  val      Default value
	 */
	inline void setBoolInt(
		const std::string & entry,
		nut::BoolInt        val = true)
	{
		setBoolInt("", entry, val);
	}

	/**
	 *  \brief  Resolve string as Boolean value
	 *
	 *  \param  str  String
	 *
	 *  \retval true  IFF the string expresses a known true value
	 *  \retval false otherwise (no errors emitted for bogus inputs)
	 */
	static bool str2bool(const std::string & str);

	/**
	 *  \brief  Convert Boolean value to string
	 *
	 *  \param  val  Boolean value
	 *
	 *  \return \c val as string
	 */
	static const std::string & bool2str(bool val);

};  // end of class GenericConfiguration



class UpsmonConfiguration : public Serialisable
{
public:
	UpsmonConfiguration();
	void parseFromString(const std::string& str);

	Settable<int>          debugMin, pollFailLogThrottleMax;
	Settable<int>          offDuration, oblbDuration;
	Settable<std::string>  runAsUser, shutdownCmd, notifyCmd, powerDownFlag;
	/* yes|no (boolean) or a delay */
	Settable<nut::BoolInt> shutdownExit;
	/* practically boolean, but in 0|1 written form (bool01 fiddling) */
	Settable<nut::BoolInt> certVerify, forceSsl;
	Settable<std::string>  certPath;
	CertIdent              certIdent;
	std::list<CertHost>    certHosts;
	Settable<unsigned int> minSupplies, pollFreq, pollFreqAlert, hostSync;
	Settable<unsigned int> deadTime, rbWarnTime, noCommWarnTime, finalDelay;

	enum NotifyFlag {
		NOTIFY_IGNORE = 0,
		NOTIFY_SYSLOG = 1,
		NOTIFY_WALL = 1 << 1,
		NOTIFY_EXEC = 1 << 2
	};

	enum NotifyType {
		NOTIFY_ONLINE,
		NOTIFY_ONBATT,
		NOTIFY_LOWBATT,
		NOTIFY_FSD,
		NOTIFY_COMMOK,
		NOTIFY_COMMBAD,
		NOTIFY_SHUTDOWN,
		NOTIFY_REPLBATT,
		NOTIFY_NOCOMM,
		NOTIFY_NOPARENT,
		NOTIFY_CAL,
		NOTIFY_NOTCAL,
		NOTIFY_OFF,
		NOTIFY_NOTOFF,
		NOTIFY_BYPASS,
		NOTIFY_NOTBYPASS,
		NOTIFY_TYPE_MAX
	};

	static NotifyFlag NotifyFlagFromString(const std::string& str);
	static NotifyType NotifyTypeFromString(const std::string& str);

	Settable<unsigned int>   notifyFlags[NOTIFY_TYPE_MAX];
	Settable<std::string>    notifyMessages[NOTIFY_TYPE_MAX];

	struct Monitor {
		std::string upsname, hostname;
		uint16_t port;
		unsigned int powerValue;
		std::string username, password;
		bool isPrimary;
	};

	std::list<Monitor> monitors;

	/** Serialisable interface implementation \{ */
	bool parseFrom(NutStream & istream) override;
	bool writeTo(NutStream & ostream) const override;
	/** \} */

};  // end of class UpsmonConfiguration



class UpsmonConfigParser : public NutConfigParser
{
public:
	UpsmonConfigParser(const char* buffer = nullptr);
	UpsmonConfigParser(const std::string& buffer);

	void parseUpsmonConfig(UpsmonConfiguration* config);
protected:
	virtual void onParseBegin() override;
	virtual void onParseComment(const std::string& comment) override;
	virtual void onParseSectionName(const std::string& sectionName, const std::string& comment = "") override;
	virtual void onParseDirective(const std::string& directiveName, char sep = 0, const ConfigParamList& values = ConfigParamList(), const std::string& comment = "") override;
	virtual void onParseEnd() override;

	UpsmonConfiguration* _config;
};


class NutConfiguration: public Serialisable
{
public:
	NutConfiguration();
	void parseFromString(const std::string& str);

	enum NutMode {
		MODE_UNKNOWN = -1,
		MODE_NONE = 0,
		MODE_STANDALONE,
		MODE_NETSERVER,
		MODE_NETCLIENT,
		MODE_CONTROLLED,
		MODE_MANUAL,
	};

	Settable<NutMode> mode;

	Settable<bool>		allowNoDevice, allowNotAllListeners, poweroffQuiet;
	Settable<std::string>	upsdOptions, upsmonOptions;
	Settable<unsigned int>	poweroffWait;
	Settable<int>		debugLevel;

	static NutMode NutModeFromString(const std::string& str);

	/** Serialisable interface implementation \{ */
	bool parseFrom(NutStream & istream) override;
	bool writeTo(NutStream & ostream) const override;
	/** \} */
};


class NutConfConfigParser : public NutConfigParser
{
public:
	NutConfConfigParser(const char* buffer = nullptr);
	NutConfConfigParser(const std::string& buffer);

	void parseNutConfConfig(NutConfiguration* config);
protected:
	virtual void onParseBegin() override;
	virtual void onParseComment(const std::string& comment) override;
	virtual void onParseSectionName(const std::string& sectionName, const std::string& comment = "") override;
	virtual void onParseDirective(const std::string& directiveName, char sep = 0, const ConfigParamList& values = ConfigParamList(), const std::string& comment = "") override;
	virtual void onParseEnd() override;

	NutConfiguration* _config;
};


class UpsdConfiguration : public Serialisable
{
public:
	UpsdConfiguration();
	void parseFromString(const std::string& str);

	Settable<int> debugMin;
	Settable<unsigned int> maxAge, maxConn, trackingDelay, certRequestLevel;
	Settable<std::string>  statePath, certFile, certPath;
	Settable<bool> allowNoDevice, allowNotAllListeners, disableWeakSsl;

	struct Listen
	{
		std::string address;
		Settable<uint16_t> port;

		inline bool operator==(const Listen& listen)const
		{
			return address == listen.address && port == listen.port;
		}
	};
	std::list<Listen> listens;

	CertIdent certIdent;

	/** Serialisable interface implementation \{ */
	bool parseFrom(NutStream & istream) override;
	bool writeTo(NutStream & ostream) const override;
	/** \} */
};




class UpsdConfigParser : public NutConfigParser
{
public:
	UpsdConfigParser(const char* buffer = nullptr);
	UpsdConfigParser(const std::string& buffer);

	void parseUpsdConfig(UpsdConfiguration* config);
protected:
	virtual void onParseBegin() override;
	virtual void onParseComment(const std::string& comment) override;
	virtual void onParseSectionName(const std::string& sectionName, const std::string& comment = "") override;
	virtual void onParseDirective(const std::string& directiveName, char sep = 0, const ConfigParamList& values = ConfigParamList(), const std::string& comment = "") override;
	virtual void onParseEnd() override;

	UpsdConfiguration* _config;
};


/** UPS configuration */
class UpsConfiguration : public GenericConfiguration
{
public:
	/** Global configuration attributes getters and setters \{ */

	inline std::string getChroot()     const { return getStr("chroot"); }
	inline std::string getDriverPath() const { return getStr("driverpath"); }
	inline std::string getGroup()      const { return getStr("group"); }
	inline std::string getSynchronous() const { return getStr("synchronous"); }
	inline std::string getUser()       const { return getStr("user"); }

	// Flag - if exists then "true"
	inline bool getNoWait()            const { return getFlag("nowait"); }

	inline long long int getDebugMin()      const { return getInt("debug_min"); }
	inline long long int getMaxRetry()      const { return getInt("maxretry"); }
	inline long long int getMaxStartDelay() const { return getInt("maxstartdelay"); }
	inline long long int getPollInterval()  const { return getInt("pollinterval", 5); }  // TODO: check the default
	inline long long int getRetryDelay()    const { return getInt("retrydelay"); }

	inline void setChroot(const std::string & path)     { setStr("chroot",     path); }
	inline void setDriverPath(const std::string & path) { setStr("driverpath", path); }
	inline void setGroup(const std::string & group)     { setStr("group",      group); }
	inline void setSynchronous(const std::string & val) { setStr("synchronous", val); }
	inline void setUser(const std::string & user)       { setStr("user",       user); }

	inline void setNoWait()                             { setFlag("nowait"); }

	inline void setDebugMin(long long int num)          { setInt("debug_min",     num); }
	inline void setMaxRetry(long long int num)          { setInt("maxretry",      num); }
	inline void setMaxStartDelay(long long int delay)   { setInt("maxstartdelay", delay); }
	inline void setPollInterval(long long int interval) { setInt("pollinterval",  interval); }
	inline void setRetryDelay(long long int delay)      { setInt("retrydelay",    delay); }

	/** \} */

	/** Generic <key>=<value> getter */
	inline std::string getKey(const std::string & ups, const std::string & key) const { return getStr(ups, key); }

	/** Generic <key>=<value> setter */
	inline void setKey(const std::string & ups, const std::string & key, const std::string & val) {
		setStr(ups, key, val);
	}

	/** PUZZLE: What to do about "default.*" and "override.*"
	 * settings that apply to anything (vars!) that follows?
	 * Maybe nest the UpsConfiguration objects, and so query
	 * e.g. upscfg.default.getBatteryNominalVoltage() ?
	 * Or just keep that info in ConfigParamList per section
	 * and wrap free-style queries? */
	inline std::string   getDefaultStr(const std::string & ups, const std::string & key)            const { return getStr(ups, "default." + key); }
	inline long long int getDefaultInt(const std::string & ups, const std::string & key)            const { return getInt(ups, "default." + key); }
	inline long long int getDefaultIntHex(const std::string & ups, const std::string & key)         const { return getIntHex(ups, "default." + key); }
	inline bool          getDefaultFlag(const std::string & ups, const std::string & key)           const { return getFlag(ups, "default." + key); }
	inline bool          getDefaultBool(const std::string & ups, const std::string & key)           const { return getBool(ups, "default." + key); }
	inline nut::BoolInt  getDefaultBoolInt(const std::string & ups, const std::string & key)        const { return getBoolInt(ups, "default." + key); }
	inline double        getDefaultDouble(const std::string & ups, const std::string & key)         const { return getDouble(ups, "default." + key); }

	inline std::string   getOverrideStr(const std::string & ups, const std::string & key)           const { return getStr(ups, "override." + key); }
	inline long long int getOverrideInt(const std::string & ups, const std::string & key)           const { return getInt(ups, "override." + key); }
	inline long long int getOverrideIntHex(const std::string & ups, const std::string & key)        const { return getIntHex(ups, "override." + key); }
	inline bool          getOverrideFlag(const std::string & ups, const std::string & key)          const { return getFlag(ups, "override." + key); }
	inline bool          getOverrideBool(const std::string & ups, const std::string & key)          const { return getBool(ups, "override." + key); }
	inline nut::BoolInt  getOverrideBoolInt(const std::string & ups, const std::string & key)       const { return getBoolInt(ups, "override." + key); }
	inline double        getOverrideDouble(const std::string & ups, const std::string & key)        const { return getDouble(ups, "override." + key); }

	inline void setDefaultStr(const std::string & ups, const std::string & key, const std::string & val)  { setStr(ups, "default." + key, val); }
	inline void setDefaultInt(const std::string & ups, const std::string & key, long long int val)        { setInt(ups, "default." + key, val); }
	inline void setDefaultIntHex(const std::string & ups, const std::string & key, long long int val)     { setIntHex(ups, "default." + key, val); }
	inline void setDefaultFlag(const std::string & ups, const std::string & key)                          { setFlag(ups, "default." + key); }
	inline void setDefaultBool(const std::string & ups, const std::string & key, bool val)                { setBool(ups, "default." + key, val); }
	inline void setDefaultBoolInt(const std::string & ups, const std::string & key, nut::BoolInt val)     { setBoolInt(ups, "default." + key, val); }
	inline void setDefaultDouble(const std::string & ups, const std::string & key, double val)            { setDouble(ups, "default." + key, val); }

	inline void setOverrideStr(const std::string & ups, const std::string & key, const std::string & val) { setStr(ups, "override." + key, val); }
	inline void setOverrideInt(const std::string & ups, const std::string & key, long long int val)       { setInt(ups, "override." + key, val); }
	inline void setOverrideIntHex(const std::string & ups, const std::string & key, long long int val)    { setIntHex(ups, "override." + key, val); }
	inline void setOverrideFlag(const std::string & ups, const std::string & key)                         { setFlag(ups, "override." + key); }
	inline void setOverrideBool(const std::string & ups, const std::string & key, bool val)               { setBool(ups, "override." + key, val); }
	inline void setOverrideBoolInt(const std::string & ups, const std::string & key, nut::BoolInt val)    { setBoolInt(ups, "override." + key, val); }
	inline void setOverrideDouble(const std::string & ups, const std::string & key, double val)           { setDouble(ups, "override." + key, val); }

	/** UPS-specific configuration attributes getters and setters \{ */
	inline std::string getDriver(const std::string & ups)              const { return getStr(ups, "driver"); }
	inline std::string getDescription(const std::string & ups)         const { return getStr(ups, "desc"); }
	inline std::string getCP(const std::string & ups)                  const { return getStr(ups, "CP"); }
	inline std::string getCS(const std::string & ups)                  const { return getStr(ups, "CS"); }
	inline std::string getID(const std::string & ups)                  const { return getStr(ups, "ID"); }
	inline std::string getLB(const std::string & ups)                  const { return getStr(ups, "LB"); }
	inline std::string getLowBatt(const std::string & ups)             const { return getStr(ups, "LowBatt"); }
	inline std::string getOL(const std::string & ups)                  const { return getStr(ups, "OL"); }
	inline std::string getSD(const std::string & ups)                  const { return getStr(ups, "SD"); }
	inline std::string getAuthPassword(const std::string & ups)        const { return getStr(ups, "authPassword"); }
	inline std::string getAuthProtocol(const std::string & ups)        const { return getStr(ups, "authProtocol"); }
	inline std::string getAuthType(const std::string & ups)            const { return getStr(ups, "authtype"); }
	inline std::string getAWD(const std::string & ups)                 const { return getStr(ups, "awd"); }
	inline std::string getBatText(const std::string & ups)             const { return getStr(ups, "battext"); }
	inline std::string getBus(const std::string & ups)                 const { return getStr(ups, "bus"); }
	inline std::string getCommunity(const std::string & ups)           const { return getStr(ups, "community"); }
	inline std::string getFRUID(const std::string & ups)               const { return getStr(ups, "fruid"); }
	inline std::string getGroup(const std::string & ups)               const { return getStr(ups, "group"); }
	inline std::string getLoadStatus(const std::string & ups)          const { return getStr(ups, "load.status"); }
	inline std::string getLogin(const std::string & ups)               const { return getStr(ups, "login"); }
	inline std::string getLowbatt(const std::string & ups)             const { return getStr(ups, "lowbatt"); }
	inline std::string getManufacturer(const std::string & ups)        const { return getStr(ups, "manufacturer"); }
	inline std::string getMethodOfFlowControl(const std::string & ups) const { return getStr(ups, "methodOfFlowControl"); }
	inline std::string getMIBs(const std::string & ups)                const { return getStr(ups, "mibs"); }
	inline std::string getModel(const std::string & ups)               const { return getStr(ups, "model"); }
	inline std::string getModelName(const std::string & ups)           const { return getStr(ups, "modelname"); }
	inline std::string getNotification(const std::string & ups)        const { return getStr(ups, "notification"); }
	inline std::string getOldMAC(const std::string & ups)              const { return getStr(ups, "oldmac"); }
	inline std::string getPassword(const std::string & ups)            const { return getStr(ups, "password"); }
	inline std::string getPort(const std::string & ups)                const { return getStr(ups, "port"); }
	inline std::string getPrefix(const std::string & ups)              const { return getStr(ups, "prefix"); }
	inline std::string getPrivPassword(const std::string & ups)        const { return getStr(ups, "privPassword"); }
	inline std::string getPrivProtocol(const std::string & ups)        const { return getStr(ups, "privProtocol"); }
	inline std::string getProduct(const std::string & ups)             const { return getStr(ups, "product"); }
	inline std::string getProductID(const std::string & ups)           const { return getStr(ups, "productid"); }
	inline std::string getProtocol(const std::string & ups)            const { return getStr(ups, "protocol"); }
	inline std::string getRuntimeCal(const std::string & ups)          const { return getStr(ups, "runtimecal"); }
	inline std::string getSDType(const std::string & ups)              const { return getStr(ups, "sdtype"); }
	inline std::string getSecLevel(const std::string & ups)            const { return getStr(ups, "secLevel"); }
	inline std::string getSecName(const std::string & ups)             const { return getStr(ups, "secName"); }
	inline std::string getSensorID(const std::string & ups)            const { return getStr(ups, "sensorid"); }
	inline std::string getSerial(const std::string & ups)              const { return getStr(ups, "serial"); }
	inline std::string getSerialNumber(const std::string & ups)        const { return getStr(ups, "serialnumber"); }
	inline std::string getShutdownArguments(const std::string & ups)   const { return getStr(ups, "shutdownArguments"); }
	inline std::string getSNMPversion(const std::string & ups)         const { return getStr(ups, "snmp_version"); }
	inline std::string getSubdriver(const std::string & ups)           const { return getStr(ups, "subdriver"); }
	inline std::string getSynchronous(const std::string & ups)         const { return getStr(ups, "synchronous"); }
	inline std::string getType(const std::string & ups)                const { return getStr(ups, "type"); }
	inline std::string getUPStype(const std::string & ups)             const { return getStr(ups, "upstype"); }
	inline std::string getUSD(const std::string & ups)                 const { return getStr(ups, "usd"); }
	inline std::string getUser(const std::string & ups)                const { return getStr(ups, "user"); }
	inline std::string getUsername(const std::string & ups)            const { return getStr(ups, "username"); }
	inline std::string getValidationSequence(const std::string & ups)  const { return getStr(ups, "validationSequence"); }
	inline std::string getVendor(const std::string & ups)              const { return getStr(ups, "vendor"); }
	inline std::string getVendorID(const std::string & ups)            const { return getStr(ups, "vendorid"); }
	inline std::string getWUGrace(const std::string & ups)             const { return getStr(ups, "wugrace"); }


	inline long long int getDebugMin(const std::string & ups)          const { return getInt(ups, "debug_min"); }
	inline long long int getSDOrder(const std::string & ups)           const { return getInt(ups, "sdorder"); }             // TODO: Is that a number?
	inline long long int getMaxStartDelay(const std::string & ups)     const { return getInt(ups, "maxstartdelay"); }
	inline long long int getAdvOrder(const std::string & ups)          const { return getInt(ups, "advorder"); }            // CHECKME
	inline long long int getBatteryPercentage(const std::string & ups) const { return getInt(ups, "batteryPercentage"); }   // CHECKME
	inline long long int getOffDelay(const std::string & ups)          const { return getInt(ups, "OffDelay"); }            // CHECKME
	inline long long int getOnDelay(const std::string & ups)           const { return getInt(ups, "OnDelay"); }             // CHECKME
	inline long long int getBattVoltMult(const std::string & ups)      const { return getInt(ups, "battvoltmult"); }        // CHECKME
	inline long long int getBaudRate(const std::string & ups)          const { return getInt(ups, "baud_rate"); }           // CHECKME
	inline long long int getBaudrate(const std::string & ups)          const { return getInt(ups, "baudrate"); }            // CHECKME
	inline long long int getCablePower(const std::string & ups)        const { return getInt(ups, "cablepower"); }          // CHECKME
	inline long long int getChargeTime(const std::string & ups)        const { return getInt(ups, "chargetime"); }          // CHECKME
	inline long long int getDaysOff(const std::string & ups)           const { return getInt(ups, "daysoff"); }             // CHECKME
	inline long long int getDaySweek(const std::string & ups)          const { return getInt(ups, "daysweek"); }            // CHECKME
	inline long long int getFrequency(const std::string & ups)         const { return getInt(ups, "frequency"); }           // CHECKME
	inline long long int getHourOff(const std::string & ups)           const { return getInt(ups, "houroff"); }             // CHECKME
	inline long long int getHourOn(const std::string & ups)            const { return getInt(ups, "houron"); }              // CHECKME
	inline long long int getIdleLoad(const std::string & ups)          const { return getInt(ups, "idleload"); }            // CHECKME
	inline long long int getInputTimeout(const std::string & ups)      const { return getInt(ups, "input_timeout"); }       // CHECKME
	inline long long int getLineVoltage(const std::string & ups)       const { return getInt(ups, "linevoltage"); }         // CHECKME
	inline long long int getLoadpercentage(const std::string & ups)    const { return getInt(ups, "loadPercentage"); }      // CHECKME
	inline long long int getMaxLoad(const std::string & ups)           const { return getInt(ups, "max_load"); }            // CHECKME
	inline long long int getMFR(const std::string & ups)               const { return getInt(ups, "mfr"); }                 // CHECKME
	inline long long int getMinCharge(const std::string & ups)         const { return getInt(ups, "mincharge"); }           // CHECKME
	inline long long int getMinRuntime(const std::string & ups)        const { return getInt(ups, "minruntime"); }          // CHECKME
	inline long long int getNomBattVolt(const std::string & ups)       const { return getInt(ups, "nombattvolt"); }         // CHECKME
	inline long long int getNumOfBytesFromUPS(const std::string & ups) const { return getInt(ups, "numOfBytesFromUPS"); }   // CHECKME
	inline long long int getOffdelay(const std::string & ups)          const { return getInt(ups, "offdelay"); }            // CHECKME
	inline long long int getOndelay(const std::string & ups)           const { return getInt(ups, "ondelay"); }             // CHECKME
	inline long long int getOutputPace(const std::string & ups)        const { return getInt(ups, "output_pace"); }         // CHECKME
	inline long long int getPollFreq(const std::string & ups)          const { return getInt(ups, "pollfreq"); }            // CHECKME
	inline long long int getPowerUp(const std::string & ups)           const { return getInt(ups, "powerup"); }             // CHECKME
	inline long long int getPrgShut(const std::string & ups)           const { return getInt(ups, "prgshut"); }             // CHECKME
	inline long long int getRebootDelay(const std::string & ups)       const { return getInt(ups, "rebootdelay"); }         // CHECKME
	inline long long int getSDtime(const std::string & ups)            const { return getInt(ups, "sdtime"); }              // CHECKME
	inline long long int getShutdownDelay(const std::string & ups)     const { return getInt(ups, "shutdown_delay"); }      // CHECKME
	inline long long int getStartDelay(const std::string & ups)        const { return getInt(ups, "startdelay"); }          // CHECKME
	inline long long int getTestTime(const std::string & ups)          const { return getInt(ups, "testtime"); }            // CHECKME
	inline long long int getTimeout(const std::string & ups)           const { return getInt(ups, "timeout"); }             // CHECKME
	inline long long int getUPSdelayShutdown(const std::string & ups)  const { return getInt(ups, "ups.delay.shutdown"); }  // CHECKME
	inline long long int getUPSdelayStart(const std::string & ups)     const { return getInt(ups, "ups.delay.start"); }     // CHECKME
	inline long long int getVoltage(const std::string & ups)           const { return getInt(ups, "voltage"); }             // CHECKME
	inline long long int getWait(const std::string & ups)              const { return getInt(ups, "wait"); }                // CHECKME

	// May be a flag or a number; 0 is among valid values (default -1 for unset)
	inline long long int getUsbSetAltInterface(const std::string & ups)          const { return getInt(ups, "usb_set_altinterface", -1); }      // CHECKME

	// NUT specifies these as "hexnum" values (optionally with prefixed 0x but hex anyway)
	inline long long int getUsbConfigIndex(const std::string & ups)              const { return getIntHex(ups, "usb_config_index"); }           // CHECKME
	inline long long int getUsbHidDescIndex(const std::string & ups)             const { return getIntHex(ups, "usb_hid_desc_index"); }         // CHECKME
	inline long long int getUsbHidRepIndex(const std::string & ups)              const { return getIntHex(ups, "usb_hid_rep_index"); }          // CHECKME
	inline long long int getUsbHidEndpointIn(const std::string & ups)            const { return getIntHex(ups, "usb_hid_ep_in"); }              // CHECKME
	inline long long int getUsbHidEndpointOut(const std::string & ups)           const { return getIntHex(ups, "usb_hid_ep_out"); }             // CHECKME

	// Flag - if exists then "true"
	inline bool getIgnoreLB(const std::string & ups)       const { return getFlag(ups, "ignorelb"); }

	inline bool getNolock(const std::string & ups)         const { return getBool(ups, "nolock"); }
	inline bool getCable(const std::string & ups)          const { return getBool(ups, "cable"); }
	inline bool getDumbTerm(const std::string & ups)       const { return getBool(ups, "dumbterm"); }
	inline bool getExplore(const std::string & ups)        const { return getBool(ups, "explore"); }
	inline bool getFakeLowBatt(const std::string & ups)    const { return getBool(ups, "fake_lowbatt"); }
	inline bool getFlash(const std::string & ups)          const { return getBool(ups, "flash"); }
	inline bool getFullUpdate(const std::string & ups)     const { return getBool(ups, "full_update"); }
	inline bool getLangIDfix(const std::string & ups)      const { return getBool(ups, "langid_fix"); }
	inline bool getLoadOff(const std::string & ups)        const { return getBool(ups, "load.off"); }
	inline bool getLoadOn(const std::string & ups)         const { return getBool(ups, "load.on"); }
	inline bool getNoHang(const std::string & ups)         const { return getBool(ups, "nohang"); }
	inline bool getNoRating(const std::string & ups)       const { return getBool(ups, "norating"); }
	inline bool getNoTransferOIDs(const std::string & ups) const { return getBool(ups, "notransferoids"); }
	inline bool getNoVendor(const std::string & ups)       const { return getBool(ups, "novendor"); }
	inline bool getNoWarnNoImp(const std::string & ups)    const { return getBool(ups, "nowarn_noimp"); }
	inline bool getPollOnly(const std::string & ups)       const { return getBool(ups, "pollonly"); }
	inline bool getSilent(const std::string & ups)         const { return getBool(ups, "silent"); }
	inline bool getStatusOnly(const std::string & ups)     const { return getBool(ups, "status_only"); }
	inline bool getSubscribe(const std::string & ups)      const { return getBool(ups, "subscribe"); }
	inline bool getUseCRLF(const std::string & ups)        const { return getBool(ups, "use_crlf"); }
	inline bool getUsePreLF(const std::string & ups)       const { return getBool(ups, "use_pre_lf"); }


	inline void setDriver(const std::string & ups, const std::string & driver)                { setStr(ups, "driver",              driver); }
	inline void setDescription(const std::string & ups, const std::string & desc)             { setStr(ups, "desc",                desc); }
	inline void setLowBatt(const std::string & ups, const std::string & lowbatt)              { setStr(ups, "LowBatt",             lowbatt); }
	inline void setOL(const std::string & ups, const std::string & ol)                        { setStr(ups, "OL",                  ol); }
	inline void setSD(const std::string & ups, const std::string & sd)                        { setStr(ups, "SD",                  sd); }
	inline void setAuthPassword(const std::string & ups, const std::string & auth_passwd)     { setStr(ups, "authPassword",        auth_passwd); }
	inline void setAuthProtocol(const std::string & ups, const std::string & auth_proto)      { setStr(ups, "authProtocol",        auth_proto); }
	inline void setAuthType(const std::string & ups, const std::string & authtype)            { setStr(ups, "authtype",            authtype); }
	inline void setAWD(const std::string & ups, const std::string & awd)                      { setStr(ups, "awd",                 awd); }
	inline void setBatText(const std::string & ups, const std::string & battext)              { setStr(ups, "battext",             battext); }
	inline void setBus(const std::string & ups, const std::string & bus)                      { setStr(ups, "bus",                 bus); }
	inline void setCommunity(const std::string & ups, const std::string & community)          { setStr(ups, "community",           community); }
	inline void setFRUID(const std::string & ups, const std::string & fruid)                  { setStr(ups, "fruid",               fruid); }
	inline void setGroup(const std::string & ups, const std::string & group)                  { setStr(ups, "group",               group); }
	inline void setLoadStatus(const std::string & ups, const std::string & load_status)       { setStr(ups, "load.status",         load_status); }
	inline void setLogin(const std::string & ups, const std::string & login)                  { setStr(ups, "login",               login); }
	inline void setLowbatt(const std::string & ups, const std::string & lowbatt)              { setStr(ups, "lowbatt",             lowbatt); }
	inline void setManufacturer(const std::string & ups, const std::string & manufacturer)    { setStr(ups, "manufacturer",        manufacturer); }
	inline void setMethodOfFlowControl(const std::string & ups, const std::string & method)   { setStr(ups, "methodOfFlowControl", method); }
	inline void setMIBs(const std::string & ups, const std::string & mibs)                    { setStr(ups, "mibs",                mibs); }
	inline void setModel(const std::string & ups, const std::string & model)                  { setStr(ups, "model",               model); }
	inline void setModelName(const std::string & ups, const std::string & modelname)          { setStr(ups, "modelname",           modelname); }
	inline void setNotification(const std::string & ups, const std::string & notification)    { setStr(ups, "notification",        notification); }
	inline void setOldMAC(const std::string & ups, const std::string & oldmac)                { setStr(ups, "oldmac",              oldmac); }
	inline void setPassword(const std::string & ups, const std::string & password)            { setStr(ups, "password",            password); }
	inline void setPort(const std::string & ups, const std::string & port)                    { setStr(ups, "port",                port); }
	inline void setPrefix(const std::string & ups, const std::string & prefix)                { setStr(ups, "prefix",              prefix); }
	inline void setPrivPassword(const std::string & ups, const std::string & priv_passwd)     { setStr(ups, "privPassword",        priv_passwd); }
	inline void setPrivProtocol(const std::string & ups, const std::string & priv_proto)      { setStr(ups, "privProtocol",        priv_proto); }
	inline void setProduct(const std::string & ups, const std::string & product)              { setStr(ups, "product",             product); }
	inline void setProductID(const std::string & ups, const std::string & productid)          { setStr(ups, "productid",           productid); }
	inline void setProtocol(const std::string & ups, const std::string & protocol)            { setStr(ups, "protocol",            protocol); }
	inline void setRuntimeCal(const std::string & ups, const std::string & runtimecal)        { setStr(ups, "runtimecal",          runtimecal); }
	inline void setSDtype(const std::string & ups, const std::string & sdtype)                { setStr(ups, "sdtype",              sdtype); }
	inline void setSecLevel(const std::string & ups, const std::string & sec_level)           { setStr(ups, "secLevel",            sec_level); }
	inline void setSecName(const std::string & ups, const std::string & sec_name)             { setStr(ups, "secName",             sec_name); }
	inline void setSensorID(const std::string & ups, const std::string & sensorid)            { setStr(ups, "sensorid",            sensorid); }
	inline void setSerial(const std::string & ups, const std::string & serial)                { setStr(ups, "serial",              serial); }
	inline void setSerialNumber(const std::string & ups, const std::string & serialnumber)    { setStr(ups, "serialnumber",        serialnumber); }
	inline void setShutdownArguments(const std::string & ups, const std::string & sd_args)    { setStr(ups, "shutdownArguments",   sd_args); }
	inline void setSNMPversion(const std::string & ups, const std::string & snmp_version)     { setStr(ups, "snmp_version",        snmp_version); }
	inline void setSubdriver(const std::string & ups, const std::string & subdriver)          { setStr(ups, "subdriver",           subdriver); }
	inline void setSynchronous(const std::string & ups, const std::string & synchronous)      { setStr(ups, "synchronous",         synchronous); }
	inline void setType(const std::string & ups, const std::string & type)                    { setStr(ups, "type",                type); }
	inline void setUPStype(const std::string & ups, const std::string & upstype)              { setStr(ups, "upstype",             upstype); }
	inline void setUSD(const std::string & ups, const std::string & usd)                      { setStr(ups, "usd",                 usd); }
	inline void setUsername(const std::string & ups, const std::string & username)            { setStr(ups, "username",            username); }
	inline void setUser(const std::string & ups, const std::string & user)                    { setStr(ups, "user",                user); }
	inline void setValidationSequence(const std::string & ups, const std::string & valid_seq) { setStr(ups, "validationSequence",  valid_seq); }
	inline void setVendor(const std::string & ups, const std::string & vendor)                { setStr(ups, "vendor",              vendor); }
	inline void setVendorID(const std::string & ups, const std::string & vendorid)            { setStr(ups, "vendorid",            vendorid); }
	inline void setWUGrace(const std::string & ups, const std::string & wugrace)              { setStr(ups, "wugrace",             wugrace); }

	inline void setDebugMin(const std::string & ups, long long int val)            { setInt(ups, "debug_min",          val); }
	inline void setSDOrder(const std::string & ups, long long int ord)             { setInt(ups, "sdorder",            ord); }
	inline void setMaxStartDelay(const std::string & ups, long long int delay)     { setInt(ups, "maxstartdelay",      delay); }
	inline void setADVorder(const std::string & ups, long long int advorder)       { setInt(ups, "advorder",           advorder); }     // CHECKME
	inline void setBatteryPercentage(const std::string & ups, long long int batt)  { setInt(ups, "batteryPercentage",  batt); }         // CHECKME
	inline void setOffDelay(const std::string & ups, long long int offdelay)       { setInt(ups, "OffDelay",           offdelay); }     // CHECKME
	inline void setOnDelay(const std::string & ups, long long int ondelay)         { setInt(ups, "OnDelay",            ondelay); }      // CHECKME
	inline void setBattVoltMult(const std::string & ups, long long int mult)       { setInt(ups, "battvoltmult",       mult); }         // CHECKME
	inline void setBaudRate(const std::string & ups, long long int baud_rate)      { setInt(ups, "baud_rate",          baud_rate); }    // CHECKME
	inline void setBaudrate(const std::string & ups, long long int baudrate)       { setInt(ups, "baudrate",           baudrate); }     // CHECKME
	inline void setCablePower(const std::string & ups, long long int cablepower)   { setInt(ups, "cablepower",         cablepower); }   // CHECKME
	inline void setChargeTime(const std::string & ups, long long int chargetime)   { setInt(ups, "chargetime",         chargetime); }   // CHECKME
	inline void setDaysOff(const std::string & ups, long long int daysoff)         { setInt(ups, "daysoff",            daysoff); }      // CHECKME
	inline void setDaysWeek(const std::string & ups, long long int daysweek)       { setInt(ups, "daysweek",           daysweek); }     // CHECKME
	inline void setFrequency(const std::string & ups, long long int frequency)     { setInt(ups, "frequency",          frequency); }    // CHECKME
	inline void setHourOff(const std::string & ups, long long int houroff)         { setInt(ups, "houroff",            houroff); }      // CHECKME
	inline void setHourOn(const std::string & ups, long long int houron)           { setInt(ups, "houron",             houron); }       // CHECKME
	inline void setIdleLoad(const std::string & ups, long long int idleload)       { setInt(ups, "idleload",           idleload); }     // CHECKME
	inline void setInputTimeout(const std::string & ups, long long int timeout)    { setInt(ups, "input_timeout",      timeout); }      // CHECKME
	inline void setLineVoltage(const std::string & ups, long long int linevoltage) { setInt(ups, "linevoltage",        linevoltage); }  // CHECKME
	inline void setLoadpercentage(const std::string & ups, long long int load)     { setInt(ups, "loadPercentage",     load); }         // CHECKME
	inline void setMaxLoad(const std::string & ups, long long int max_load)        { setInt(ups, "max_load",           max_load); }     // CHECKME
	inline void setMFR(const std::string & ups, long long int mfr)                 { setInt(ups, "mfr",                mfr); }          // CHECKME
	inline void setMinCharge(const std::string & ups, long long int mincharge)     { setInt(ups, "mincharge",          mincharge); }    // CHECKME
	inline void setMinRuntime(const std::string & ups, long long int minruntime)   { setInt(ups, "minruntime",         minruntime); }   // CHECKME
	inline void setNomBattVolt(const std::string & ups, long long int nombattvolt) { setInt(ups, "nombattvolt",        nombattvolt); }  // CHECKME
	inline void setNumOfBytesFromUPS(const std::string & ups, long long int bytes) { setInt(ups, "numOfBytesFromUPS",  bytes); }        // CHECKME
	inline void setOffdelay(const std::string & ups, long long int offdelay)       { setInt(ups, "offdelay",           offdelay); }     // CHECKME
	inline void setOndelay(const std::string & ups, long long int ondelay)         { setInt(ups, "ondelay",            ondelay); }      // CHECKME
	inline void setOutputPace(const std::string & ups, long long int output_pace)  { setInt(ups, "output_pace",        output_pace); }  // CHECKME
	inline void setPollFreq(const std::string & ups, long long int pollfreq)       { setInt(ups, "pollfreq",           pollfreq); }     // CHECKME
	inline void setPowerUp(const std::string & ups, long long int powerup)         { setInt(ups, "powerup",            powerup); }      // CHECKME
	inline void setPrgShut(const std::string & ups, long long int prgshut)         { setInt(ups, "prgshut",            prgshut); }      // CHECKME
	inline void setRebootDelay(const std::string & ups, long long int delay)       { setInt(ups, "rebootdelay",        delay); }        // CHECKME
	inline void setSDtime(const std::string & ups, long long int sdtime)           { setInt(ups, "sdtime",             sdtime); }       // CHECKME
	inline void setShutdownDelay(const std::string & ups, long long int delay)     { setInt(ups, "shutdown_delay",     delay); }        // CHECKME
	inline void setStartDelay(const std::string & ups, long long int delay)        { setInt(ups, "startdelay",         delay); }        // CHECKME
	inline void setTestTime(const std::string & ups, long long int testtime)       { setInt(ups, "testtime",           testtime); }     // CHECKME
	inline void setTimeout(const std::string & ups, long long int timeout)         { setInt(ups, "timeout",            timeout); }      // CHECKME
	inline void setUPSdelayShutdown(const std::string & ups, long long int delay)  { setInt(ups, "ups.delay.shutdown", delay); }        // CHECKME
	inline void setUPSdelayStart(const std::string & ups, long long int delay)     { setInt(ups, "ups.delay.start",    delay); }        // CHECKME
	inline void setVoltage(const std::string & ups, long long int voltage)         { setInt(ups, "voltage",            voltage); }      // CHECKME
	inline void setWait(const std::string & ups, long long int wait)               { setInt(ups, "wait",               wait); }         // CHECKME

	// May be a flag or a number; 0 is among valid values (default -1 for unset)
	inline void setUsbSetAltInterface(const std::string & ups, long long int val = 0)       { if (val >= 0) { setInt(ups, "usb_set_altinterface", val); } else { remove(ups, "usb_set_altinterface"); } }         // CHECKME

	// NUT specifies these as "hexnum" values (optionally with prefixed 0x but hex anyway)
	inline void setUsbConfigIndex(const std::string & ups, long long int val)               { setIntHex(ups, "usb_config_index",               val); }         // CHECKME
	inline void setUsbHidDescIndex(const std::string & ups, long long int val)              { setIntHex(ups, "usb_hid_desc_index",             val); }         // CHECKME
	inline void setUsbHidRepIndex(const std::string & ups, long long int val)               { setIntHex(ups, "usb_hid_rep_index",              val); }         // CHECKME
	inline void setUsbHidEndpointIn(const std::string & ups, long long int val)             { setIntHex(ups, "usb_hid_ep_in",                  val); }         // CHECKME
	inline void setUsbHidEndpointOut(const std::string & ups, long long int val)            { setIntHex(ups, "usb_hid_ep_out",                 val); }         // CHECKME

	// Flag - if exists then "true"; remove() to "unset" => "false"
	inline void setIgnoreLB(const std::string & ups)                        { setFlag(ups, "ignorelb"); }

	inline void setNolock(const std::string & ups, bool set = true)         { setBool(ups, "nolock",         set); }
	inline void setCable(const std::string & ups, bool set = true)          { setBool(ups, "cable",          set); }
	inline void setDumbTerm(const std::string & ups, bool set = true)       { setBool(ups, "dumbterm",       set); }
	inline void setExplore(const std::string & ups, bool set = true)        { setBool(ups, "explore",        set); }
	inline void setFakeLowBatt(const std::string & ups, bool set = true)    { setBool(ups, "fake_lowbatt",   set); }
	inline void setFlash(const std::string & ups, bool set = true)          { setBool(ups, "flash",          set); }
	inline void setFullUpdate(const std::string & ups, bool set = true)     { setBool(ups, "full_update",    set); }
	inline void setLangIDfix(const std::string & ups, bool set = true)      { setBool(ups, "langid_fix",     set); }
	inline void setLoadOff(const std::string & ups, bool set = true)        { setBool(ups, "load.off",       set); }
	inline void setLoadOn(const std::string & ups, bool set = true)         { setBool(ups, "load.on",        set); }
	inline void setNoHang(const std::string & ups, bool set = true)         { setBool(ups, "nohang",         set); }
	inline void setNoRating(const std::string & ups, bool set = true)       { setBool(ups, "norating",       set); }
	inline void setNoTransferOIDs(const std::string & ups, bool set = true) { setBool(ups, "notransferoids", set); }
	inline void setNoVendor(const std::string & ups, bool set = true)       { setBool(ups, "novendor",       set); }
	inline void setNoWarnNoImp(const std::string & ups, bool set = true)    { setBool(ups, "nowarn_noimp",   set); }
	inline void setPollOnly(const std::string & ups, bool set = true)       { setBool(ups, "pollonly",       set); }
	inline void setSilent(const std::string & ups, bool set = true)         { setBool(ups, "silent",         set); }
	inline void setStatusOnly(const std::string & ups, bool set = true)     { setBool(ups, "status_only",    set); }
	inline void setSubscribe(const std::string & ups, bool set = true)      { setBool(ups, "subscribe",      set); }
	inline void setUseCRLF(const std::string & ups, bool set = true)        { setBool(ups, "use_crlf",       set); }
	inline void setUsePreLF(const std::string & ups, bool set = true)       { setBool(ups, "use_pre_lf",     set); }

	/** \} */

	virtual ~UpsConfiguration() override;
};  // end of class UpsConfiguration


/** upsd users configuration */
class UpsdUsersConfiguration : public GenericConfiguration
{
public:
	/** upsmon mode */
	typedef enum {
		UPSMON_UNDEF = 0,  /**< Unknown mode */
		UPSMON_PRIMARY,    /**< Primary   (legacy "Master") mode */
		UPSMON_SECONDARY,  /**< Secondary (legacy "Slave")  mode */
	} upsmon_mode_t;

	/** User-specific configuration attributes getters and setters \{ */

	inline std::string getPassword(const std::string & user) const { return getStr(user, "password"); }

	/** Currently valid actions include "SET" and "FSD",
	 *  but the method does not constrain the values */
	inline ConfigParamList getActions(const std::string & user) const
	{
		ConfigParamList actions;
		get(user, "actions", actions);
		return actions;
	}

	/** Valid commands are "ALL" or a list of specific commands
	 *  supported by the device (NUT driver dependent) */
	inline ConfigParamList getInstantCommands(const std::string & user) const
	{
		ConfigParamList cmds;
		get(user, "instcmds", cmds);
		return cmds;
	}

	upsmon_mode_t getUpsmonMode() const;

	inline void setPassword(const std::string & user, const std::string & passwd) { setStr(user, "password", passwd); }

	inline void setActions(const std::string & user, const ConfigParamList & actions)      { set(user, "actions",  actions); }
	inline void setInstantCommands(const std::string & user, const ConfigParamList & cmds) { set(user, "instcmds", cmds); }

	inline void addActions(const std::string & user, const ConfigParamList & actions)      { add(user, "actions",  actions); }
	inline void addInstantCommands(const std::string & user, const ConfigParamList & cmds) { add(user, "instcmds", cmds); }

	/**
	 *  \brief  upsmon mode setter
	 *
	 *  Note that the UPSMON_UNDEF mode isn't allowed as parameter
	 *  (logically, if you set something, it shall be defined...)
	 *
	 *  \param  mode  Mode
	 */
	/* TOTHINK: Do we need a writer (other method, optional parameter
	 * to this one) for obsolete wordings of the upsmon mode?
	 * Note: reader in the getter accepts both old and new values. */
	void setUpsmonMode(upsmon_mode_t mode);

	/** \} */

	/** Serialisable interface implementation overload \{ */
	bool parseFrom(NutStream & istream) override;
	bool writeTo(NutStream & ostream) const override;
	/** \} */

};  // end of class UpsdUsersConfiguration

} /* namespace nut */
#endif /* __cplusplus */
#endif	/* NUTCONF_H_SEEN */
