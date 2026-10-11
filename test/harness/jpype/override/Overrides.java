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
package jpype.override;

// Fixtures for the sticky-customizer inheritance/interface-depth tests
// (test_customizer.py's testStickyInheritanceAndInterfaceDepth and
// testStickyMismatchedRenameStack), grouped as nested types in one file
// rather than one top-level type per file - see jpype.overloads.Test1 for
// the same convention elsewhere in this harness.
public class Overrides
{
  // Customizer target for the interface-based sticky-method tests, mirroring
  // java.util.List's role in the original bug report (jpype-project/jpype#1473).
  public interface I0
  {
    int remove(Object o);
  }

  // An interface farther down the tree than the customizer target (I0),
  // redeclaring nothing of its own - mirrors an EMF-style EList extending
  // java.util.List without adding to the method it inherits.
  public interface I1 extends I0
  {
  }

  // Directly implements the customizer target interface, with its own
  // remove() - the "AbstractList" role: the first concrete redeclaration
  // below I0.
  public static class IBase implements I0
  {
    public int remove(Object o)
    {
      return 1;
    }
  }

  // Inherits I0's remove() through IBase without redeclaring it - the
  // "EList" role from jpype-project/jpype#1473: a class reached purely
  // through inheritance from a class that implements the customizer's
  // target interface.
  public static class ISub extends IBase
  {
  }

  // Inherits I0's remove() through IBase (no redeclaration), and also
  // implements I1 - the sub-interface farther down the tree than the
  // customizer target. Exercises the case where the no-override class
  // reaches the target interface through two paths (extends + implements).
  public static class ISubIface extends IBase implements I1
  {
  }

  // Redeclares remove() at this depth despite also implementing I1 - the
  // "ArrayList" role: confirms a genuine re-override still gets its own
  // fresh rename even below a farther-down interface.
  public static class ISubOverride extends IBase implements I1
  {
    public int remove(Object o)
    {
      return 2;
    }
  }

  // A customizer target dedicated to sticky-method stacking tests
  // (3+ customizers, mismatched rename targets) - kept separate from I0's
  // family so these tests don't interact with the inheritance/interface
  // depth coverage on I0/I1, regardless of test execution order within the
  // shared JVM session.
  public interface IStack
  {
    int remove(Object o);
  }

  public static class IStackImpl implements IStack
  {
    public int remove(Object o)
    {
      return 1;
    }
  }

  // Customizer target dedicated to the multiple-registrations-for-the-same-
  // -target tests (__jclass_init__ hook composition, retroactive sticky
  // registration) - kept separate from the other families above so these
  // tests don't interact with them regardless of execution order within
  // the shared JVM session.
  public interface IRetro
  {
    int remove(Object o);
  }

  public static class IRetroImpl implements IRetro
  {
    public int remove(Object o)
    {
      return 1;
    }
  }

  // Created (in the test) only after a second customizer for IRetro has
  // been registered retroactively - exercises whether earlier
  // registrations still apply to classes built afterward.
  public static class IRetroSub extends IRetroImpl
  {
  }
}
