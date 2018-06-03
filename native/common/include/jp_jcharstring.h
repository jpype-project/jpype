/*****************************************************************************
   Copyright 2004 Steve Ménard

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
*****************************************************************************/   
#ifndef _JP_CHAR_STRING_H_
#define _JP_CHAR_STRING_H_

/** Use this class instead of basic_string<jchar> because compiler support is not great cross-platform */
class JCharString
{
public :
	JCharString(const jchar*);
	JCharString(const JCharString&);
	JCharString(size_t);
	virtual ~JCharString();

	const jchar* c_str();

	size_t length() { return m_Length; }

	jchar& operator[](size_t ndx) { return m_Value[ndx]; }

private :
	jchar* m_Value;
	size_t m_Length;
};

#endif // _JP_CHAR_STRING_H_
