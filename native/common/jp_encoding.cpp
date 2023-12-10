/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#include "jp_encoding.h"

// These encoders handle all the codes expected to be passed between
// Java and Python assuming they both generate compliant codings.  However,
// this code does not handle miscodings very well.  The current behavior
// is to simply terminate the string at the bad code without producing
// a warning.  I can add errors, but I am not sure how I would test it
// as both java and python will produce an error if I tried to force a
// bad encoding on them.  Thus I am going to leave this with the truncation
// behavior for now.
//
// Examples of bad encodings:
//   - non-utf8 such as extended ascii passed from python
//   - encoded forbidden utf passed from python
//   - truncated surrogate codes passed from java
//
// The secondary problem is that string conversion is part of exception
// handling.  And the last thing we want to do is throw an exception
// while trying to convert an exception string while reporting.  That
// would completely redirect the user.  Thus truncation seems like
// a sane policy unless we can verify all the potential edge cases.
//
// Alternatively we could make them a bit more permissive and
// try to automatically correct bad encodings by recognizing
// extend ASCII, etc.  But this is potentially complex and
// has the downside that we can't test it currently.

JPEncoding::~JPEncoding()
= default;

// char* to stream from
// https://stackoverflow.com/questions/7781898/get-an-istream-from-a-char

struct membuf : std::streambuf
{

	membuf(char* begin, char* end)
	{
		setg(begin, begin, end);
	}
} ;

// Convert a string from one encoding to another.
// Currently we use this to transcribe from utf-8 to java-utf-8 and back.
// It could do other encodings, but would need to be generalized to
// a template to handle wider character sets.

std::string transcribe(const char* in, size_t len,
		const JPEncoding& sourceEncoding,
		const JPEncoding& targetEncoding)
{
	// ASCII bypass
	bool ascii = true;
	for (size_t i = 0; i < len; ++i)
	{
		if (in[i]&0x80 || in[i] == 0)
		{
			ascii = false;
			break;
		}
	}
	if (ascii)
	{
		return {in, len};
	}

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
			if (inStream.eof())
				break;
			// Truncate bad strings for now.
			return outStream.str();
		}

		targetEncoding.encode(outStream, coding);
	}
	return outStream.str();
}

//**************************************************************

// Encode a 21 bit code point as UTF-8
//
// There are 4 encodings used
//
// Range                     Encoding
// 0x000000-0x00007F
//                           0xxxxxxx
//                    bits    6543210
//
// 0x000080-0x0007FF
//                           110xxxxx  10xxxxxx
//                              1
//                    bits      09876    543210
//
// 0x000800-0x000FFFF (excluding 0xD800-0xDFFF)
//                           1110xxxx  10xxxxxx  10xxxxxx
//                               1111    11
//                    bits       5432    109876    543210
//
// 0x100000-0x10FFFF
//                           11110xxx  10xxxxxx  10xxxxxx  10xxxxxx
//                                211    111111    11
//                    bits        098    765432    109876    543210
//
// The encoding process is simply to add the constant portion encoding
// the byte position, then shifting to get the lowest bits, the masking
// off the required bits.
//

void JPEncodingUTF8::encode(std::ostream& out, unsigned int c) const
{
	if (c < 0x80)
	{
		// encode 1
		out.put(char(c & 0xff));
	} else if (c < 0x800)
	{
		// encode 2
		out.put(char(0xc0 + ((c >> 6)&0x1f)));
		out.put(char(0x80 + ((c >> 0)&0x3f)));
	} else if (c < 0x10000) // Note 0xd800-0xdfff are not valid codes
	{
		// encode 3
		out.put(char(0xe0 + ((c >> 12)&0x0f)));
		out.put(char(0x80 + ((c >> 6)&0x3f)));
		out.put(char(0x80 + ((c >> 0)&0x3f)));
	} else if (c < 0x110000)
	{
		// encode 4
		out.put(char(0xf0 + ((c >> 18)&0x07)));
		out.put(char(0x80 + ((c >> 12)&0x3f)));
		out.put(char(0x80 + ((c >> 6)&0x3f)));
		out.put(char(0x80 + ((c >> 0)&0x3f)));
	}
}

// Retrieve a 21 unicode code point from a stream of bytes.
//
// This is the reverse of the process of encoding.
// We must first locate the position encoding bits from the
// top of the byte, then pull off the encoded bits.  The
// final code point is the sum of each set of encoded bits.
//

unsigned int JPEncodingUTF8::fetch(std::istream& in) const
{
	unsigned int c0 = in.get();
	if (in.eof()) return -1;

	// 1 byte code
	if ((c0 & 0x80) == 0)
		return c0;

	unsigned int c1 = in.get();
	if (in.eof()) return -1;

	// 2 byte code
	if ((c0 & 0xe0) == 0xc0)
	{
		if ((c1 & 0xc0) == 0x80)
			return ((c0 & 0x1f) << 6) + (c1 & 0x3f);
		else
			return -1; // bad 2 byte format
	}

	unsigned int c2 = in.get();
	if (in.eof()) return -1;

	// 3 byte code
	if ((c0 & 0xf0) == 0xe0)
	{
		if ((c1 & 0xc0) == 0x80 && (c2 & 0xc0) == 0x80)
			return ((c0 & 0xf) << 12) + ((c1 & 0x3f) << 6) + (c2 & 0x3f);
		return -1;
	}

	unsigned int c3 = in.get();
	if (in.eof()) return -1;

	// 4 byte code
	if ((c0 & 0xf8) == 0xf0)
	{
		if ((c1 & 0xc0) == 0x80 && (c2 & 0xc0) == 0x80 && (c3 & 0xc0) == 0x80)
			return ((c0 & 0xf) << 18) + ((c1 & 0x3f) << 12) + ((c2 & 0x3f) << 6) + (c3 & 0x3f);
		return -1;
	}
	return -1;
}


// Encode a 21 code point into a Java UTF char stream.
//
// Java uses 16 bit characters to represent a string, but this is not
// sufficient to hold all 21 bits of valid unicode chars.  Thus to
// encode the upper bits java uses UTF-16 encoding internally.  Unfortunately,
// when java converts the 16 bit string into 8 bit encoding, it does not
// decode the string first.  As a result we receive not UTF-8 but rather
// doublely encoded UTF-16/UTF-8.  Further UTF-16 encoding requires
// not just simple byte packing but also an offset of the bits.  Thus this
// process becomes a real mess.  Also a special code point is required for
// embedded nulls.
//
// There are 4 encodings used
//
// Range                     Encoding
// 0x000001-0x00007F
//                           0xxxxxxx
//                    bits    6543210
//
// 0x000080-0x0007FF, special code 0
//                           110xxxxx  10xxxxxx
//                              1
//                    bits      09876    543210
//
// 0x000800-0x000D7FF or 0x00DFFF-0x000FFFF
//                           1110xxxx  10xxxxxx  10xxxxxx
//                               1111    11
//                    bits       5432    109876    543210
//
// 0x100000-0x10FFFF (called surgate codes in UTF-16)
// (doublely encoded 6 byte code point)
// first subtract 0x10000 which reduces the range by 2 bits
//                           11101101  1010xxxx  10xxxxxx  11101101  1011xxxx  10xxxxxx
//                                         1111    111111
//                    bits                 9876    543210                9876    543210
//
// The encoding process is simply to add the constant portion encoding
// the byte position, then shifting to get the lowest bits, the masking
// off the required bits. The exception being the surgate codes which
// require an offset then a bit pack.
//
// 4 byte unicode or coding a 3 point in the invalid range followed by anything other
// than 3 byte encoding (in the invalid range) is a coding error.
//

void JPEncodingJavaUTF8::encode(std::ostream& out, unsigned int c) const
{
	if (c == 0)
	{
		// encode 0 as 2
		out.put(char(0xc0));
		out.put(char(0x80));
	} else if (c < 0x80)
	{
		// encode 1
		out.put(char(c & 0xff));
	} else if (c < 0x800)
	{
		// encode 2
		out.put(char(0xc0 + ((c >> 6)&0x1f)));
		out.put(char(0x80 + ((c >> 0)&0x3f)));
	} else if (c < 0xd800 || (c >= 0xe000 && c < 0x10000))
	{
		// encode 3
		out.put(char(0xe0 + char((c >> 12)&0x0f)));
		out.put(char(0x80 + char((c >> 6)&0x3f)));
		out.put(char(0x80 + char((c >> 0)&0x3f)));
	} else if (c < 0x110000)
	{
		c = c - 0x10000;

		// encode 6
		out.put(char(0xed));
		out.put(char(0xa0 + ((c >> 16)&0xf)));
		out.put(char(0x80 + ((c >> 10)&0x3f)));
		out.put(char(0xed));
		out.put(char(0xb0 + ((c >> 6)&0xf)));
		out.put(char(0x80 + ((c >> 0)&0x3f)));
	}
}

// Decoding is the reverse of encoding, but there
// is a special gotcha because the position encoding bits
// are not unique.  Both the 3 and 6 points codes share
// the position encoding 0xED for the first byte.
//
// The unicde packing solves this by removing a portion of
// the encoding such that 0xD800-0xDFFF are not valid unicode code
// points.  Thus these invalid code points are used to represent
// the upper and lower encoding for the extended unicode
// code points.
//
// Thus the procedure is decode the top 3 and check to see if
// it is in a valid range.  If it isn't then decode the
// second set of 3.  So long as the bytes are coded through the
// java encoder it will be valid.  However, a user could
// manually encode a string with an invalid coding.
//

unsigned int JPEncodingJavaUTF8::fetch(std::istream& in) const
{
	unsigned int out = 0;
	unsigned int c0 = in.get();
	if (in.eof()) return -1;

	if ((c0 & 0x80) == 0)
		return c0;

	unsigned int c1 = in.get();
	if (in.eof()) return -1;

	if ((c0 & 0xe0) == 0xc0)
	{
		if ((c1 & 0xc0) == 0x80)
			return ((c0 & 0x1f) << 6) + (c1 & 0x3f);
		else
			return -1; // bad 2 byte format
	}

	unsigned int c2 = in.get();
	if (in.eof()) return -1;

	if ((c0 & 0xf0) == 0xe0 && (c1 & 0xc0) == 0x80 && (c2 & 0xc0) == 0x80)
	{
		out = ((c0 & 0xf) << 12) + ((c1 & 0x3f) << 6) + (c2 & 0x3f);
		// 0xD800-0xDF00 are surrogate codes for 6 byte encoding
		// High code is 0xD800-0xDBFF, Low code is 0xDC00-0xDFFF

		// Plain old code if between 0x0000-0xD7FF, 0xE000-0xFFFF
		if ((out & 0xf800) != 0xd800)
			return out;
	} else
	{
		return -1;
	}

	// Surrogate code should be followed by 3 byte utf8 code
	unsigned int next = in.peek();
	if (next == (unsigned int) (-1) || (next & 0xf0) != 0xe0)
		return out; // unpaired surrogate error.

	// Grab the low word for surrogate code
	c0 = in.get();

	// Note: we should technically check to see what the first byte is here.
	// for a valid code point it must be 0xED.  But if the user has
	// manually encoded an invalid string there is not much we can
	// do as we already are pulling bytes for the next character assuming
	// it was a valid coding. We don't really want to throw an exception
	// because this routine can happen at any point that string is being
	// passed, including reporting of an exception.
	//
	// Manually encoding invalid code points and asking them to be
	// converted is undefined behavior, but I don't know the path
	// to nethack.

	c1 = in.get();
	c2 = in.get();
	next = ((c0 & 0xf) << 12) + ((c1 & 0x3f) << 6) + (c2 & 0x3f);
	if (in.eof())
		return -1;
	unsigned int q1 = (out & 0x3ff);
	unsigned int q2 = (next & 0x3ff);
	return 0x10000 + (q1 << 10) + q2;
}

