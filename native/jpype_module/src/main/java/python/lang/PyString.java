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

import python.protocol.PyIterable;
import python.protocol.PyMapping;
import python.protocol.PySequence;

/**
 * Interface for Python objects acting as strings.
 *
 * This interface provides methods to mimic Python string operations in Java. It
 * includes common string manipulation methods, formatting, and encoding
 * options, as well as behaviors to facilitate seamless integration with Java
 * code.
 *
 * Method names mimic those in Java String where possible, while maintaining
 * Python-like functionality for developers familiar with Python string
 * behavior.
 *
 */
public interface PyString extends PyObject, CharSequence
{

  /**
   * Creates a PyString instance from a given CharSequence.
   *
   * @param sequence The input sequence to convert into a PyString.
   * @return A PyString instance representing the given sequence.
   */
  public static PyString from(CharSequence sequence)
  {
    return PyBuiltIn.str(sequence);
  }

  /**
   * Returns the Python type object for strings.
   *
   * @return The Python type object representing strings.
   */
  static PyType type()
  {
    return (PyType) PyBuiltIn.eval("str", null, null);
  }

  /**
   * Returns the character at the specified index.
   *
   * @param index The index of the character to retrieve.
   * @return The character at the specified index.
   */
  @Override
  char charAt(int index);

  /**
   * Checks if the string contains the specified substring.
   *
   * @param substring The substring to search for.
   * @return True if the substring is found, false otherwise.
   */
  boolean containsSubstring(CharSequence substring);

  /**
   * Counts the occurrences of a substring in the string.
   *
   * @param substring The substring to count.
   * @return The number of occurrences of the substring.
   */
  int countOccurrences(CharSequence substring);

  /**
   * Counts the occurrences of a substring in the string, starting from a
   * specific index.
   *
   * @param substring The substring to count.
   * @param start The starting index for the search.
   * @return The number of occurrences of the substring.
   */
  int countOccurrences(CharSequence substring, int start);

  /**
   * Counts the occurrences of a substring in the string within a specific
   * range.
   *
   * @param substring The substring to count.
   * @param start The starting index for the search.
   * @param end The ending index for the search.
   * @return The number of occurrences of the substring.
   */
  int countOccurrences(CharSequence substring, int start, int end);

  /**
   * Checks if the string ends with the specified suffix.
   *
   * @param suffix The suffix to check.
   * @return True if the string ends with the suffix, false otherwise.
   */
  boolean endsWithSuffix(CharSequence suffix);

  /**
   * Checks if the string ends with the specified suffix, starting from a
   * specific index.
   *
   * @param suffix The suffix to check.
   * @param start The starting index for the search.
   * @return True if the string ends with the suffix, false otherwise.
   */
  boolean endsWithSuffix(CharSequence suffix, int start);

  /**
   * Checks if the string ends with the specified suffix within a specific
   * range.
   *
   * @param suffix The suffix to check.
   * @param start The starting index for the search.
   * @param end The ending index for the search.
   * @return True if the string ends with the suffix, false otherwise.
   */
  boolean endsWithSuffix(CharSequence suffix, int start, int end);

  /**
   * Expands tabs in the string to spaces.
   *
   * @param tabSize The number of spaces to replace each tab.
   * @return A new string with tabs replaced by spaces.
   */
  PyString expandTabsToSpaces(int tabSize);

  /**
   * Finds the index of the last occurrence of a substring in the string.
   *
   * @param substring The substring to search for.
   * @return The index of the last occurrence, or -1 if not found.
   */
  int findLastSubstring(CharSequence substring);

  /**
   * Finds the index of the last occurrence of a substring in the string,
   * starting from a specific index.
   *
   * @param substring The substring to search for.
   * @param start The starting index for the search.
   * @return The index of the last occurrence, or -1 if not found.
   */
  int findLastSubstring(CharSequence substring, int start);

  /**
   * Finds the index of the last occurrence of a substring in the string within
   * a specific range.
   *
   * @param substring The substring to search for.
   * @param start The starting index for the search.
   * @param end The ending index for the search.
   * @return The index of the last occurrence, or -1 if not found.
   */
  int findLastSubstring(CharSequence substring, int start, int end);

  /**
   * Finds the first occurrence of a substring in the string.
   *
   * @param substring The substring to search for.
   * @return The index of the first occurrence, or -1 if not found.
   */
  int findSubstring(CharSequence substring);

  /**
   * Finds the first occurrence of a substring in the string, starting from a
   * specific index.
   *
   * @param substring The substring to search for.
   * @param start The starting index for the search.
   * @return The index of the first occurrence, or -1 if not found.
   */
  int findSubstring(CharSequence substring, int start);

  /**
   * Finds the first occurrence of a substring in the string within a specific
   * range.
   *
   * @param substring The substring to search for.
   * @param start The starting index for the search.
   * @param end The ending index for the search.
   * @return The index of the first occurrence, or -1 if not found.
   */
  int findSubstring(CharSequence substring, int start, int end);

  /**
   * Formats the string using a mapping of key-value pairs.
   *
   * @param mapping A mapping object containing keys and their corresponding
   * values.
   * @return A formatted PyString instance.
   */
  PyString formatUsingMapping(PyMapping mapping);

  /**
   * Formats the string using positional and keyword arguments.
   *
   * @param args A tuple containing positional arguments for formatting.
   * @param kwargs A dictionary containing keyword arguments for formatting.
   * @return A formatted PyString instance.
   */
  PyString formatWith(PyTuple args, PyDict kwargs);

  /**
   * Formats the string using a format string and variable arguments.
   *
   * @param format The format string.
   * @param args The arguments to substitute into the format string.
   * @return A formatted PyString instance.
   */
  PyString formatWith(String format, Object... args);

  /**
   * Retrieves the character at the specified index.
   *
   * @param index The index of the character to retrieve.
   * @return The character at the specified index.
   */
  char getCharacterAt(int index);

  /**
   * Finds the index of the last occurrence of a substring in the string. Throws
   * an exception if the substring is not found.
   *
   * @param substring The substring to search for.
   * @return The index of the last occurrence.
   */
  int indexOfLastSubstring(CharSequence substring);

  /**
   * Finds the index of the last occurrence of a substring in the string,
   * starting from a specific index. Throws an exception if the substring is not
   * found.
   *
   * @param substring The substring to search for.
   * @param start The starting index for the search.
   * @return The index of the last occurrence.
   */
  int indexOfLastSubstring(CharSequence substring, int start);

  /**
   * Finds the index of the last occurrence of a substring in the string within
   * a specific range. Throws an exception if the substring is not found.
   *
   * @param substring The substring to search for.
   * @param start The starting index for the search.
   * @param end The ending index for the search.
   * @return The index of the last occurrence.
   */
  int indexOfLastSubstring(CharSequence substring, int start, int end);

  /**
   * Finds the index of the first occurrence of a substring.
   *
   * @param substring The substring to search for.
   * @return The index of the first occurrence, or -1 if not found.
   */
  int indexOfSubstring(CharSequence substring);

  /**
   * Finds the index of the first occurrence of a substring, starting from a
   * specific index.
   *
   * @param substring The substring to search for.
   * @param start The starting index for the search.
   * @return The index of the first occurrence, or -1 if not found.
   */
  int indexOfSubstring(CharSequence substring, int start);

  /**
   * Finds the index of the first occurrence of a substring within a specific
   * range.
   *
   * @param substring The substring to search for.
   * @param start The starting index for the search.
   * @param end The ending index for the search.
   * @return The index of the first occurrence, or -1 if not found.
   */
  int indexOfSubstring(CharSequence substring, int start, int end);

  /**
   * Checks if the string contains only alphabetic characters.
   *
   * @return True if the string is alphabetic, false otherwise.
   */
  boolean isAlphabetic();

  /**
   * Checks if the string contains only alphanumeric characters.
   *
   * @return True if the string is alphanumeric, false otherwise.
   */
  boolean isAlphanumeric();

  /**
   * Checks if the string contains only ASCII characters.
   *
   * @return True if the string contains only ASCII characters, false otherwise.
   */
  boolean isAsciiCharacters();

  /**
   * Checks if the string represents a decimal number.
   *
   * @return True if the string is a decimal number, false otherwise.
   */
  boolean isDecimalNumber();

  /**
   * Checks if the string contains only digit characters.
   *
   * @return True if the string contains only digit characters, false otherwise.
   */
  boolean isDigitCharacters();

  /**
   * Checks if the string is in lowercase.
   *
   * @return True if the string is lowercase, false otherwise.
   */
  boolean isLowercase();

  /**
   * Checks if the string contains only numeric characters.
   *
   * @return True if the string contains only numeric characters, false
   * otherwise.
   */
  boolean isNumericCharacters();

  /**
   * Checks if the string contains only printable characters.
   *
   * @return True if the string is printable, false otherwise.
   */
  boolean isPrintableCharacters();

  /**
   * Checks if the string is in title case.
   *
   * @return True if the string is in title case, false otherwise.
   */
  boolean isTitleCase();

  /**
   * Checks if the string is in uppercase.
   *
   * @return True if the string is uppercase, false otherwise.
   */
  boolean isUppercase();

  /**
   * Checks if the string is a valid Python identifier.
   *
   * @return True if the string is a valid identifier, false otherwise.
   */
  boolean isValidIdentifier();

  /**
   * Checks if the string contains only whitespace characters.
   *
   * @return True if the string is whitespace, false otherwise.
   */
  boolean isWhitespace();

  /**
   * Joins the elements of an iterable into a single string, separated by the
   * current string.
   *
   * @param iterable The iterable containing elements to join.
   * @return A new PyString instance with the joined elements.
   */
  PyString joinWith(PyIterable iterable);

  /**
   * Returns the length of the string.
   *
   * @return The length of the string.
   */
  @Override
  int length();

  /**
   * Centers the string within a specified width, padded with spaces.
   *
   * @param width The total width of the resulting string.
   * @return A new string centered within the specified width.
   */
  PyString paddedCenter(int width);

  /**
   * Centers the string within a specified width, padded with a specified
   * character.
   *
   * @param width The total width of the resulting string.
   * @param fill The character used for padding.
   * @return A new string centered within the specified width.
   */
  PyString paddedCenter(int width, char fill);

  /**
   * Removes the specified prefix from the string if it exists.
   *
   * @param prefix The prefix to remove.
   * @return A new PyString instance with the prefix removed.
   */
  PyString removeLeadingPrefix(CharSequence prefix);

  /**
   * Removes the specified suffix from the string if it exists.
   *
   * @param suffix The suffix to remove.
   * @return A new PyString instance with the suffix removed.
   */
  PyString removeTrailingSuffix(CharSequence suffix);

  /**
   * Replaces occurrences of a substring with a replacement string.
   *
   * @param oldSubstring The substring to replace.
   * @param replacement The replacement string.
   * @return A new PyString instance with the replacements applied.
   */
  PyString replaceSubstring(CharSequence oldSubstring, CharSequence replacement);

  /**
   * Replaces a specified number of occurrences of a substring with a
   * replacement string.
   *
   * @param oldSubstring The substring to replace.
   * @param replacement The replacement string.
   * @param count The maximum number of replacements to perform.
   * @return A new PyString instance with the replacements applied.
   */
  PyString replaceSubstring(CharSequence oldSubstring, CharSequence replacement, int count);

  /**
   * Splits the string into a list of substrings using the specified separator.
   *
   * @param separator The separator to split on.
   * @return A PyList containing the substrings.
   */
  PyList splitInto(CharSequence separator);

  /**
   * Splits the string into a list of substrings using the specified separator,
   * with a maximum number of splits.
   *
   * @param separator The separator to split on.
   * @param maxSplit The maximum number of splits to perform.
   * @return A PyList containing the substrings.
   */
  PyList splitInto(CharSequence separator, int maxSplit);

  /**
   * Splits the string into a list of substrings using whitespace as the default
   * separator.
   *
   * @return A PyList containing the substrings.
   */
  PyList splitInto();

  /**
   * Splits the string into lines, optionally keeping line endings.
   *
   * @param keepEnds Whether to keep line endings in the result.
   * @return A PyList containing the lines.
   */
  PyList splitIntoLines(boolean keepEnds);

  /**
   * Splits the string into a tuple of three parts: the part before the
   * separator, the separator itself, and the part after the separator.
   *
   * @param separator The separator to split on.
   * @return A PyTuple containing the three parts.
   */
  PyTuple splitIntoPartition(CharSequence separator);

  /**
   * Splits the string into a list of substrings, starting from the end of the
   * string.
   *
   * @param separator The separator to split on.
   * @return A PyList containing the substrings.
   */
  PyList splitIntoReverse(CharSequence separator);

  /**
   * Splits the string into a list of substrings, starting from the end of the
   * string, with a maximum number of splits.
   *
   * @param separator The separator to split on.
   * @param maxSplit The maximum number of splits to perform.
   * @return A PyList containing the substrings.
   */
  PyList splitIntoReverse(CharSequence separator, int maxSplit);

  /**
   * Splits the string into a list of substrings, starting from the end of the
   * string. Uses whitespace as the default separator.
   *
   * @return A PyList containing the substrings.
   */
  PyList splitIntoReverse();

  /**
   * Splits the string into three parts: the part before the separator, the
   * separator itself, and the part after the separator, searching from the end
   * of the string.
   *
   * @param separator The separator to split on.
   * @return A PyTuple containing the three parts.
   */
  PyTuple splitIntoReversePartition(CharSequence separator);

  /**
   * Checks if the string starts with the specified prefix.
   *
   * @param prefix The prefix to check.
   * @return True if the string starts with the prefix, false otherwise.
   */
  boolean startsWithPrefix(CharSequence prefix);

  /**
   * Checks if the string starts with the specified prefix, starting from a
   * specific index.
   *
   * @param prefix The prefix to check.
   * @param start The starting index for the check.
   * @return True if the string starts with the prefix, false otherwise.
   */
  boolean startsWithPrefix(CharSequence prefix, int start);

  /**
   * Checks if the string starts with the specified prefix within a specific
   * range.
   *
   * @param prefix The prefix to check.
   * @param start The starting index for the check.
   * @param end The ending index for the check.
   * @return True if the string starts with the prefix, false otherwise.
   */
  boolean startsWithPrefix(CharSequence prefix, int start, int end);

  /**
   * Removes all leading and trailing occurrences of the specified characters
   * from the string.
   *
   * @param characters The characters to remove.
   * @return A new PyString instance with characters removed.
   */
  PyString stripCharacters(CharSequence characters);

  /**
   * Removes leading whitespace from the string.
   *
   * @return A new PyString instance with leading whitespace removed.
   */
  PyString stripLeading();

  /**
   * Removes leading occurrences of the specified characters from the string.
   *
   * @param characters The characters to remove.
   * @return A new PyString instance with leading characters removed.
   */
  PyString stripLeading(CharSequence characters);

  /**
   * Removes trailing occurrences of the specified characters from the string.
   *
   * @param characters The characters to remove.
   * @return A new PyString instance with trailing characters removed.
   */
  PyString stripTrailing(CharSequence characters);

  /**
   * Removes all leading and trailing whitespace from the string.
   *
   * @return A new PyString instance with whitespace removed.
   */
  PyString stripWhitespace();

  /**
   * Returns a subsequence of the string between the specified start and end
   * indices.
   *
   * @param start The starting index of the subsequence.
   * @param end The ending index of the subsequence.
   * @return A PyString representing the subsequence.
   */
  @Override
  PyString subSequence(int start, int end);

  /**
   * Swaps the case of all characters in the string. Uppercase characters are
   * converted to lowercase, and vice versa.
   *
   * @return A new PyString instance with swapped case characters.
   */
  PyString swapCaseCharacters();

  /**
   * Converts the string to a capitalized version. The first character is
   * converted to uppercase, and the rest to lowercase.
   *
   * @return A capitalized version of the string.
   */
  PyString toCapitalized();

  /**
   * Converts the string to a case-folded version. Case folding is useful for
   * caseless matching.
   *
   * @return A case-folded version of the string.
   */
  PyString toCaseFolded();

  /**
   * Encodes the string using the default encoding.
   *
   * @return A PyBytes object representing the encoded string.
   */
  PyBytes toEncoded();

  /**
   * Encodes the string using the specified encoding.
   *
   * @param encoding The name of the encoding to use.
   * @return A PyBytes object representing the encoded string.
   */
  PyBytes toEncoded(CharSequence encoding);

  /**
   * Encodes the string using the specified encoding and error handling
   * strategy.
   *
   * @param encoding The name of the encoding to use.
   * @param errorHandling The error handling strategy (e.g., "strict",
   * "ignore").
   * @return A PyBytes object representing the encoded string.
   */
  PyBytes toEncoded(CharSequence encoding, String errorHandling);

  /**
   * Converts the string to title case. Each word's first character is
   * capitalized, and the rest are lowercase.
   *
   * @return A new PyString instance in title case.
   */
  PyString toTitleCase();

  /**
   * Converts the string to uppercase.
   *
   * @return A new PyString instance in uppercase.
   */
  PyString toUppercase();

  /**
   * Translates the string using a mapping object.
   *
   * @param mapping The mapping object specifying character replacements.
   * @return A new PyString instance with translated characters.
   */
  PyString translateUsingMapping(PyMapping mapping);

  /**
   * Translates the string using a sequence of character replacements.
   *
   * @param sequence The sequence specifying character replacements.
   * @return A new PyString instance with translated characters.
   */
  PyString translateUsingSequence(PySequence sequence);

  /**
   * Pads the string with zeros on the left to reach the specified width.
   *
   * @param width The total width of the resulting string.
   * @return A new PyString instance with zero padding.
   */
  PyString zeroFill(int width);
}
