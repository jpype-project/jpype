#include <jp_encoding.h>

JPEncoding::~JPEncoding()
{
}

// char* to stream from 
// https://stackoverflow.com/questions/7781898/get-an-istream-from-a-char

struct membuf : std::streambuf
{

	membuf(char* begin, char* end)
	{
		this->setg(begin, begin, end);
	}
};

std::string transcribe(const char* in, size_t len,
		const JPEncoding& sourceEncoding,
		const JPEncoding& targetEncoding)
{
	// Convert input to istream source
	membuf sbuf(const_cast<char*> (in), const_cast<char*> (in + len));
	std::istream inStream(&sbuf);

	// Create an output stream with reserve
	std::string out;
	out.reserve(len + 8);
	std::ostringstream outStream(out);

	while (!inStream.eof())
	{
		unsigned int coding = sourceEncoding.fetch(inStream);
		if (coding == (unsigned int) - 1)
		{
			// Truncate bad strings for now.
			return outStream.str();
		}

		targetEncoding.encode(outStream, coding);
	}
	return outStream.str();
}

//**************************************************************

void JPEncodingUTF8::encode(std::ostream& out, unsigned int c) const
{
	if (c < 0x80)
	{
		// encode 1
		out.put(char(c & 0xff));
	}
	else if (c < 0x800)
	{
		// encode 2
		out.put(char(0xc0 + ((c >> 6)&0x1f)));
		out.put(char(0x80 + ((c >> 0)&0x3f)));
	}
	else if (c < 0x10000) // Note 0xd800-0xdfff are not valid codes
	{
		// encode 3
		out.put(char(0xe0 + ((c >> 12)&0x0f)));
		out.put(char(0x80 + ((c >> 6)&0x3f)));
		out.put(char(0x80 + ((c >> 0)&0x3f)));
	}
	else if (c < 0x110000)
	{
		// encode 4
		out.put(char(0xf0 + ((c >> 18)&0x07)));
		out.put(char(0x80 + ((c >> 12)&0x3f)));
		out.put(char(0x80 + ((c >> 6)&0x3f)));
		out.put(char(0x80 + ((c >> 0)&0x3f)));
	}
}

unsigned int JPEncodingUTF8::fetch(std::istream& in) const
{
	if (in.eof()) return -1;
	unsigned int c0 = in.get();

	// 1 byte code
	if ((c0 & 0x80) == 0)
		return c0;

	if (in.eof()) return -1;
	unsigned int c1 = in.get();

	// 2 byte code
	if ((c0 & 0xe0) == 0xc0)
	{
		if ((c1 & 0xc0) == 0x80)
			return ((c0 & 0x1f) << 6) + (c1 & 0x3f);
		else
			return -1; // bad 2 byte format
	}

	if (in.eof()) return -1;
	unsigned int c2 = in.get();

	// 3 byte code
	if ((c0 & 0xf0) == 0x80)
	{
		if ((c1 & 0xc0) == 0x80 && (c2 & 0xc0) == 0x80)
			return ((c0 & 0xf) << 12) + ((c1 & 0x3f) << 6) + (c2 & 0x3f);
		return -1;
	}

	if (in.eof()) return -1;
	unsigned int c3 = in.get();

	// 4 byte code
	if ((c0 & 0xf8) == 0xf0)
	{
		if ((c1 & 0xc0) == 0x80 && (c2 & 0xc0) == 0x80 && (c3 & 0xc0) == 0x80)
			return ((c0 & 0xf) << 18) + ((c1 & 0x3f) << 12) + ((c2 & 0x3f) << 6) + (c3 & 0x3f);
		return -1;
	}
	return -1;
}

//*****************************************************************
//
// To the Sun programmer who wrote GetStringUTFchars() that thought it was a
// good idea to return UTF-16 embedded in UTF-8, you sir are an embarrassment.
// 
// You had one job here, to take an internal representation in UTF16 and produce
// a stream of chars that could be read by the outside world.  It had to be 
// translated as those encodings are incompatible and rather that writing
// to a defined format, you simply lazily packed each UTF-16 into UTF-8 resulting 
// in surrogate codes in UTF-8.  It would have taken you all of 20 extra minutes
// to do it right and you failed. 
//
// Any time saving that you got by doing this in a lazy fashion was lost when 
// the second programmer besides yourself had to write a translation code.  And
// all efficiency gains that thought were made have been lost in the 
// memory management and retranslation that anyone would need to make use of your
// U+1F4A9.  You are solely responsible for wasting thousands of hours of other 
// programmer's lives and promoting the heat death of the universe.
//
// And in case you feel that documentation makes it all okay, to top everything off
// your documentation is incorrect.  You list encodings between U+0800 and U+FFFF 
// as being the 3 byte encoding, but the document fails to mention that U+D800 to U+DFFF
// are not a valid encoding.  Thus leaving any coder to struggle to 
// figure out how there would not be a conflict between coding 3 and coding 4 without
// consulting an additional source.
//
// You are an embarrassment to the craft.  Shame on you, shame on your family,
// and shame on the goat that hauled your sorry butt into work each day.

void JPEncodingJavaUTF8::encode(std::ostream& out, unsigned int c) const
{
	if (c == 0)
	{
		// encode 0 as 2
		out.put(char(0xc0));
		out.put(char(0x80));
	}
	else if (c < 0x80)
	{
		// encode 1
		out.put(char(c & 0xff));
	}
	else if (c < 0x800)
	{
		// encode 2
		out.put(char(0xc0 + ((c >> 6)&0x1f)));
		out.put(char(0x80 + ((c >> 0)&0x3f)));
	}
	else if (c < 0xd800 || (c >= 0xe000 && c < 0x10000))
	{
		// encode 3
		out.put(char(0xe0 + char((c >> 12)&0x0f)));
		out.put(char(0x80 + char((c >> 6)&0x3f)));
		out.put(char(0x80 + char((c >> 0)&0x3f)));
	}
	else if (c < 0x110000)
	{
		c = c - 0x10000;

		// encode 6
		out.put(char(0xed));
		out.put(char(0x80 + ((c >> 16)&0xf)));
		out.put(char(0x80 + ((c >> 10)&0x3f)));
		out.put(char(0xed));
		out.put(char(0x80 + ((c >> 6)&0x0f)));
		out.put(char(0x80 + ((c >> 0)&0x3f)));
	}
}

unsigned int JPEncodingJavaUTF8::fetch(std::istream& in) const
{
	if (in.eof()) return -1;
	unsigned int c0 = in.get();

	if ((c0 & 0x80) == 0)
		return c0;

	if (in.eof()) return -1;
	unsigned int c1 = in.get();

	if ((c0 & 0xe0) == 0xc0)
	{
		if ((c1 & 0xc0) == 0x80)
			return ((c0 & 0x1f) << 6) + (c1 & 0x3f);
		else
			return -1; // bad 2 byte format
	}

	if (in.eof()) return -1;
	unsigned int c2 = in.get();

	if ((c0 & 0xf0) == 0x80 && (c1 & 0xc0) == 0x80 && (c2 & 0xc0) == 0x80)
	{
		unsigned int out = ((c0 & 0xf) << 12) + ((c1 & 0x3f) << 6) + (c2 & 0x3f);
		// We can only hold up to 0xd7ff
		// Surrogate code 2 pairs of 3.
		if ((out & 0xf300) == 0xd800)
		{
			unsigned int next = fetch(in);
			if (next == (unsigned int) - 1)
				return -1;

			if ((next & 0xf300) == 0xd800)
			{
				unsigned int q1 = (out & 0x3ff);
				unsigned int q2 = (next & 0x3ff);
				return 0x10000 + (q1 << 10) + q2;
			}
			return -1;
		}
		return out;
	}
	return -1;
}

