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

/**
 *
 * @author Karl Einar Nelson
 */
public class ByteBufferInputStream extends InputStream
{
  private LinkedList<ByteBuffer> buffers = new LinkedList<>();
  private byte[] one_byte = new byte[1];

  public void put(byte[] bytes)
  {
    ByteBuffer buffer = ByteBuffer.wrap(bytes); // TODO: check if we can just wrap the buffer instead of copying it.
    buffer.put(bytes);
    buffer.flip();
    buffers.add(buffer);
    System.out.println("buffers length: " + buffer.capacity());
  }

  @Override
  public int read() throws IOException
  {
    return read(one_byte) == 1 ? one_byte[0] : -1;
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
    {
        throw new NullPointerException("Buffer cannot be null");
    }
    if (offset < 0 || len < 0 || len > buffer.length - offset)
    {
        throw new IndexOutOfBoundsException("Invalid offset/length parameters");
    }
    if (len == 0)
    {
        return 0;
    }

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
        one_byte = null;
    }
}
