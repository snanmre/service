#ifndef AUTORESIZINGBUF_H_
#define AUTORESIZINGBUF_H_

#include <stdexcept>

/**
 * @author Sinan Emre Kutlu
 *
 * @date 2013/08/22
 *
 * @brief This class can be used as infinite array
 *
 */
template<class T>
class AutoResizingBuf
{
#define MIN(a,b) ((a)<(b)?(a):(b))

public:
	/**
	 * @brief Create a AutoBuf object with capacity
	 * @param capacity		initial capacity
	 */
	AutoResizingBuf(const int capacity = step) :
			capacity(capacity), buf(new T[capacity])
	{
	}

	/**
	 * @brief Copy constructor
	 */
	AutoResizingBuf(const AutoResizingBuf<T> &autoBuf) :
			capacity(autoBuf.capacity)
	{
		buf = new T[capacity];

		for (int i = 0; i < capacity; ++i)
			buf[i] = autoBuf.buf[i];
	}

	/**
	 * @brief Virtual destructor
	 */
	virtual ~AutoResizingBuf()
	{
		delete[] buf;
	}

	/**
	 * @brief Assignment operator
	 * @param autoBuf				AutoBuf object
	 * @return						this object
	 */
	const AutoResizingBuf & operator =(const AutoResizingBuf<T> &autoBuf)
	{
		capacity = autoBuf.capacity;

		delete[] buf;

		buf = new T[capacity];

		for (int i = 0; i < capacity; ++i)
			buf[i] = autoBuf.buf[i];

		return *this;
	}

	/**
	 * @brief Subscript operator
	 * @param index				index
	 * @return					element at index \a index
	 */
	T & operator[](const int index)
	{
		// if index larger than the capacity, then resize buf
		if (index >= capacity)
		{
			int new_capacity = (index / step + 1) * step;

			resize(new_capacity);
		}

		return buf[index];
	}

	/**
	 * @brief Subscript operator for const objects
	 * @param index				index
	 * @return					element at index \a index
	 */
	const T & operator[](const int index) const
	{
		if (index >= capacity)
			throw std::runtime_error("AutoBuf: index out of range");

		return buf[index];
	}

	/**
	 * @brief T * cast operator
	 */
	operator T *()
	{
		return (this->buf);
	}

	/**
	 * @brief const T * cast operator
	 */
	operator T *() const
	{
		return (this->buf);
	}

	/**
	 * @brief Resize buffer
	 * @param capacity			new capacity
	 */
	void resize(const int capacity = step)
	{
		if (capacity <= 0)
			throw std::invalid_argument(
					"AutoBuf: new capacity value cannot be less than zero.");

		if (this->capacity == capacity)
			return;

		// allocate new array
		T * tmp = new T[capacity];

		// copy content
		int copy_count = MIN(this->capacity, capacity);
		for (int i = 0; i < copy_count; ++i)
			tmp[i] = buf[i];

		// save new capacity
		this->capacity = capacity;

		// redirect to new array
		delete[] buf;
		buf = tmp;
	}

	/**
	 * @brief Buffer size
	 * @return					buffer size
	 */
	int size() const
	{
		return capacity;
	}

protected:

	/** @brief Buffer step size used at reallocation level */
	const static int step = 64;

	/** @brief Buffer size */
	int capacity;

	T * buf;
};

#endif /* AUTORESIZINGBUF_H_ */
