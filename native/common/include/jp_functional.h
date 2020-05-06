#ifndef JP_FUNCTIONAL_H
#define JP_FUNCTIONAL_H

class JPFunctional : public JPClass
{
public:
	JPFunctional(JPJavaFrame& frame,
			jclass clss,
			const string& name,
			JPClass* super,
			JPClassList& interfaces,
			jint modifiers);
	virtual ~JPFunctional();

	virtual JPMatch::Type findJavaConversion(JPMatch &match) override;
	virtual void getConversionInfo(JPConversionInfo &info) override;

	string getMethod()
	{
		return m_Method;
	}
protected:
	string  m_Method;
} ;

#endif /* JP_FUNCTIONAL_H */