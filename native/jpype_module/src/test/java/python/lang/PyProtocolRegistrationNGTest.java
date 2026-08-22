// --- file: python/lang/PyProtocolRegistrationNGTest.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 *
 *  See NOTICE file for details.
 */
package python.lang;

import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Verifies the concrete-type registrations added for interfaces that
 * previously had no wiring in {@code _jbridge.py}'s {@code _jpype._concrete}
 * table: a bare Python function/lambda crossing the bridge is recognized as
 * {@link PyFunction}, a {@code filter(...)} object as {@link PyFilter}, and
 * the {@code ...}/{@code NotImplemented} singletons as {@link PyEllipsis}/
 * {@link PyNotImplemented}. Before this wiring, PyJP_probe had nothing to
 * match these Python types against, so they only ever surfaced as plain
 * PyObject.
 */
public class PyProtocolRegistrationNGTest extends PyTestHarness
{

  @Test
  public void testPlainFunctionIsPyFunction()
  {
    context.exec("def _reg_fn():\n    return 1\n");
    PyObject fn = context.eval("_reg_fn");

    assertTrue(fn instanceof PyFunction);
    assertTrue(fn instanceof PyCallable);
  }

  @Test
  public void testLambdaIsPyFunction()
  {
    PyObject fn = context.eval("lambda x: x");

    assertTrue(fn instanceof PyFunction);
  }

  @Test
  public void testFilterIsPyFilter()
  {
    context.exec("_reg_filter = filter(lambda x: x > 1, [1, 2, 3])\n");
    PyObject f = context.eval("_reg_filter");

    assertTrue(f instanceof PyFilter);
  }

  @Test
  public void testEllipsisIsPyEllipsis()
  {
    PyObject e = context.eval("...");

    assertTrue(e instanceof PyEllipsis);
  }

  @Test
  public void testNotImplementedIsPyNotImplemented()
  {
    PyObject ni = context.eval("NotImplemented");

    assertTrue(ni instanceof PyNotImplemented);
  }
}
