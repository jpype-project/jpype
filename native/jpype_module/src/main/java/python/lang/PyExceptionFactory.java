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
import python.exception.PyADirectionError;
import python.exception.PyArithmeticError;
import python.exception.PyAssertionError;
import python.exception.PyAttributeError;
import python.exception.PyBlockingIOError;
import python.exception.PyBufferError;
import python.exception.PyChildProcessError;
import python.exception.PyConnectionError;
import python.exception.PyEOFError;
import python.exception.PyException;
import python.exception.PyFileExistsError;
import python.exception.PyFileNotFoundError;
import python.exception.PyFloatingPointError;
import python.exception.PyImportError;
import python.exception.PyIndentationError;
import python.exception.PyIndexError;
import python.exception.PyInterruptedError;
import python.exception.PyKeyError;
import python.exception.PyLookupError;
import python.exception.PyModuleNotFoundError;
import python.exception.PyNameError;
import python.exception.PyNotADirectoryError;
import python.exception.PyNotImplementedError;
import python.exception.PyOSError;
import python.exception.PyOverflowError;
import python.exception.PyPermissionError;
import python.exception.PyProcessLookupError;
import python.exception.PyRecursionError;
import python.exception.PyReferenceError;
import python.exception.PyRuntimeError;
import python.exception.PySyntaxError;
import python.exception.PySystemError;
import python.exception.PyTimeoutError;
import python.exception.PyTypeError;
import python.exception.PyValueError;
import python.exception.PyWarning;
import python.exception.PyZeroDivisionError;

/**
 *
 * @author nelson85
 */
class PyExceptionFactory
{
  
  final static HashMap<String, Class> LOOKUP = new HashMap<>();

  static
  {
    LOOKUP.put("Exception", PyException.class);
    LOOKUP.put("ADirectionError", PyADirectionError.class);
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
