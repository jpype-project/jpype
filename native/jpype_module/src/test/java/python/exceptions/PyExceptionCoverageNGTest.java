// --- file: python/exceptions/PyExceptionCoverageNGTest.java ---
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
package python.exceptions;

import python.lang.PyExc;
import python.lang.PyTestHarness;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Exercises every {@code python.exceptions.Py*Error} class that isn't
 * already reached incidentally by other suites (see plan/Coverage.md), by
 * raising the matching Python builtin exception in the embedded interpreter
 * and catching the corresponding generated Java type on the way back out.
 */
public class PyExceptionCoverageNGTest extends PyTestHarness
{

  @Test
  public void testAssertionError()
  {
    try
    {
      context.exec("assert False, 'boom'");
      fail("Expected a PyAssertionError");
    } catch (PyAssertionError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testBlockingIOError()
  {
    try
    {
      context.exec("raise BlockingIOError('boom')");
      fail("Expected a PyBlockingIOError");
    } catch (PyBlockingIOError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testBufferError()
  {
    try
    {
      context.exec("raise BufferError('boom')");
      fail("Expected a PyBufferError");
    } catch (PyBufferError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testChildProcessError()
  {
    try
    {
      context.exec("raise ChildProcessError('boom')");
      fail("Expected a PyChildProcessError");
    } catch (PyChildProcessError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testConnectionError()
  {
    try
    {
      context.exec("raise ConnectionError('boom')");
      fail("Expected a PyConnectionError");
    } catch (PyConnectionError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testEOFError()
  {
    try
    {
      context.exec("raise EOFError('boom')");
      fail("Expected a PyEOFError");
    } catch (PyEOFError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testFileExistsError()
  {
    try
    {
      context.exec("raise FileExistsError('boom')");
      fail("Expected a PyFileExistsError");
    } catch (PyFileExistsError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testFileNotFoundError()
  {
    try
    {
      context.exec("raise FileNotFoundError('boom')");
      fail("Expected a PyFileNotFoundError");
    } catch (PyFileNotFoundError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testFloatingPointError()
  {
    try
    {
      context.exec("raise FloatingPointError('boom')");
      fail("Expected a PyFloatingPointError");
    } catch (PyFloatingPointError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testGeneratorExit()
  {
    try
    {
      context.exec("raise GeneratorExit('boom')");
      fail("Expected a PyGeneratorExit");
    } catch (PyGeneratorExit ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testImportError()
  {
    try
    {
      context.exec("raise ImportError('boom')");
      fail("Expected a PyImportError");
    } catch (PyImportError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testIndentationError()
  {
    try
    {
      context.exec("raise IndentationError('boom')");
      fail("Expected a PyIndentationError");
    } catch (PyIndentationError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testInterruptedError()
  {
    try
    {
      context.exec("raise InterruptedError('boom')");
      fail("Expected a PyInterruptedError");
    } catch (PyInterruptedError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testIsADirectoryError()
  {
    try
    {
      context.exec("raise IsADirectoryError('boom')");
      fail("Expected a PyIsADirectoryError");
    } catch (PyIsADirectoryError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testKeyboardInterrupt()
  {
    try
    {
      context.exec("raise KeyboardInterrupt('boom')");
      fail("Expected a PyKeyboardInterrupt");
    } catch (PyKeyboardInterrupt ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testMemoryError()
  {
    try
    {
      context.exec("raise MemoryError('boom')");
      fail("Expected a PyMemoryError");
    } catch (PyMemoryError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testModuleNotFoundError()
  {
    try
    {
      context.exec("import this_module_does_not_exist_xyz");
      fail("Expected a PyModuleNotFoundError");
    } catch (PyModuleNotFoundError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testNameError()
  {
    try
    {
      context.eval("this_name_is_not_defined_xyz");
      fail("Expected a PyNameError");
    } catch (PyNameError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testNotADirectoryError()
  {
    try
    {
      context.exec("raise NotADirectoryError('boom')");
      fail("Expected a PyNotADirectoryError");
    } catch (PyNotADirectoryError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testNotImplementedError()
  {
    try
    {
      context.exec("raise NotImplementedError('boom')");
      fail("Expected a PyNotImplementedError");
    } catch (PyNotImplementedError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testOSError()
  {
    try
    {
      context.exec("raise OSError('boom')");
      fail("Expected a PyOSError");
    } catch (PyOSError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testOverflowError()
  {
    try
    {
      context.exec("raise OverflowError('boom')");
      fail("Expected a PyOverflowError");
    } catch (PyOverflowError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testPermissionError()
  {
    try
    {
      context.exec("raise PermissionError('boom')");
      fail("Expected a PyPermissionError");
    } catch (PyPermissionError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testProcessLookupError()
  {
    try
    {
      context.exec("raise ProcessLookupError('boom')");
      fail("Expected a PyProcessLookupError");
    } catch (PyProcessLookupError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testRecursionError()
  {
    try
    {
      context.exec("raise RecursionError('boom')");
      fail("Expected a PyRecursionError");
    } catch (PyRecursionError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testReferenceError()
  {
    try
    {
      context.exec("raise ReferenceError('boom')");
      fail("Expected a PyReferenceError");
    } catch (PyReferenceError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testRuntimeError()
  {
    try
    {
      context.exec("raise RuntimeError('boom')");
      fail("Expected a PyRuntimeError");
    } catch (PyRuntimeError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testStopAsyncIteration()
  {
    try
    {
      context.exec("raise StopAsyncIteration('boom')");
      fail("Expected a PyStopAsyncIteration");
    } catch (PyStopAsyncIteration ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testStopIteration()
  {
    try
    {
      context.eval("next(iter([]))");
      fail("Expected a PyStopIteration");
    } catch (PyStopIteration ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testSystemError()
  {
    try
    {
      context.exec("raise SystemError('boom')");
      fail("Expected a PySystemError");
    } catch (PySystemError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testTabError()
  {
    try
    {
      context.exec("raise TabError('boom')");
      fail("Expected a PyTabError");
    } catch (PyTabError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testTimeoutError()
  {
    try
    {
      context.exec("raise TimeoutError('boom')");
      fail("Expected a PyTimeoutError");
    } catch (PyTimeoutError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testUnboundLocalError()
  {
    try
    {
      context.exec("raise UnboundLocalError('boom')");
      fail("Expected a PyUnboundLocalError");
    } catch (PyUnboundLocalError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testUnicodeDecodeError()
  {
    try
    {
      context.eval("b'\\xff'.decode('utf-8')");
      fail("Expected a PyUnicodeDecodeError");
    } catch (PyUnicodeDecodeError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testUnicodeEncodeError()
  {
    try
    {
      context.eval("'\\u2603'.encode('ascii')");
      fail("Expected a PyUnicodeEncodeError");
    } catch (PyUnicodeEncodeError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testUnicodeError()
  {
    try
    {
      context.exec("raise UnicodeError('boom')");
      fail("Expected a PyUnicodeError");
    } catch (PyUnicodeError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testUnicodeTranslateError()
  {
    try
    {
      context.exec("raise UnicodeTranslateError('s', 0, 1, 'boom')");
      fail("Expected a PyUnicodeTranslateError");
    } catch (PyUnicodeTranslateError ex)
    {
      assertNotNull(ex.get());
    }
  }

  @Test
  public void testWarning()
  {
    try
    {
      context.exec("raise Warning('boom')");
      fail("Expected a PyWarning");
    } catch (PyWarning ex)
    {
      assertNotNull(ex.get());
    }
  }

}
