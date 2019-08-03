/*
 * Bundle.cpp
 *
 *  Created on: 22 Ağu 2013
 *      Author: root
 */

#include "serializer/Bundle.h"

#include <stdexcept>
#include <iostream>
#include <sstream>

using namespace std;

#include "serializer/Serializable.h"

Bundle::Bundle() :
		wind(0), rind(0)
{
}

Bundle::Bundle(const unsigned char* buf, const int size)
{
	if (!importData(buf, size))
		throw std::invalid_argument(
				string(__PRETTY_FUNCTION__) + " : buf is in invalid format.");
}

Bundle::Bundle(const Bundle & bundle) :
		buf(bundle.buf), wind(bundle.wind), rind(bundle.rind)
{
}

Bundle::~Bundle()
{

}

const Bundle & Bundle::operator =(const Bundle & bundle)
{
	this->buf = bundle.buf;
	this->wind = bundle.wind;
	this->rind = bundle.rind;

	return *this;
}

void Bundle::putInt(const int& i)
{
	buf[wind++] = TYPE_INT;

	buf[wind++] = 0x00;
	buf[wind++] = 0x01;

	Bundle::put(i);
}

void Bundle::putChar(const char& c)
{
	buf[wind++] = TYPE_CHAR;

	buf[wind++] = 0x00;
	buf[wind++] = 0x01;

	Bundle::put(c);
}

void Bundle::putBool(const bool& b)
{
	buf[wind++] = TYPE_BOOL;

	buf[wind++] = 0x00;
	buf[wind++] = 0x01;

	Bundle::put(b);
}

void Bundle::putDouble(const double& d)
{
	buf[wind++] = TYPE_DOUBLE;

	buf[wind++] = 0x00;
	buf[wind++] = 0x01;

	Bundle::put(d);
}

void Bundle::putString(const string& s)
{
	buf[wind++] = TYPE_STRING;

	buf[wind++] = 0x00;
	buf[wind++] = 0x01;

	Bundle::put(s);
}

void Bundle::putBundle(const Bundle& b)
{
	for (int i = 0; i < b.wind; ++i)
		buf[wind++] = b.buf[i];
}

void Bundle::putSerializable(const Serializable & s)
{
	s.writeToBundle(*this);
}

void Bundle::clear()
{
	wind = 0;
	rind = 0;
	buf.resize();
}

int Bundle::getInt()
{
	int i;

	get(i);

	return i;
}

char Bundle::getChar()
{
	char c = 0;

	get(c);

	return c;
}

bool Bundle::getBool()
{
	bool b = false;

	get(b);

	return b;
}

double Bundle::getDouble()
{
	double d;

	get(d);

	return d;
}

string Bundle::getString()
{
	string s;

	get(s);

	return s;
}

void Bundle::getSerializable(Serializable& s)
{
	s.readFromBundle(*this);
}

/* Stream Insertion Operators */

Bundle& Bundle::operator <<(const int& i)
{
	putInt(i);
	return *this;
}

Bundle& Bundle::operator <<(const char& i)
{
	putChar(i);
	return *this;
}

Bundle& Bundle::operator <<(const bool& i)
{
	putBool(i);
	return *this;
}

Bundle& Bundle::operator <<(const double& i)
{
	putDouble(i);
	return *this;
}

Bundle& Bundle::operator <<(const string& i)
{
	putString(i);
	return *this;
}

Bundle & Bundle::operator <<(const char * i)
{
	putString(i);
	return *this;
}

Bundle& Bundle::operator <<(const Bundle& i)
{
	putBundle(i);
	return *this;
}

Bundle& Bundle::operator <<(const Serializable& s)
{
	putSerializable(s);
	return *this;
}

/* Stream Extraction Operators */

Bundle& Bundle::operator >>(int& i)
{
	i = getInt();
	return *this;
}

Bundle& Bundle::operator >>(char& i)
{
	i = getChar();
	return *this;
}

Bundle& Bundle::operator >>(bool& i)
{
	i = getBool();
	return *this;
}

Bundle& Bundle::operator >>(double& i)
{
	i = getDouble();
	return *this;
}

Bundle& Bundle::operator >>(string& i)
{
	i = getString();
	return *this;
}

Bundle& Bundle::operator >>(Serializable& s)
{
	getSerializable(s);
	return *this;
}

int Bundle::getArray(string *& arr)
{
	unsigned char type = buf[rind++];

	if (type != TYPE_STRING)
		throw std::invalid_argument(
				string(__func__) + " Invalid argument type");

	uint count, length;

	count = (buf[rind] << 8) | buf[rind + 1];
	rind += 2;

	arr = new string[count];

	for (uint i = 0; i < count; ++i)
	{
		length = (buf[rind] << 8) | buf[rind + 1];
		rind += 2;

		arr[i].resize(length);

		for (size_t j = 0; j < length; ++j)
			arr[i][j] = buf[rind++];

//		cerr << "arr[" << i << "] : " << vec[i] << endl;
	}

	// reset indices
	if (wind == rind)
		wind = rind = 0;

	return count;
}

void Bundle::getArray(vector<string>& vec)
{
	unsigned char type = buf[rind];
	if (type != TYPE_STRING)
		throw std::invalid_argument("Invalid argument type");

	++rind;

	uint count, length;

	count = (buf[rind] << 8) | buf[rind + 1];
	rind += 2;

	vec.resize(count);

	for (uint i = 0; i < count; ++i)
	{
		length = (buf[rind] << 8) | buf[rind + 1];
		rind += 2;

		vec[i].resize(length);

		for (size_t j = 0; j < length; ++j)
			vec[i][j] = buf[rind++];

//		cerr << "arr[" << i << "] : " << vec[i] << endl;
	}

	// reset indices
	if (wind == rind)
		wind = rind = 0;
}

bool Bundle::importData(const unsigned char* buf, const int size)
{
	if (Bundle::checkValidity(buf, size) < 0)
		return false;

	rind = 0;
	wind = size;

	this->buf.resize(size);

	for (int i = 0; i < size; ++i)
		this->buf[i] = buf[i];

	return true;
}

int Bundle::exportData(unsigned char* buf, const int size) const
{
	if (size < wind - rind)
		return -1;

	int i = 0;
	for (int j = rind; j < wind; ++i, ++j)
		buf[i] = this->buf[j];

	return i;
}

void Bundle::print() const
{
	cerr << (wind - rind) << " | ";
	for (int i = rind; i < wind; ++i)
	{
		cerr << " " << (int) buf[i];
	}
	cerr << endl;
}

// utility functions
template<typename T>
ostream & operator<<(ostream & o, const vector<T> & v)
{
	for (size_t i = 0; i < v.size(); ++i)
		o << v[i] << endl;

	return o;
}

ostream & operator<<(ostream & o, const vector<bool> & v)
{
	for (size_t i = 0; i < v.size(); ++i)
		o << (v[i] ? "true" : "false") << endl;

	return o;
}
//ostream & operator<<(ostream & o, const vector<char> & v)
//{
//	for (size_t i = 0 ; i < v.size() ; ++i)
//		o << "\'" << v[i] << "\'" << endl;
//
//	return o;
//}

void Bundle::printElements() const
{
	cout << this->toString();
}

string Bundle::toString() const
{
	Bundle b = *this;
	vector<bool> vbool;
	vector<char> vchar;
	vector<double> vdouble;
	vector<int> vint;
	vector<string> vstring;
	stringstream s_str;

	int count = b.count();
	for (int i = 0; i < count; ++i)
	{
		switch (b.getNextType())
		{
		case Bundle::TYPE_BOOL:
//			cout << "TYPE_BOOL:" << endl;
			b.getArray(vbool);
			s_str << vbool;
			break;

		case Bundle::TYPE_CHAR:
//			cout << "TYPE_CHAR:" << endl;
			b.getArray(vchar);
			s_str << vchar;
			break;

		case Bundle::TYPE_DOUBLE:
//			cout << "TYPE_DOUBLE:" << endl;
			b.getArray(vdouble);
			s_str << vdouble;
			break;

		case Bundle::TYPE_INT:
//			cout << "TYPE_INT:" << endl;
			b.getArray(vint);
			s_str << vint;
			break;

		case Bundle::TYPE_STRING:
//			cout << "TYPE_STRING:" << endl;
			b.getArray(vstring);
			s_str << vstring;
			break;

		case Bundle::TYPE_UNDEF:
//			cerr << "\tUNDEFINED RESPONSE" << endl;
			break;

		default:
//			cerr << "\tUNRECOGNIZED RESPONSE" << endl;
			break;
		}
	}

	return s_str.str();
}

int Bundle::count() const
{
	int _count = checkValidity(((const unsigned char *) this->buf) + rind,
			wind - rind);
	return _count;
}

int Bundle::byteCount() const
{
	return wind - rind;
}

unsigned char Bundle::getNextType() const
{
	if (count() <= 0)
		return TYPE_UNDEF;

	return buf[rind];
}

unsigned char Bundle::getType(const std::type_info & ti)
{
	if (ti == typeid(int) || ti == typeid(unsigned int))
		return TYPE_INT;

	if (ti == typeid(char) || ti == typeid(unsigned char))
		return TYPE_CHAR;

	if (ti == typeid(bool))
		return TYPE_BOOL;

	if (ti == typeid(double) || ti == typeid(float))
		return TYPE_DOUBLE;

	if (ti == typeid(string))
		return TYPE_STRING;

	return 0;
}

int Bundle::checkValidity(const unsigned char * buf, const int size)
{
	int elementCount = 0;

	int i = 0;
	unsigned char type;
	uint count, length;

	while (i < size)
	{
		// type
		type = buf[i++];
		if (type == 0 || type >= TYPE_UNDEF)
		{
			elementCount = -1;
//			cerr << __PRETTY_FUNCTION__ << " TYPE UNDEFINED  " << i << endl;
			break;
		}

		// count
		count = (buf[i] << 8) | buf[i + 1];
		i += 2;

		// length + data
		for (uint j = 0; j < count; ++j)
		{
			length = (buf[i] << 8) | buf[i + 1];
			i += 2;

			i += length;

			if (i > size)
			{
				elementCount = -2;
//				cerr << __PRETTY_FUNCTION__ << " LENGTH + DATA       " << i << " >= " << size << endl;
				break;
			}
		} // end-of-for

		++elementCount;

	} // end-of-while

	return elementCount;
}

void Bundle::put(const string & s)
{
	buf[wind++] = (s.length() >> 8) & 0xFF;
	buf[wind++] = s.length() & 0xFF;

	// data
	for (size_t i = 0; i < s.length(); ++i)
		buf[wind++] = s[i];
}

void Bundle::get(string & s)
{
	unsigned char type = buf[rind++];
	uint count, length;

	count = (buf[rind] << 8) | buf[rind + 1];
	rind += 2;

	length = (buf[rind] << 8) | buf[rind + 1];
	rind += 2;

	if (type != TYPE_STRING || count != 1)
		throw std::runtime_error(
				string(__PRETTY_FUNCTION__) + " Invalid operation");

	s.resize(length);

	for (uint i = 0; i < length; ++i)
		s[i] = buf[rind++];

	// reset indices
	if (wind == rind)
		wind = rind = 0;

	rearrange();
}

void Bundle::rearrange()
{
	if (rind < 256)
		return;

//	DD("arranging... : %d   %d\n", rind, wind);

	for (int i = rind, j = 0; i < wind; ++i, ++j)
		buf[j] = buf[i];

	wind -= rind;
	rind = 0;

//	DD("arranged.... : %d   %d\n", rind, wind);
}
