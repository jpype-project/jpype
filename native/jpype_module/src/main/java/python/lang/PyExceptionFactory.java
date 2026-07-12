// --- file: python/lang/PyExceptionFactory.java ---
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

import java.util.HashMap;
import python.exceptions.PyIsADirectoryError;
import python.exceptions.PyArithmeticError;
import python.exceptions.PyAssertionError;
import python.exceptions.PyAttributeError;
import python.exceptions.PyBlockingIOError;
import python.exceptions.PyBufferError;
import python.exceptions.PyChildProcessError;
import python.exceptions.PyConnectionError;
import python.exceptions.PyEOFError;
import python.exceptions.PyException;
import python.exceptions.PyFileExistsError;
import python.exceptions.PyFileNotFoundError;
import python.exceptions.PyFloatingPointError;
import python.exceptions.PyImportError;
import python.exceptions.PyIndentationError;
import python.exceptions.PyIndexError;
import python.exceptions.PyInterruptedError;
import python.exceptions.PyKeyError;
import python.exceptions.PyLookupError;
import python.exceptions.PyModuleNotFoundError;
import python.exceptions.PyNameError;
import python.exceptions.PyNotADirectoryError;
import python.exceptions.PyNotImplementedError;
import python.exceptions.PyOSError;
import python.exceptions.PyOverflowError;
import python.exceptions.PyPermissionError;
import python.exceptions.PyProcessLookupError;
import python.exceptions.PyRecursionError;
import python.exceptions.PyReferenceError;
import python.exceptions.PyRuntimeError;
import python.exceptions.PySyntaxError;
import python.exceptions.PySystemError;
import python.exceptions.PyTimeoutError;
import python.exceptions.PyTypeError;
import python.exceptions.PyValueError;
import python.exceptions.PyWarning;
import python.exceptions.PyZeroDivisionError;

/**
 *
 */
class PyExceptionFactory
{

  final static HashMap<String, Class> LOOKUP = new HashMap<>();

  static
  {
    LOOKUP.put("Exception", PyException.class);
    LOOKUP.put("ADirectionError", PyIsADirectoryError.class);
    LOOKUP.put("ArithmeticError", PyArithmeticError.class);
    LOOKUP.put("AssertionError", PyAssertionError.class);
    LOOKUP.put("AttributeError", PyAttributeError.class);
    LOOKUP.put("BlockingIOError", PyBlockingIOError.class);
    LOOKUP.put("BufferError", PyBufferError.class);
    LOOKUP.put("ChildProcessError", PyChildProcessError.class);
    LOOKUP.put("ConnectionError", PyConnectionError.class);
    LOOKUP.put("EOFError", PyEOFError.class);
    LOOKUP.put("FileExistsError", PyFileExistsError.class);
    LOOKUP.put("FileNotFoundError", PyFileNotFoundError.class);
    LOOKUP.put("FloatingPointError", PyFloatingPointError.class);
    LOOKUP.put("ImportError", PyImportError.class);
    LOOKUP.put("IndentationError", PyIndentationError.class);
    LOOKUP.put("IndexError", PyIndexError.class);
    LOOKUP.put("InterruptedError", PyInterruptedError.class);
    LOOKUP.put("KeyError", PyKeyError.class);
    LOOKUP.put("LookupError", PyLookupError.class);
    LOOKUP.put("ModuleNotFoundError", PyModuleNotFoundError.class);
    LOOKUP.put("NameError", PyNameError.class);
    LOOKUP.put("NotADirectoryError", PyNotADirectoryError.class);
    LOOKUP.put("NotImplementedError", PyNotImplementedError.class);
    LOOKUP.put("OSError", PyOSError.class);
    LOOKUP.put("OverflowError", PyOverflowError.class);
    LOOKUP.put("PermissionError", PyPermissionError.class);
    LOOKUP.put("ProcessLookupError", PyProcessLookupError.class);
    LOOKUP.put("RecursionError", PyRecursionError.class);
    LOOKUP.put("ReferenceError", PyReferenceError.class);
    LOOKUP.put("RuntimeError", PyRuntimeError.class);
    LOOKUP.put("SyntaxError", PySyntaxError.class);
    LOOKUP.put("SystemError", PySystemError.class);
    LOOKUP.put("TimeoutError", PyTimeoutError.class);
    LOOKUP.put("TypeError", PyTypeError.class);
    LOOKUP.put("ValueError", PyValueError.class);
    LOOKUP.put("Warning", PyWarning.class);
    LOOKUP.put("ZeroDivisionError", PyZeroDivisionError.class);
  }

}
