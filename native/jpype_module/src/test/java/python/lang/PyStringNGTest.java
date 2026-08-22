// --- file: python/lang/PyStringNGTest.java ---
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

public class PyStringNGTest extends PyTestHarness
{

  @Test
  public void testCharAt()
  {
    PyString s = context.str("hello");
    assertEquals(s.charAt(1), 'e');
  }

  @Test
  public void testGetCharacterAt()
  {
    PyString s = context.str("hello");
    assertEquals(s.getCharacterAt(0), 'h');
  }

  @Test
  public void testLength()
  {
    PyString s = context.str("hello");
    assertEquals(s.length(), 5);
  }

  @Test
  public void testContainsSubstring()
  {
    PyString s = context.str("hello world");
    assertTrue(s.containsSubstring("wor"));
    assertFalse(s.containsSubstring("xyz"));
  }

  @Test
  public void testCountOccurrences()
  {
    PyString s = context.str("abcabcabc");
    assertEquals(s.countOccurrences("abc"), 3);
    assertEquals(s.countOccurrences("abc", 3), 2);
    assertEquals(s.countOccurrences("abc", 0, 6), 2);
  }

  @Test
  public void testEndsWithSuffix()
  {
    PyString s = context.str("hello world");
    assertTrue(s.endsWithSuffix("world"));
    assertFalse(s.endsWithSuffix("hello"));
    assertTrue(s.endsWithSuffix("hello", 0, 5));
  }

  @Test
  public void testExpandTabs()
  {
    PyString s = context.str("a\tb");
    PyString expanded = s.expandTabs(4);
    assertEquals(expanded.toString(), "a   b");
  }

  @Test
  public void testFindSubstring()
  {
    PyString s = context.str("hello world");
    assertEquals(s.findSubstring("world"), 6);
    assertEquals(s.findSubstring("xyz"), -1);
    assertEquals(s.findSubstring("o", 5), 7);
  }

  @Test
  public void testFindLastSubstring()
  {
    PyString s = context.str("abcabc");
    assertEquals(s.findLastSubstring("abc"), 3);
  }

  @Test
  public void testFormatUsingMapping()
  {
    PyString s = context.str("Hello, {name}!");
    PyDict mapping = context.dict();
    mapping.putAny("name", "World");
    PyString result = s.formatUsingMapping(mapping);
    assertEquals(result.toString(), "Hello, World!");
  }

  @Test
  public void testFormatWith()
  {
    PyString s = context.str("{} and {}");
    PyString result = s.formatWith(context.tuple("a", "b"), context.dict());
    assertEquals(result.toString(), "a and b");
  }

  @Test
  public void testIndexOfSubstring()
  {
    PyString s = context.str("hello world");
    assertEquals(s.indexOfSubstring("world"), 6);
  }

  @Test
  public void testIndexOfLastSubstring()
  {
    PyString s = context.str("abcabc");
    assertEquals(s.indexOfLastSubstring("abc"), 3);
  }

  @Test
  public void testIsAlphabetic()
  {
    assertTrue(context.str("hello").isAlphabetic());
    assertFalse(context.str("hello1").isAlphabetic());
  }

  @Test
  public void testIsAlphanumeric()
  {
    assertTrue(context.str("hello1").isAlphanumeric());
    assertFalse(context.str("hello!").isAlphanumeric());
  }

  @Test
  public void testIsAsciiCharacters()
  {
    assertTrue(context.str("hello").isAsciiCharacters());
  }

  @Test
  public void testIsDecimalNumber()
  {
    assertTrue(context.str("123").isDecimalNumber());
    assertFalse(context.str("12.3").isDecimalNumber());
  }

  @Test
  public void testIsDigitCharacters()
  {
    assertTrue(context.str("123").isDigitCharacters());
    assertFalse(context.str("abc").isDigitCharacters());
  }

  @Test
  public void testIsLowercase()
  {
    assertTrue(context.str("hello").isLowercase());
    assertFalse(context.str("Hello").isLowercase());
  }

  @Test
  public void testIsNumericCharacters()
  {
    assertTrue(context.str("123").isNumericCharacters());
    assertFalse(context.str("abc").isNumericCharacters());
  }

  @Test
  public void testIsPrintableCharacters()
  {
    assertTrue(context.str("hello").isPrintableCharacters());
  }

  @Test
  public void testIsTitleCase()
  {
    assertTrue(context.str("Hello World").isTitleCase());
    assertFalse(context.str("hello world").isTitleCase());
  }

  @Test
  public void testIsUppercase()
  {
    assertTrue(context.str("HELLO").isUppercase());
    assertFalse(context.str("Hello").isUppercase());
  }

  @Test
  public void testIsValidIdentifier()
  {
    assertTrue(context.str("my_var").isValidIdentifier());
    assertFalse(context.str("123abc").isValidIdentifier());
  }

  @Test
  public void testIsWhitespace()
  {
    assertTrue(context.str("   ").isWhitespace());
    assertFalse(context.str("a b").isWhitespace());
  }

  @Test
  public void testJoin()
  {
    PyString sep = context.str(",");
    PyList items = context.listFromObjects(context.str("a"), context.str("b"), context.str("c"));
    PyString result = sep.join(items);
    assertEquals(result.toString(), "a,b,c");
  }

  @Test
  public void testPaddedCenter()
  {
    PyString s = context.str("hi");
    assertEquals(s.paddedCenter(6).toString(), "  hi  ");
    assertEquals(s.paddedCenter(6, '*').toString(), "**hi**");
  }

  @Test
  public void testRemovePrefix()
  {
    PyString s = context.str("hello world");
    assertEquals(s.removePrefix("hello ").toString(), "world");
  }

  @Test
  public void testRemoveSuffix()
  {
    PyString s = context.str("hello world");
    assertEquals(s.removeSuffix(" world").toString(), "hello");
  }

  @Test
  public void testReplaceSubstring()
  {
    PyString s = context.str("hello world");
    assertEquals(s.replaceSubstring("world", "there").toString(), "hello there");
  }

  @Test
  public void testReplaceSubstringWithCount()
  {
    PyString s = context.str("aaaa");
    assertEquals(s.replaceSubstring("a", "b", 2).toString(), "bbaa");
  }

  @Test
  public void testSplitInto()
  {
    PyString s = context.str("a,b,c");
    PyList parts = s.splitInto(",");
    assertEquals(parts.size(), 3);
    assertEquals(parts.get(0).toString(), "a");
  }

  @Test
  public void testSplitIntoDefault()
  {
    PyString s = context.str("a b  c");
    PyList parts = s.splitInto();
    assertEquals(parts.size(), 3);
  }

  @Test
  public void testSplitIntoMaxSplit()
  {
    PyString s = context.str("a,b,c");
    PyList parts = s.splitInto(",", 1);
    assertEquals(parts.size(), 2);
    assertEquals(parts.get(1).toString(), "b,c");
  }

  @Test
  public void testSplitIntoLines()
  {
    PyString s = context.str("a\nb\nc");
    PyList lines = s.splitIntoLines(false);
    assertEquals(lines.size(), 3);
  }

  @Test
  public void testSplitIntoPartition()
  {
    PyString s = context.str("key=value");
    PyTuple parts = s.splitIntoPartition("=");
    assertEquals(parts.get(0).toString(), "key");
    assertEquals(parts.get(1).toString(), "=");
    assertEquals(parts.get(2).toString(), "value");
  }

  @Test
  public void testSplitIntoReverse()
  {
    PyString s = context.str("a,b,c");
    PyList parts = s.splitIntoReverse(",", 1);
    assertEquals(parts.size(), 2);
    assertEquals(parts.get(0).toString(), "a,b");
  }

  @Test
  public void testSplitIntoReversePartition()
  {
    PyString s = context.str("key=value=extra");
    PyTuple parts = s.splitIntoReversePartition("=");
    assertEquals(parts.get(0).toString(), "key=value");
    assertEquals(parts.get(2).toString(), "extra");
  }

  @Test
  public void testStartsWithPrefix()
  {
    PyString s = context.str("hello world");
    assertTrue(s.startsWithPrefix("hello"));
    assertFalse(s.startsWithPrefix("world"));
    assertTrue(s.startsWithPrefix("world", 6));
  }

  @Test
  public void testStripCharacters()
  {
    PyString s = context.str("xxhelloxx");
    assertEquals(s.stripCharacters("x").toString(), "hello");
  }

  @Test
  public void testStripLeading()
  {
    PyString s = context.str("  hello");
    assertEquals(s.stripLeading().toString(), "hello");
  }

  @Test
  public void testStripTrailing()
  {
    PyString s = context.str("helloxx");
    assertEquals(s.stripTrailing("x").toString(), "hello");
  }

  @Test
  public void testStripWhitespace()
  {
    PyString s = context.str("  hello  ");
    assertEquals(s.stripWhitespace().toString(), "hello");
  }

  @Test
  public void testSubSequence()
  {
    PyString s = context.str("hello world");
    assertEquals(s.subSequence(0, 5).toString(), "hello");
  }

  @Test
  public void testSwapCaseCharacters()
  {
    PyString s = context.str("Hello World");
    assertEquals(s.swapCaseCharacters().toString(), "hELLO wORLD");
  }

  @Test
  public void testToCapitalized()
  {
    PyString s = context.str("hello world");
    assertEquals(s.toCapitalized().toString(), "Hello world");
  }

  @Test
  public void testToCaseFolded()
  {
    PyString s = context.str("Hello");
    assertEquals(s.toCaseFolded().toString(), "hello");
  }

  @Test
  public void testToEncoded()
  {
    PyString s = context.str("hello");
    PyBytes b = s.toEncoded();
    assertEquals(b.size(), 5);
  }

  @Test
  public void testToTitleCase()
  {
    PyString s = context.str("hello world");
    assertEquals(s.toTitleCase().toString(), "Hello World");
  }

  @Test
  public void testToUppercase()
  {
    PyString s = context.str("hello");
    assertEquals(s.toUppercase().toString(), "HELLO");
  }

  @Test
  public void testZeroFill()
  {
    PyString s = context.str("42");
    assertEquals(s.zeroFill(5).toString(), "00042");
  }

  @Test
  public void testToStringMatchesJavaEquivalent()
  {
    PyString s = context.str("hello");
    assertEquals(s.toString(), "hello");
  }

}
