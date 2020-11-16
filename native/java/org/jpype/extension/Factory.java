/** ***************************************************************************
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
 * See NOTICE file for details.
 **************************************************************************** */
package org.jpype.extension;

import java.io.IOException;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.jpype.JPypeContext;
import org.jpype.asm.ClassWriter;
import org.jpype.asm.MethodVisitor;
import org.jpype.asm.Opcodes;
import org.jpype.asm.Type;

/**
 *
 * @author nelson85
 */
public class Factory
{

  public static ClassDecl newClass(String name, Class[] bases)
  {
    return new ClassDecl(name, bases);
  }

  public static Class loadClass(ClassDecl decl)
  {
    Class base = null;
    List<Class> interfaces = new ArrayList<>();

    for (Class cls : decl.bases)
    {
      if (cls.isInterface())
      {
        interfaces.add(cls);
        continue;
      }

      // There can only be one base
      if (base != null)
        throw new RuntimeException("Multiple bases not allowed");

      // Base must not be final
      if (Modifier.isFinal(cls.getModifiers()))
        throw new RuntimeException("Cannot extend final class");

      // Select this as the base
      base = cls;
    }

    if (base == null)
    {
      base = Object.class;
    }

    // Write back to the decl for auditing.
    decl.setBase(base);
    decl.setInterfaces(interfaces);

    // Traverse all the bases to see what methods we are covering
    for (Class i : decl.bases)
    {
      // FIXME watch for methods that have already been implemented.    
      for (Method m : i.getMethods())
      {
        MethodDecl m3 = null;
        for (MethodDecl m2 : decl.methods)
          if (m2.matches(m))
          {
            m3 = m2;
            break;
          }

        if (m3 == null && Modifier.isAbstract(m.getModifiers()))
          throw new RuntimeException("Method " + m + " must be overriden");

        if (m3 != null)
          m3.bind(m);
      }
    }

    byte[] out = buildClass(decl);
    try
    {
      OutputStream fs = Files.newOutputStream(Paths.get("test.class"));
      fs.write(out);
      fs.close();
    } catch (IOException ex)
    {
      Logger.getLogger(Factory.class.getName()).log(Level.SEVERE, null, ex);
    }

    return null;
  }

  static byte[] buildClass(ClassDecl cdecl)
  {
    // Create the class
    ClassWriter cw = new ClassWriter(ClassWriter.COMPUTE_FRAMES);
    cdecl.internalName = "dynamic/" + cdecl.name;
    cw.visit(Opcodes.V1_7, Opcodes.ACC_PUBLIC, cdecl.internalName,
            null,
            Type.getInternalName(cdecl.base),
            cdecl.interfaces.stream().map(p -> Type.getInternalName(p))
                    .toArray(String[]::new));

    // Reserve space for parameter fields
    implementFields(cw, cdecl);

    for (MethodDecl mdecl : cdecl.methods)
    {
      implementMethod(cw, cdecl, mdecl);
    }

    cw.visitEnd();
    return cw.toByteArray();
  }

//<editor-fold desc="hooks" defaultstate="collapsed">
  /**
   * Hook to create a new instance of the object.
   *
   * This is called by the ctor of object to invoke __init__(self)
   */
  static native long _create(long context, long self);

  //
  static native Object _call(long context, long functionID, Object self,
          long returnType, long[] argsTypes, Object[] args);

  static native long _getDict(long context, long self);

//</editor_fold>
//<editor-fold desc="code generators" defaultstate="collapsed">
  private static void handleReturn(MethodVisitor mv, Class ret)
  {
    if (!ret.isPrimitive())
    {
      mv.visitTypeInsn(Opcodes.CHECKCAST, Type.getInternalName(ret));
      mv.visitInsn(Opcodes.ARETURN);
      return;
    }

    // Handle return
    if (ret == Void.TYPE)
    {
      mv.visitInsn(Opcodes.POP);
      mv.visitInsn(Opcodes.RETURN);
      return;
    }

    if (ret == Boolean.TYPE)
    {
      mv.visitTypeInsn(Opcodes.CHECKCAST, "java/lang/Boolean");
      mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL,
              "java/lang/Boolean",
              "booleanValue",
              "()Z",
              false);
      mv.visitInsn(Opcodes.IRETURN);
      return;
    }

    if (ret == Character.TYPE)
    {
      mv.visitTypeInsn(Opcodes.CHECKCAST, "java/lang/Character");
      mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL,
              "java/lang/Character",
              "charValue",
              "()C",
              false);
      mv.visitInsn(Opcodes.IRETURN);
      return;
    }

    mv.visitTypeInsn(Opcodes.CHECKCAST, "java/lang/Number");
    if (ret == Byte.TYPE)
    {
      mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL,
              "java/lang/Number",
              "byteValue",
              "()B",
              false);
      mv.visitInsn(Opcodes.IRETURN);
      return;
    }

    if (ret == Short.TYPE)
    {
      mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL,
              "java/lang/Number",
              "shortValue",
              "()S",
              false);
      mv.visitInsn(Opcodes.IRETURN);
      return;
    }

    if (ret == Integer.TYPE)
    {
      mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL,
              "java/lang/Number",
              "intValue",
              "()I",
              false);
      mv.visitInsn(Opcodes.IRETURN);
      return;
    }

    if (ret == Long.TYPE)
    {
      mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL,
              "java/lang/Number",
              "longValue",
              "()L",
              false);
      mv.visitInsn(Opcodes.LRETURN);
      return;
    }

    if (ret == Float.TYPE)
    {
      mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL,
              "java/lang/Number",
              "floatValue",
              "()F",
              false);
      mv.visitInsn(Opcodes.FRETURN);
      return;
    }

    if (ret == Double.TYPE)
    {
      mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL,
              "java/lang/Number",
              "doubleValue",
              "()D",
              false);
      mv.visitInsn(Opcodes.DRETURN);
      return;
    }

    // Unexpected failure
    throw new RuntimeException();
  }

  private static void implementFields(ClassWriter cw, ClassDecl decl)
  {
    int i = 0;
    for (MethodDecl mdecl : decl.methods)
    {
      mdecl.resolve();
      mdecl.parametersName = mdecl.name + "$" + i;
      i++;
      cw.visitField(Opcodes.ACC_PRIVATE | Opcodes.ACC_STATIC, mdecl.parametersName, "[J", null, null);
    }

    // Implement fields
    for (FieldDecl fdecl : decl.fields)
    {
      // FIXME initialize values
      cw.visitField(fdecl.modifiers, fdecl.name, Type.getDescriptor(fdecl.type), null, null);
    }

    {
      // Initialize the parameter lists
      MethodVisitor mv = cw.visitMethod(Opcodes.ACC_STATIC, "<clinit>", "()V", null, null);
      mv.visitCode();
      for (MethodDecl mdecl : decl.methods)
      {
        mv.visitIntInsn(Opcodes.BIPUSH, mdecl.parametersId.length);
        mv.visitIntInsn(Opcodes.NEWARRAY, Opcodes.T_LONG);
        mv.visitInsn(Opcodes.DUP); // two copies of array on stack
        for (int j = 0; j < mdecl.parametersId.length; ++j)
        {
          mv.visitIntInsn(Opcodes.BIPUSH, j);
          mv.visitLdcInsn(mdecl.parametersId[j]);
          mv.visitInsn(Opcodes.LASTORE);
          mv.visitInsn(Opcodes.DUP); // two copies of array on stack
        }
        mv.visitFieldInsn(Opcodes.PUTSTATIC, decl.internalName, mdecl.parametersName, "[J");
      }
      mv.visitEnd();
    }
  }

  private static void implementMethod(ClassWriter cw, ClassDecl cdecl, MethodDecl mdecl)
  {
    MethodVisitor mv = cw.visitMethod(mdecl.modifiers, mdecl.name, mdecl.descriptor(), null, null);
    // FIXME the exception information needs to go here.

    // Start the implementation
    mv.visitCode();

    //  static native Object _call(long context, long functionID, Object self,
    // long returnType, long[] argsTypes, Object[] args);
    // Place the interpretation information on the stack
    long context = JPypeContext.getInstance().getContext();
    mv.visitLdcInsn(context);
    mv.visitLdcInsn(mdecl.functionId);
    mv.visitIntInsn(Opcodes.ALOAD, 0);
    mv.visitLdcInsn(mdecl.retId);
    System.out.println(cdecl.internalName + " " + mdecl.parametersName + " " + Type.getDescriptor(long[].class));
    mv.visitFieldInsn(Opcodes.GETSTATIC, cdecl.internalName,
            mdecl.parametersName, "[J");

    // Pack the parameter array
    if (mdecl.parameters.length == 0)
    {
      mv.visitInsn(Opcodes.ACONST_NULL);
    } else
    {
      mv.visitIntInsn(Opcodes.BIPUSH, mdecl.parameters.length);
      mv.visitTypeInsn(Opcodes.ANEWARRAY, "java/lang/Object");
    }
    int k = 1;
    for (int j = 0; j < mdecl.parameters.length; ++j)
    {
      Class param = mdecl.parameters[j];
      mv.visitInsn(Opcodes.DUP); // two copies of thearray
      mv.visitIntInsn(Opcodes.BIPUSH, j);
      if (param.isPrimitive())
      {
        if (param == Boolean.TYPE)
        {
          mv.visitIntInsn(Opcodes.ILOAD, k++);
          mv.visitMethodInsn(Opcodes.INVOKESTATIC, "java/lang/Boolean", "valueOf", "(Z)Ljava/lang/Boolean;", false);
        } else if (param == Byte.TYPE)
        {
          mv.visitIntInsn(Opcodes.ILOAD, k++);
          mv.visitMethodInsn(Opcodes.INVOKESTATIC, "java/lang/Byte", "valueOf", "(B)Ljava/lang/Byte;", false);
        } else if (param == Character.TYPE)
        {
          mv.visitIntInsn(Opcodes.ILOAD, k++);
          mv.visitMethodInsn(Opcodes.INVOKESTATIC, "java/lang/Character", "valueOf", "(C)Ljava/lang/Character;", false);
        } else if (param == Short.TYPE)
        {
          mv.visitIntInsn(Opcodes.ILOAD, k++);
          mv.visitMethodInsn(Opcodes.INVOKESTATIC, "java/lang/Short", "valueOf", "(S)Ljava/lang/Short;", false);
        } else if (param == Integer.TYPE)
        {
          mv.visitIntInsn(Opcodes.ILOAD, k++);
          mv.visitMethodInsn(Opcodes.INVOKESTATIC, "java/lang/Integer", "valueOf", "(I)Ljava/lang/Integer;", false);
        } else if (param == Long.TYPE)
        {
          mv.visitIntInsn(Opcodes.LLOAD, k++);
          k++;  // bad design by Java.
          mv.visitMethodInsn(Opcodes.INVOKESTATIC, "java/lang/Long", "valueOf", "(L)Ljava/lang/Long;", false);
        } else if (param == Float.TYPE)
        {
          mv.visitIntInsn(Opcodes.FLOAD, k++);
          mv.visitMethodInsn(Opcodes.INVOKESTATIC, "java/lang/Float", "valueOf", "(L)Ljava/lang/Float;", false);
        } else if (param == Double.TYPE)
        {
          mv.visitIntInsn(Opcodes.DLOAD, k++);
          k++; // bad design by Java.
          mv.visitMethodInsn(Opcodes.INVOKESTATIC, "java/lang/Double", "valueOf", "(L)Ljava/lang/Double;", false);
        }
      } else
      {
        mv.visitIntInsn(Opcodes.ALOAD, k++);
      }
      mv.visitInsn(Opcodes.AASTORE);
    }

    // Call the hook in native
    mv.visitMethodInsn(Opcodes.INVOKESTATIC, "org/jpype/extension/Factory",
            "_call",
            "(JJLjava/lang/Object;J[J[Ljava/lang/Object;)Ljava/lang/Object;", false);

    // Process the return
    handleReturn(mv, mdecl.ret);
    mv.visitEnd();
  }
  //</editor-fold>
}

// FIXME how do we define what arguments are passed to the ctor?
// FIXME how do we define ctors
// FIXME how do we get to TypeError rather that RuntimeException?
// FIXME how do we keep from clobbering.
// FIXME how does the type system know that this is an extension class?
