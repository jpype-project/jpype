/* ****************************************************************************
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
**************************************************************************** */
package jpype.overloads;

/**
 * Torture test for fixed-arity vs varargs overload specificity
 * (org.jpype.manager.MethodResolution), covering combinations of
 * primitives, Object, a Parent/Child hierarchy, and an unrelated type
 * that Test1's testFixedVsVarArgs/testExpandedVsVarArgs/
 * testMinimalVsVarArgs/testVarArgsVsVarArgs (String-only) do not exercise.
 * <p>
 * Each method family below is a minimal, unambiguous overload pair/set so
 * a given call resolves to exactly one candidate; the returned marker
 * string records which overload actually ran, giving a real end-to-end
 * ground truth (through JPype's live dispatcher, not just the isolated
 * algorithm) for test/jpypetest/test_overloads.py to assert against.
 */
public class TestVarArgsHierarchy
{

  public static class Parent
  {
  }

  public static class Child extends Parent
  {
  }

  public static class Unrelated
  {
  }

  // Case 3 (same slot count): fixed(Object,Parent) vs varargs(Object,Parent...)
  public String testFixedParentVsVarArgsParent(Object o, Parent p)
  {
    return "fixed(O,P)";
  }

  public String testFixedParentVsVarArgsParent(Object o, Parent... p)
  {
    return "varargs(O,P...)";
  }

  // Case 3 with a subtype last param: fixed(Object,Child) vs varargs(Object,Parent...)
  public String testFixedChildVsVarArgsParent(Object o, Child c)
  {
    return "fixed(O,C)";
  }

  public String testFixedChildVsVarArgsParent(Object o, Parent... p)
  {
    return "varargs(O,P...)";
  }

  // Case 2 (fixed has exactly one fewer param): fixed(Object) vs varargs(Object,Parent...)
  public String testFixedShortVsVarArgsParent(Object o)
  {
    return "fixed(O)";
  }

  public String testFixedShortVsVarArgsParent(Object o, Parent... p)
  {
    return "varargs(O,P...)";
  }

  // Case 4 (fixed has more params): (O,P,P) vs (O,P...)
  public String testFixedPPVsVarArgsParent(Object o, Parent p1, Parent p2)
  {
    return "fixed(O,P,P)";
  }

  public String testFixedPPVsVarArgsParent(Object o, Parent... p)
  {
    return "varargs(O,P...)";
  }

  // Case 4: (O,P,C) vs (O,P...)
  public String testFixedPCVsVarArgsParent(Object o, Parent p1, Child p2)
  {
    return "fixed(O,P,C)";
  }

  public String testFixedPCVsVarArgsParent(Object o, Parent... p)
  {
    return "varargs(O,P...)";
  }

  // Case 4: (O,C,C) vs (O,P...)
  public String testFixedCCVsVarArgsParent(Object o, Child p1, Child p2)
  {
    return "fixed(O,C,C)";
  }

  public String testFixedCCVsVarArgsParent(Object o, Parent... p)
  {
    return "varargs(O,P...)";
  }

  // Case 4 with a larger fixed/varargs gap: (O,P,P,P) vs (O,P...)
  public String testFixedPPPVsVarArgsParent(Object o, Parent p1, Parent p2, Parent p3)
  {
    return "fixed(O,P,P,P)";
  }

  public String testFixedPPPVsVarArgsParent(Object o, Parent... p)
  {
    return "varargs(O,P...)";
  }

  // Both varargs, narrower component type wins: (O,C...) vs (O,P...)
  public String testVarArgsChildVsVarArgsParent(Object o, Child... c)
  {
    return "varargs(O,C...)";
  }

  public String testVarArgsChildVsVarArgsParent(Object o, Parent... p)
  {
    return "varargs(O,P...)";
  }

  // An unrelated type cannot bind the Parent-typed fixed overload at all,
  // so it must fall through to the generic Object varargs form.
  public String testUnrelatedFallsThroughToObject(Object o, Parent p)
  {
    return "fixed(O,P)";
  }

  public String testUnrelatedFallsThroughToObject(Object o, Object... p)
  {
    return "varargs(O,Object...)";
  }

  // Primitive widening interacting with varargs (Case 3): fixed(Object,int) vs varargs(Object,long...)
  public String testFixedIntVsVarArgsLong(Object o, int i)
  {
    return "fixed(O,int)";
  }

  public String testFixedIntVsVarArgsLong(Object o, long... l)
  {
    return "varargs(O,long...)";
  }

  // Both varargs, primitive widening: (O,int...) vs (O,long...)
  public String testVarArgsIntVsVarArgsLong(Object o, int... i)
  {
    return "varargs(O,int...)";
  }

  public String testVarArgsIntVsVarArgsLong(Object o, long... l)
  {
    return "varargs(O,long...)";
  }

  // Case 4 with a primitive suffix: fixed(Object,int,int) vs varargs(Object,long...)
  public String testFixedIntIntVsVarArgsLong(Object o, int i1, int i2)
  {
    return "fixed(O,int,int)";
  }

  public String testFixedIntIntVsVarArgsLong(Object o, long... l)
  {
    return "varargs(O,long...)";
  }
}
