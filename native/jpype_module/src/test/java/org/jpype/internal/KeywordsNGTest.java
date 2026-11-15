// --- file: org/jpype/internal/KeywordsNGTest.java ---
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
package org.jpype.internal;

import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class KeywordsNGTest
{

  // A synthetic keyword unlikely to collide with any real Python keyword
  // that the interpreter bootstrap may have already registered into the
  // shared static Keywords.keywords set elsewhere in this JVM.
  private static final String TEST_KEYWORD = "jpype_test_keyword_xyz";

  @Test
  public void testWrapNonKeywordUnchanged()
  {
    assertEquals(Keywords.wrap("not_a_keyword_xyz"), "not_a_keyword_xyz");
  }

  @Test
  public void testWrapAndUnwrapRoundTrip()
  {
    Keywords.setKeywords(new String[]
    {
      TEST_KEYWORD
    });
    assertTrue(Keywords.keywords.contains(TEST_KEYWORD));
    String wrapped = Keywords.wrap(TEST_KEYWORD);
    assertEquals(wrapped, TEST_KEYWORD + "_");
    assertEquals(Keywords.unwrap(wrapped), TEST_KEYWORD);
  }

  @Test
  public void testUnwrapNameNotEndingInUnderscoreUnchanged()
  {
    assertEquals(Keywords.unwrap("plain_name"), "plain_name");
  }

  @Test
  public void testUnwrapTrailingUnderscoreThatIsNotAKeywordUnchanged()
  {
    // Ends with "_" but the part before it was never registered as a keyword.
    assertEquals(Keywords.unwrap("jpype_test_not_registered_"), "jpype_test_not_registered_");
  }

  @Test
  public void testSafepkgWithoutUnderscoreUnchanged()
  {
    assertEquals(Keywords.safepkg("python.lang"), "python.lang");
  }

  @Test
  public void testSafepkgUnwrapsEachDottedPart()
  {
    Keywords.setKeywords(new String[]
    {
      TEST_KEYWORD
    });
    String wrapped = TEST_KEYWORD + "_";
    assertEquals(Keywords.safepkg("a." + wrapped + ".b"), "a." + TEST_KEYWORD + ".b");
  }

}
