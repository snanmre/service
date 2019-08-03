#ifndef SERIALIZABLE_H_
#define SERIALIZABLE_H_

#include "Bundle.h"

/**
 * @author Sinan Emre Kutlu
 *
 * @date 2013/08/29
 */
class Serializable
{
public:
	/**
	 * @brief Virtual destructor
	 */
	virtual ~Serializable()
	{
	}

	/**
	 * @brief Fill bundle with the object content
	 * @param bundle	bundle to fill
	 */
	virtual void writeToBundle(Bundle & bundle) const = 0;

	/**
	 * @brief Read object content from bundle in the order of writing to the bundle
	 * @param bundle	bundle to read
	 */
	virtual void readFromBundle(Bundle & bundle) = 0;
};

#endif /* SERIALIZABLE_H_ */
