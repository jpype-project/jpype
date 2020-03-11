#ifndef JP_MATCH_H
#define JP_MATCH_H

class JPConversion;

class JPMatch
{
public:

	enum Type
	{
		_none = 0,
		_explicit = 1,
		_implicit = 2,
		_exact = 3
	} ;
public:
	JPMatch::Type type;
	JPConversion* conversion;
} ;

class JPMethodMatch
{
public:
	JPMatch::Type type;
	bool isVarIndirect;
	JPMethod* overload;
	char offset;
	char skip;
	std::vector<JPMatch> argument;

	JPMatch& operator[](size_t i)
	{
		return argument[i];
	}

	const JPMatch& operator[](size_t i) const
	{
		return argument[i];
	}

	JPMethodMatch(size_t size)
	: argument(size)
	{
		type = JPMatch::_none;
		isVarIndirect = false;
		overload = 0;
		offset = 0;
		skip = 0;
	}
} ;

#endif /* JP_MATCH_H */

