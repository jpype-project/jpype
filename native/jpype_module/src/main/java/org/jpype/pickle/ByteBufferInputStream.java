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
package org.jpype.pickle;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.LinkedList;

public class ByteBufferInputStream extends InputStream
{

  private LinkedList<ByteBuffer> buffers = new LinkedList<>();

  public void put(byte[] bytes)
  {
    // We can just wrap the buffer instead of copying it, since the buffer is
    // wrapped we don't need to write the bytes to it. Wrapping the bytes relies
    // on the array not being changed before being read.
    ByteBuffer buffer = ByteBuffer.wrap(bytes);
    buffers.add(buffer);
  }

  @Override
  public int read() throws IOException
  {
    if (buffers.isEmpty())
      return -1;

    ByteBuffer b = buffers.getFirst();
    while (b.remaining() == 0)
    {
      buffers.removeFirst();
      if (buffers.isEmpty())
        return -1; // EOF
      b = buffers.getFirst();
    }
    return b.get() & 0xFF; // Mask with 0xFF to convert signed byte to int (range 0-255)
  }

  @Override
  public int read(byte[] arg0) throws IOException
  {
    return read(arg0, 0, arg0.length);
  }

  @Override
  public int read(byte[] buffer, int offset, int len) throws IOException
  {
    if (buffer == null)
      throw new NullPointerException("Buffer cannot be null");
    if (offset < 0 || len < 0 || len > buffer.length - offset)
      throw new IndexOutOfBoundsException("Invalid offset/length parameters");
    if (len == 0)
      return 0;

    int total = 0;
    while (len > 0 && !buffers.isEmpty())
    {
      ByteBuffer b = buffers.getFirst();
      int remaining = b.remaining();
      if (remaining == 0)
      {
        buffers.removeFirst();
        continue;
      }

      int toRead = Math.min(len, remaining);
      b.get(buffer, offset, toRead);
      total += toRead;
      len -= toRead;
      offset += toRead;
    }
    return (total == 0) ? -1 : total;
  }

  @Override
  public void close() throws IOException
  {
    buffers.clear();
  }
}
