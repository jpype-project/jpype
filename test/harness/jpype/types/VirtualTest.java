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
package jpype.types;

public class VirtualTest
{

  public interface ObjectSupplier
  {

    Object get();
  }

  public static class ClassObjectSupplier implements ObjectSupplier
  {

    public Object get()
    {
      return "implements";
    }
  }

  public static class ClassObjectSupplier2 extends ClassObjectSupplier
  {

    public Object get()
    {
      return "extends";
    }
  }

  public static ObjectSupplier getObjectImplements()
  {
    return new ClassObjectSupplier();
  }

  public static ObjectSupplier getObjectExtends()
  {
    return new ClassObjectSupplier2();
  }

  public static ObjectSupplier getObjectAnon()
  {
    return new ObjectSupplier()
    {
      public Object get()
      {
        return "anon";
      }
    };
  }

  public static ObjectSupplier getObjectAnonExtends()
  {
    return new ClassObjectSupplier()
    {
      public Object get()
      {
        return "anon-override";
      }
    };
  }

// This one has to wait for us to abandon Java 7
//  public static ObjectSupplier getLambda()
//  {
//    return () -> "lambda";
//  }
//<editor-fold desc="int" defaultstate="collapsed">
  public interface IntegerSupplier
  {

    int get();
  }

  public static class ClassIntegerSupplier implements IntegerSupplier
  {

    public int get()
    {
      return 1;
    }
  }

  public static class ClassIntegerSupplier2 extends ClassIntegerSupplier
  {

    public int get()
    {
      return 2;
    }
  }

  public static IntegerSupplier getIntegerImplements()
  {
    return new ClassIntegerSupplier();
  }

  public static IntegerSupplier getIntegerExtends()
  {
    return new ClassIntegerSupplier2();
  }

  public static IntegerSupplier getIntegerAnon()
  {
    return new IntegerSupplier()
    {
      public int get()
      {
        return 3;
      }
    };
  }

  public static IntegerSupplier getIntegerAnonExtends()
  {
    return new ClassIntegerSupplier()
    {
      public int get()
      {
        return 4;
      }
    };
  }
//</editor-fold>
//<editor-fold desc="short" defaultstate="collapsed">

  public interface ShortSupplier
  {

    short get();
  }

  public static class ClassShortSupplier implements ShortSupplier
  {

    public short get()
    {
      return 1;
    }
  }

  public static class ClassShortSupplier2 extends ClassShortSupplier
  {

    public short get()
    {
      return 2;
    }
  }

  public static ShortSupplier getShortImplements()
  {
    return new ClassShortSupplier();
  }

  public static ShortSupplier getShortExtends()
  {
    return new ClassShortSupplier2();
  }

  public static ShortSupplier getShortAnon()
  {
    return new ShortSupplier()
    {
      public short get()
      {
        return 3;
      }
    };
  }

  public static ShortSupplier getShortAnonExtends()
  {
    return new ClassShortSupplier()
    {
      public short get()
      {
        return 4;
      }
    };
  }
//</editor-fold>
//<editor-fold desc="long" defaultstate="collapsed">

  public interface LongSupplier
  {

    long get();
  }

  public static class ClassLongSupplier implements LongSupplier
  {

    public long get()
    {
      return 1;
    }
  }

  public static class ClassLongSupplier2 extends ClassLongSupplier
  {

    public long get()
    {
      return 2;
    }
  }

  public static LongSupplier getLongImplements()
  {
    return new ClassLongSupplier();
  }

  public static LongSupplier getLongExtends()
  {
    return new ClassLongSupplier2();
  }

  public static LongSupplier getLongAnon()
  {
    return new LongSupplier()
    {
      public long get()
      {
        return 3;
      }
    };
  }

  public static LongSupplier getLongAnonExtends()
  {
    return new ClassLongSupplier()
    {
      public long get()
      {
        return 4;
      }
    };
  }
//</editor-fold>
//<editor-fold desc="float" defaultstate="collapsed">

  public interface FloatSupplier
  {

    float get();
  }

  public static class ClassFloatSupplier implements FloatSupplier
  {

    public float get()
    {
      return 1;
    }
  }

  public static class ClassFloatSupplier2 extends ClassFloatSupplier
  {

    public float get()
    {
      return 2;
    }
  }

  public static FloatSupplier getFloatImplements()
  {
    return new ClassFloatSupplier();
  }

  public static FloatSupplier getFloatExtends()
  {
    return new ClassFloatSupplier2();
  }

  public static FloatSupplier getFloatAnon()
  {
    return new FloatSupplier()
    {
      public float get()
      {
        return 3;
      }
    };
  }

  public static FloatSupplier getFloatAnonExtends()
  {
    return new ClassFloatSupplier()
    {
      public float get()
      {
        return 4;
      }
    };
  }
//</editor-fold>
//<editor-fold desc="double" defaultstate="collapsed">

  public interface DoubleSupplier
  {

    double get();
  }

  public static class ClassDoubleSupplier implements DoubleSupplier
  {

    public double get()
    {
      return 1;
    }
  }

  public static class ClassDoubleSupplier2 extends ClassDoubleSupplier
  {

    public double get()
    {
      return 2;
    }
  }

  public static DoubleSupplier getDoubleImplements()
  {
    return new ClassDoubleSupplier();
  }

  public static DoubleSupplier getDoubleExtends()
  {
    return new ClassDoubleSupplier2();
  }

  public static DoubleSupplier getDoubleAnon()
  {
    return new DoubleSupplier()
    {
      public double get()
      {
        return 3;
      }
    };
  }

  public static DoubleSupplier getDoubleAnonExtends()
  {
    return new ClassDoubleSupplier()
    {
      public double get()
      {
        return 4;
      }
    };
  }
//</editor-fold>
//<editor-fold desc="byte" defaultstate="collapsed">

  public interface ByteSupplier
  {

    byte get();
  }

  public static class ClassByteSupplier implements ByteSupplier
  {

    public byte get()
    {
      return 1;
    }
  }

  public static class ClassByteSupplier2 extends ClassByteSupplier
  {

    public byte get()
    {
      return 2;
    }
  }

  public static ByteSupplier getByteImplements()
  {
    return new ClassByteSupplier();
  }

  public static ByteSupplier getByteExtends()
  {
    return new ClassByteSupplier2();
  }

  public static ByteSupplier getByteAnon()
  {
    return new ByteSupplier()
    {
      public byte get()
      {
        return 3;
      }
    };
  }

  public static ByteSupplier getByteAnonExtends()
  {
    return new ClassByteSupplier()
    {
      public byte get()
      {
        return 4;
      }
    };
  }
//</editor-fold>
//<editor-fold desc="char" defaultstate="collapsed">

  public interface CharSupplier
  {

    char get();
  }

  public static class ClassCharSupplier implements CharSupplier
  {

    public char get()
    {
      return '1';
    }
  }

  public static class ClassCharSupplier2 extends ClassCharSupplier
  {

    public char get()
    {
      return '2';
    }
  }

  public static CharSupplier getCharImplements()
  {
    return new ClassCharSupplier();
  }

  public static CharSupplier getCharExtends()
  {
    return new ClassCharSupplier2();
  }

  public static CharSupplier getCharAnon()
  {
    return new CharSupplier()
    {
      public char get()
      {
        return '3';
      }
    };
  }

  public static CharSupplier getCharAnonExtends()
  {
    return new ClassCharSupplier()
    {
      public char get()
      {
        return '4';
      }
    };
  }
//</editor-fold>
//<editor-fold desc="boolean" defaultstate="collapsed">

  public interface BooleanSupplier
  {

    boolean get();
  }

  public static class ClassBooleanSupplier implements BooleanSupplier
  {

    public boolean get()
    {
      return true;
    }
  }

  public static class ClassBooleanSupplier2 extends ClassBooleanSupplier
  {

    public boolean get()
    {
      return false;
    }
  }

  public static BooleanSupplier getBooleanImplements()
  {
    return new ClassBooleanSupplier();
  }

  public static BooleanSupplier getBooleanExtends()
  {
    return new ClassBooleanSupplier2();
  }

  public static BooleanSupplier getBooleanAnon()
  {
    return new BooleanSupplier()
    {
      public boolean get()
      {
        return false;
      }
    };
  }

  public static BooleanSupplier getBooleanAnonExtends()
  {
    return new ClassBooleanSupplier()
    {
      public boolean get()
      {
        return false;
      }
    };
  }
//</editor-fold>
//<editor-fold desc="void" defaultstate="collapsed">
// Yes, we have to do this one as well. All paths need exercise.

  public interface VoidSupplier
  {

    void get();
  }

  public static class ClassVoidSupplier implements VoidSupplier
  {

    public void get()
    {
      return;
    }
  }

  public static class ClassVoidSupplier2 extends ClassVoidSupplier
  {

    public void get()
    {
      return;
    }
  }

  public static VoidSupplier getVoidImplements()
  {
    return new ClassVoidSupplier();
  }

  public static VoidSupplier getVoidExtends()
  {
    return new ClassVoidSupplier2();
  }

  public static VoidSupplier getVoidAnon()
  {
    return new VoidSupplier()
    {
      public void get()
      {
        return;
      }
    };
  }

  public static VoidSupplier getVoidAnonExtends()
  {
    return new ClassVoidSupplier()
    {
      public void get()
      {
        return;
      }
    };
  }
//</editor-fold>
}
