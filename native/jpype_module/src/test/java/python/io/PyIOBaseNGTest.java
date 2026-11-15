// --- file: python/io/PyIOBaseNGTest.java ---
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
package python.io;

import static org.testng.Assert.assertEquals;
import org.testng.annotations.Test;

/**
 * {@code PyIOBase} itself has only one non-abstract method:
 * {@code seek(long)}'s default {@code whence=0} overload. Like
 * {@code PyDeque.rotate()} (see python.collections finding), the real
 * reverse-bridge proxy dispatches {@code seek(offset)} straight to
 * Python's own {@code seek(offset, whence=0)} by mangled name, so this
 * Java-side default is never actually reached through any bridge test - a
 * plain non-bridge stub is the only way to exercise it.
 */
public class PyIOBaseNGTest
{

  private static final class RecordingIOBase implements PyIOBase
  {
    long lastOffset = Long.MIN_VALUE;
    int lastWhence = Integer.MIN_VALUE;

    @Override
    public long seek(long offset, int whence)
    {
      lastOffset = offset;
      lastWhence = whence;
      return offset;
    }

    @Override
    public python.lang.PyBuiltIn builtin()
    {
      throw new UnsupportedOperationException();
    }

    @Override
    public void close()
    {
      throw new UnsupportedOperationException();
    }

    @Override
    public boolean closed()
    {
      throw new UnsupportedOperationException();
    }

    @Override
    public void flush()
    {
      throw new UnsupportedOperationException();
    }

    @Override
    public boolean readable()
    {
      throw new UnsupportedOperationException();
    }

    @Override
    public boolean writable()
    {
      throw new UnsupportedOperationException();
    }

    @Override
    public boolean seekable()
    {
      throw new UnsupportedOperationException();
    }

    @Override
    public long tell()
    {
      throw new UnsupportedOperationException();
    }
  }

  @Test
  public void testSeekNoWhenceDefaultsToZero()
  {
    RecordingIOBase io = new RecordingIOBase();

    long result = io.seek(42L);

    assertEquals(io.lastOffset, 42L);
    assertEquals(io.lastWhence, 0);
    assertEquals(result, 42L);
  }
}
