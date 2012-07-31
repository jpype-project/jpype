//*****************************************************************************
//   Copyright 2004-2008 Steve Menard
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//   
//*****************************************************************************
package jpype.objectwrapper;

public class Test1
{
	public Test1()
	{
	}
	
	public int Method1(Number n)
	{
		return 1;
	}
	
	public int Method1(Integer n)
	{
		return 2;
	}

	public int Method1(Object n)
	{
		return 3;
	}

	public int Method1(String n)
	{
		return 4;
	}
	
}