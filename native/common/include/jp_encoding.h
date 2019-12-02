#ifndef _JP_ENCODING_H_
#define _JP_ENCODING_H_

#include <iostream>
#include <string>
#include <sstream>

class JPEncoding
{
public:

	JPEncoding()
	{
	}
	virtual ~JPEncoding();

	/** Store a code point in an outgoing buffer. */
	virtual void encode(std::ostream& out, unsigned int codePoint) const = 0;

	/** Retrieve a coding point from an incoming buffer. */
	virtual unsigned int fetch(std::istream& is) const = 0;
} ;

class JPEncodingUTF8 : public JPEncoding
{
public:
	/** Store a code point in an outgoing buffer. */
	void encode(std::ostream& out, unsigned int codePoint) const override;

	/** Retrieve a coding point from an incoming buffer. */
	unsigned int fetch(std::istream& is) const override;
} ;

class JPEncodingJavaUTF8 : public JPEncoding
{
public:
	/** Store a code point in an outgoing buffer. */
	void encode(std::ostream& out, unsigned int codePoint) const override;

	/** Retrieve a coding point from an incoming buffer.
	 * returns the next coding point or -1 on invalid coding.
	 */
	unsigned int fetch(std::istream& is) const override;
} ;

std::string transcribe(const char* in, size_t len,
		const JPEncoding& sourceEncoding,
		const JPEncoding& targetEncoding);

#endif // _JP_ENCODING_H_