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
	JPMatch();
	JPMatch(JPJavaFrame *frame, PyObject *object);

	JPMatch::Type type;
	JPConversion *conversion;
	JPJavaFrame *frame;
	PyObject *object;

	JPContext *getContext()
	{
		if (frame == NULL)
			return NULL;
		return frame->getContext();
	}
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

	JPMethodMatch(JPJavaFrame &frame, JPPyObjectVector& args)
	: argument(args.size())
	{
		type = JPMatch::_none;
		isVarIndirect = false;
		overload = 0;
		offset = 0;
		skip = 0;
		for (size_t i = 0; i < args.size(); ++i)
		{
			argument[i] = JPMatch(&frame, args[i]);
		}
	}
} ;

#endif /* JP_MATCH_H */

