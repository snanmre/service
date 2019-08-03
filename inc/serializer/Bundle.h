#ifndef BUNDLE_H_
#define BUNDLE_H_

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "AutoResizingBuf.h"

class Serializable;

/**
 * @author Sinan Emre Kutlu
 *
 * @date 2013/08/29
 *
 * @brief This class is used to serialize primitive types, strings, objects etc.
 * Main purpose is to provide a communication channel between processes via DomainSocket class.
 *
 * Buffer:  [T] [C] [L1][D1] [L2][D2] [L3][D3]...
 *
 * [T] : Type							(1 byte)
 * [C] : Count							(2 bytes)
 * [L] : Length of following data		(2 bytes)
 * [D] : Data							(n bytes)
 *
 */
class Bundle
{
public:
	/**
	 * @brief Default constructor
	 */
	Bundle();

	/**
	 * @brief Create a bundle and import data
	 * @param buf		source buffer
	 * @param size		buffer size
	 * @see Bundle::importData
	 */
	Bundle(const unsigned char * buf, const int size);

	/**
	 * @brief Copy constructor
	 * @param bundle	source object
	 */
	Bundle(const Bundle & bundle);

	/**
	 * @brief Virtual destructor
	 */
	virtual ~Bundle();

	/**
	 * @brief assignment operator
	 */
	const Bundle & operator =(const Bundle & bundle);

	/**
	 * @brief put integer into bundle
	 */
	void putInt(const int &i);

	/**
	 * @brief put char into bundle
	 */
	void putChar(const char &c);

	/**
	 * @brief put bool into bundle
	 */
	void putBool(const bool &b);

	/**
	 * @brief put double into bundle
	 */
	void putDouble(const double &d);

	/**
	 * @brief put string into bundle
	 */
	void putString(const std::string &s);

	/**
	 * @brief put bundle into bundle
	 */
	void putBundle(const Bundle &b);

	/**
	 * @brief put serializable into bundle
	 * @param s
	 */
	void putSerializable(const Serializable &s);

	/**
	 * @brief put array into bundle
	 * @param arr		source array
	 * @param size		array size
	 */
	template<typename T>
	void putArray(const T * arr, const int size)
	{
		unsigned char type = Bundle::getType(typeid(T));

		if (type == 0)
		{
			throw std::invalid_argument("Invalid argument type");
		}

		buf[wind++] = type;

		buf[wind++] = (size >> 8) & 0xFF;
		buf[wind++] = size & 0xFF;

		for (int i = 0; i < size; ++i)
			Bundle::put(arr[i]);
	}

	/**
	 * @brief put array into bundle
	 * @param vec		source vector
	 */
	template<typename T>
	void putArray(const std::vector<T> & vec)
	{
		unsigned char type = Bundle::getType(typeid(T));

		if (type == 0)
		{
			throw std::invalid_argument("Invalid argument type");
		}

		buf[wind++] = type;

		buf[wind++] = (vec.size() >> 8) & 0xFF;
		buf[wind++] = vec.size() & 0xFF;

		for (size_t i = 0; i < vec.size(); ++i)
			Bundle::put(vec[i]);
	}

	/*
	 * Overloading stream insertion operators for types above.
	 */
	Bundle & operator <<(const int &i);
	Bundle & operator <<(const char &i);
	Bundle & operator <<(const bool &i);
	Bundle & operator <<(const double &i);
	Bundle & operator <<(const std::string &i);
	Bundle & operator <<(const char * i);
	Bundle & operator <<(const Bundle &i);
	Bundle & operator <<(const Serializable &s);

	template<typename T>
	Bundle & operator <<(const std::vector<T> & vec)
	{
		putArray(vec);
		return *this;
	}

	/**
	 * @brief Clear bundle content
	 */
	void clear();

	/**
	 * @brief Get integer from bundle
	 */
	int getInt();

	/**
	 * @brief Get char from bundle
	 */
	char getChar();

	/**
	 * @brief Get bool from bundle
	 */
	bool getBool();

	/**
	 * @brief Get double from bundle
	 */
	double getDouble();

	/**
	 * @brief Get string from bundle
	 */
	std::string getString();

	/**
	 * @brief Get serializable object from bundle
	 */
	void getSerializable(Serializable &s);

	/*
	 * Overloading stream extraction operators for types above.
	 */
	Bundle & operator >>(int &i);
	Bundle & operator >>(char &i);
	Bundle & operator >>(bool &i);
	Bundle & operator >>(double &i);
	Bundle & operator >>(std::string &i);
	Bundle & operator >>(Serializable &s);

	template<typename T>
	Bundle & operator >>(std::vector<T> & vec)
	{
		getArray(vec);
		return *this;
	}

	/**
	 * @brief Get array from bundle
	 * @param arr	pointer to get array(not allocated)
	 * @return		element count
	 */
	template<typename T>
	int getArray(T *& arr)
	{
		unsigned char type = buf[rind++];

		if (type != Bundle::getType(typeid(T)))
			throw std::invalid_argument("Invalid argument type");

		uint count, len;

		count = (buf[rind] << 8) | buf[rind + 1];
		rind += 2;

		arr = new T[count];

		for (int i = 0; i < count; ++i)
		{
			len = (buf[rind] << 8) | buf[rind + 1];
			rind += 2;

			if (len != sizeof(T))
			{
				throw std::runtime_error("Invalid length");
			}

			memcpy(&(arr[i]), &(((unsigned char *) buf)[rind]), sizeof(T));
			rind += sizeof(T);
//			cerr << "arr[" << i << "] : " << vec[i] << endl;
		}

		// reset indices
		if (wind == rind)
			wind = rind = 0;

		return count;
	}

	/**
	 * @brief Get string array from bundle
	 * @param arr	pointer to get array(not allocated)
	 * @return		element count
	 */
	int getArray(std::string *& arr);

	/**
	 * @brief Get array from bundle into vector
	 * @param vec	destination vector
	 */
	template<typename T>
	void getArray(std::vector<T> & vec)
	{
		unsigned char type = buf[rind++];

		if (type != Bundle::getType(typeid(T)))
			throw std::invalid_argument("Invalid argument type");

		uint count, len;
		T elem;

		count = (buf[rind] << 8) | buf[rind + 1];
		rind += 2;

		vec.resize(count);

		for (uint i = 0; i < count; ++i)
		{
			len = (buf[rind] << 8) | buf[rind + 1];
			rind += 2;

			if (len != sizeof(T))
			{
//				cerr << "len : " << len << "     sizeof(T) = " << sizeof(T) << endl;
				throw std::runtime_error("invalid length");
			}

			memcpy(&elem, &(((unsigned char *) buf)[rind]), sizeof(T));
			vec[i] = elem;

			rind += sizeof(T);
//			cerr << "arr[" << i << "] : " << vec[i] << endl;
		}

		// reset indices
		if (wind == rind)
			wind = rind = 0;
	}

	/**
	 * @brief Get string array from bundle into vector
	 * @param vec	destination vector
	 */
	void getArray(std::vector<std::string> & vec);

	/**
	 * @brief Import bundle-like formatted buffer
	 * @param buf	source buffer
	 * @param size	buffer size
	 * @return		true if successfully done, otherwise false
	 */
	bool importData(const unsigned char * buf, const int size);

	/**
	 * @brief Export formatted data into given buffer
	 * @param buf	destination buffer
	 * @param size	buffer size
	 * @return		data length placed into the buffer, otherwise a negative error code
	 */
	int exportData(unsigned char * buf, const int size) const;

	/**
	 * @brief Print content to the stderr as a byte order
	 * @warning use for debug purpose
	 */
	void print() const;

	/**
	 * @brief Print elements in the human readable form
	 */
	void printElements() const;

	/**
	 * @brief Get string representation (new-lines between elements)
	 * @return		string representation
	 */
	std::string toString() const;

	/**
	 * @brief Element count in bundle
	 * @return		element count
	 */
	int count() const;

	/**
	 * @brief bytes count in bundle raw data
	 * @return		# of bytes
	 */
	int byteCount() const;

	/**
	 * @brief Get next element type
	 * @return		next element type if available, otherwise TYPE_UNDEF
	 */
	unsigned char getNextType() const;

	const static unsigned char TYPE_INT = 0x01;			// singular data
	const static unsigned char TYPE_CHAR = 0x02;
	const static unsigned char TYPE_BOOL = 0x03;
	const static unsigned char TYPE_DOUBLE = 0x04;
	const static unsigned char TYPE_STRING = 0x05;
	const static unsigned char TYPE_UNDEF = 0x06;		// must be the largest

protected:

	/**
	 * @brief Get type id of given type info
	 * @param ti	type info
	 * @return		type id or 0
	 */
	static unsigned char getType(const std::type_info & ti);

	/**
	 * @brief Check whether given buffer is formatted like a bundle
	 * @param buf	source buffer
	 * @param size	buffer size
	 * @return		element count if it is valid, otherwise negative error code
	 */
	static int checkValidity(const unsigned char * buf, const int size);

	/**
	 * @brief Push argument's length and content into buffer
	 * @param t		source variable
	 */
	template<typename T>
	void put(const T & t)
	{
		// length
		size_t size = sizeof(T);
		buf[wind++] = (size >> 8) & 0xFF;
		buf[wind++] = size & 0xFF;

		// resize buffer
		buf[wind + sizeof(T)] = 0;

		// data
		memcpy(&(((unsigned char *) buf)[wind]), &t, sizeof(T));

		wind += sizeof(T);
	}

	/**
	 * @brief Push argument's length and content into buffer
	 * @param s		source string
	 */
	void put(const std::string & s);

	/**
	 * @brief Pop element from the buffer
	 * @param t		destination variable
	 */
	template<typename T>
	void get(T & t)
	{
		unsigned char type = buf[rind++];
		ushort count, length;

		count = (buf[rind] << 8) | buf[rind + 1];
		rind += 2;

		length = (buf[rind] << 8) | buf[rind + 1];
		rind += 2;

		unsigned char argType = getType(typeid(T));

		if (type != argType || count != 1 || length != sizeof(T))
			throw std::runtime_error("invalid operation");

		switch (argType)
		{
		case TYPE_INT:
		case TYPE_CHAR:
		case TYPE_DOUBLE:
			memcpy(&t, &(((unsigned char *) buf)[rind]), sizeof(T));
			rind += sizeof(T);
			break;

		case TYPE_BOOL:
			t = (buf[rind++] != 0);
			break;
		}

		// reset indices
		if (wind == rind)
			wind = rind = 0;

		rearrange();
	}

	/**
	 * @brief Pop string element from the buffer
	 * @param s		destination string
	 */
	void get(std::string & s);

	/**
	 * @brief Re-arrange the buffer to reduce memory usage
	 */
	void rearrange();

	AutoResizingBuf<unsigned char> buf;
	int wind; /** write index */
	int rind; /** read index */
};

#endif /* BUNDLE_H_ */
