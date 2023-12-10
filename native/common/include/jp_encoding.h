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
#ifndef _JP_ENCODING_H_
#define _JP_ENCODING_H_

#include <iostream>
#include <string>
#include <sstream>

class JPEncoding
{
public:

	JPEncoding()
	= default;
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