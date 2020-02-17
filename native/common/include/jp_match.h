#ifndef JP_MATCH_H
#define JP_MATCH_H

class JPMethodOverload;

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

	Type type;
	bool isVarDirect;
	bool isVarIndirect;
	JPMethodOverload* overload;
	char offset;
	char skip;

	JPMatch()
	{
		type = JPMatch::_none;
		isVarDirect = false;
		isVarIndirect = false;
		overload = NULL;
		offset = 0;
		skip = 0;
	}
} ;

#endif /* JP_MATCH_H */

