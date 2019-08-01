/*
 * Copyright 2018, Karl Nelson
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
package org.jpype.pickle;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

/**
 *
 * @author Karl Einar Nelson
 */
public class ByteBufferInputStream extends InputStream
{
  ByteBuffer bb = ByteBuffer.allocate(1024);
  int loaded = 0;

  public void put(byte[] bytes)
  {
    // If we have additional capacity, use it
    if (bytes.length < bb.remaining())
    {
      int p = bb.position();
      bb.position(loaded);
      bb.put(bytes);
      loaded = bb.position();
      bb.position(p);
      return;
    }

    // Okay we may need to allocate more
    ByteBuffer bb2 = bb;
    int r = loaded - bb.position();

    // If we don't have space, make a new buffer.
    if (r + bytes.length > bb.capacity())
      bb = ByteBuffer.allocate(r + bytes.length);

    // If we have remaining bytes, then keep them
    if (r > 0)
    {
      bb.put(bb2.array(), bb2.position(), r);
    }

    // Add the new data
    bb.put(bytes);
    loaded = bb.position();
    bb.position(0);
  }

  public int available()
  {
    int out = loaded - bb.position();
    return out;
  }

  @Override
  public int read() throws IOException
  {
    int r = loaded - bb.position();
    if (r > 0)
    {
      int p = bb.get();
      return p;
    }
    return -1;
  }

  @Override
  public int read(byte[] arg0) throws IOException
  {
    int r = loaded - bb.position();
    if (arg0.length <= r)
    {
      bb.get(arg0);
      return arg0.length;
    }

    bb.get(arg0, 0, r);
    return r;
  }

  @Override
  public int read(byte[] buffer, int offset, int len) throws IOException
  {
    int r = loaded - bb.position();
    if (r==0)
      return -1;

    if (len> r)
    {
      len = r;
    }

    bb.get(buffer, offset, len);
    return len;
  }
}
