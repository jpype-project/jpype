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

	JPContext *getContext()
	{
		if (frame == NULL)
			return NULL;
		return frame->getContext();
	}

	/**
	 * Get the Java slot associated with the Python object.
	 *
	 * Thus uses caching.
	 *
	 * @return the Java slot or 0 if not available.
	 */
	JPValue *getJavaSlot();

	jvalue convert();

public:
	JPMatch::Type type;
	JPConversion *conversion;
	JPJavaFrame *frame;
	PyObject *object;
	JPValue *slot;
	void *closure;
} ;

class JPMethodMatch
{
public:

	JPMethodMatch(JPJavaFrame &frame, JPPyObjectVector& args);

	JPMatch& operator[](size_t i)
	{
		return argument[i];
	}

	const JPMatch& operator[](size_t i) const
	{
		return argument[i];
	}

public:
	JPMatch::Type type;
	bool isVarIndirect;
	JPMethod* overload;
	char offset;
	char skip;
	std::vector<JPMatch> argument;
} ;

#endif /* JP_MATCH_H */

