
package jpype.lambda;
import java.util.function.Function;

public class Test1
{
	public Function<Double, Double> getFunction()
	{
		return new Function<Double,Double>()
		{
			public Double apply(Double d)
			{
				return d+1;
			}
		};
	}

	public Function<Double, Double> getLambda()
	{
		return (Double d)->(d+1);
	}
}

