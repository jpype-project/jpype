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
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.jpype.JPypeContext;
import org.jpype.asm.AnnotationVisitor;
import org.jpype.asm.ClassWriter;
import org.jpype.asm.FieldVisitor;
import org.jpype.asm.MethodVisitor;
import org.jpype.asm.Opcodes;
import org.jpype.asm.Type;
import org.jpype.extension.AnnotationDecl.ValueHelper;
import org.jpype.manager.ClassDescriptor;
import org.jpype.manager.TypeManager;

/**
 * This is used to create an extension class.
 *
 * The process starts by having Python class marked up with declarations. When
 * then metaclass JClassBase sees the class it transfers the important annotated
 * methods to the ClassDecl. It then calls the factory to instantiate the new
 * class.
 *
 * @author nelson85
 */
public class Factory {

	private static final Type CONTEXT_TYPE = Type.getType(JPypeContext.class);
	private static final Type TYPE_MANAGER_TYPE = Type.getType(TypeManager.class);
	private static final Type FACTORY_TYPE = Type.getType(Factory.class);
	private static final String CALL_DESCRIPTOR = "(JJ[Ljava/lang/Object;)Ljava/lang/Object;";
	private static final String FIND_CLASS_DESCRIPTOR = "(Ljava/lang/Class;)J";
	private static final String JCLASS_FIELD = "$jclass";
	private static final String INSTANCE_FIELD = "$instance";

	public static boolean isExtension(Class<?> cls) {
		if (cls == null) {
			return false;
		}
		try {
			Field[] fields = cls.getDeclaredFields();
			if (fields.length < 1) {
				return false;
			}
			// this will always be the first field
			return fields[0].getName().equals(JCLASS_FIELD);
		} catch (Throwable t) {}
		return false;
	}

	public static ClassLoader getNewExtensionClassLoader() {
		return new ExtensionClassLoader();
	}

	public static boolean isExtensionField(Field field) {
		if (field == null) {
			return false;
		}
		String name = field.getName();
		return name.equals(JCLASS_FIELD) || name.equals(INSTANCE_FIELD);
	}

	private static native Object _call(long ctx, long id, Object[] args);

	//<editor-fold desc="hooks" defaultstate="collapsed">
	/**
	 * Hook to call a Python implemented method
	 */
	public static Object call(long ctx, long id, Object... args) throws InstantiationException {
		if (ctx == 0) {
			StackTraceElement ste = Thread.currentThread().getStackTrace()[2];
            String cls = ste.getClassName();
			if (ste.getMethodName().equals("<init>")) {
				throw new InstantiationException(cls + " has been collected");
			}
			throw new IllegalStateException(cls + " has been collected");
		}
		return _call(ctx, id, args);
	}

	//</editor_fold>

	/**
	 * Start a new class declaration.
	 *
	 * @param name is the name of the nee class.
	 * @param bases is a list of the bases for this class containing no more than
	 * one base class.
	 * @param ldr the ExtensionClassLoader to use for the class or null for the builtin loader.
	 * @return a new class declaration.
	 */
	public static ClassDecl newClass(String name, Class<?>[] bases, ExtensionClassLoader ldr) {
		return new ClassDecl(name, bases, ldr);
	}

	public static long loadClass(ClassDecl decl) {
		decl.internalName = "dynamic/" + decl.name.replace('.', '/');
		try {
			String name = decl.internalName.replace('/', '.');
			Class<?> cls = Class.forName(name, false, decl.ldr);

			// Oh joy, someone decided to use importlib.reload.
			// While it is a niche edge case, we can and should
			// handle it gracefully.

			// To handle this, we get the existing JPClass, release
			// the old host Python type and return the old JPClass.
			// This allows a new Python type to be created and allows
			// the old one float into the dark abyss.
			// Whether or not it will see the light of day again
			// depends on what references still exist and whoever decided
			// to do this. It will most definetely leak something, probably,
			// but if you didn't want a leak, then don't do that...

			ClassDescriptor desc = JPypeContext.getInstance().getTypeManager().classMap.get(cls);
			ExtensionClassLoader.clearHost(desc.classPtr);
			return desc.classPtr;
		} catch (ClassNotFoundException e) {
			// not yet defined
		}

		Class<?> base = null;
		List<Class<?>> interfaces = new ArrayList<>();

		for (Class<?> cls : decl.bases) {
			if (cls.isInterface()) {
				interfaces.add(cls);
				continue;
			}

			// There can only be one base
			if (base != null) {
				throw new RuntimeException("Multiple bases not allowed");
			}

			// Base must not be final
			if (Modifier.isFinal(cls.getModifiers())) {
				throw new RuntimeException("Cannot extend final class");
			}

			// Select this as the base
			base = cls;
		}

		if (base == null) {
			base = Object.class;
		}

		// Write back to the decl for auditing.
		decl.setBase(base);
		decl.setInterfaces(interfaces);

		// Traverse all the bases to see what methods we are covering
		for (Class<?> i : decl.bases) {
			for (Method m : i.getMethods()) {
				MethodDecl m3 = null;
				for (MethodDecl m2 : decl.methods) {
					if (m2.matches(m)) {
						m3 = m2;
						break;
					}
				}

				if (m3 == null && Modifier.isAbstract(m.getModifiers())) {
					throw new RuntimeException("Method " + m + " must be overriden");
				}

				if (m3 != null) {
					m3.bind(m);
				}
			}
		}

		byte[] out = buildClass(decl);
		try {
			OutputStream fs = Files.newOutputStream(Paths.get("test.class"));
			fs.write(out);
			fs.close();
		} catch (IOException ex) {
			Logger.getLogger(Factory.class.getName()).log(Level.SEVERE, null, ex);
		}

		try {
			String name = decl.internalName.replace('/', '.');
			Class<?> res = decl.ldr.loadClass(name, out);
			decl.ldr = null;
			for (MethodDecl method : decl.methods) {
				// resolve must occur AFTER class creation
				method.resolve();
			}
			return JPypeContext.getInstance().getTypeManager().findClass(res);
		} catch (Exception ex) {
			Logger.getLogger(Factory.class.getName()).log(Level.SEVERE, null, ex);
		}

		return 0;
	}

	static byte[] buildClass(ClassDecl cdecl) {
		// Create the class
		ClassWriter cw = new ClassWriter(ClassWriter.COMPUTE_FRAMES);
		cw.visit(Opcodes.V1_7, Opcodes.ACC_PUBLIC, cdecl.internalName,
			null,
			Type.getInternalName(cdecl.base),
			cdecl.interfaces.stream()
					.map(Type::getInternalName)
					.toArray(String[]::new));

		addAnnotations(cw, cdecl);

		// Reserve space for parameter fields
		implementFields(cw, cdecl);

		for (MethodDecl mdecl : cdecl.methods) {
			if (mdecl.name.equals("<init>")) {
				implementCtor(cw, cdecl, mdecl);
			} else {
				implementMethod(cw, cdecl, mdecl);
			}
		}

		cw.visitEnd();
		return cw.toByteArray();
	}

	private static void addAnnotations(ClassWriter cw, ClassDecl cdecl) {
		for (AnnotationDecl annotation : cdecl.annotations) {
			AnnotationVisitor av = cw.visitAnnotation(annotation.getDescriptor(), true);
			for (Map.Entry<String, ValueHelper> entry : annotation.elements.entrySet()) {
				av.visit(entry.getKey(), entry.getValue().value);
			}
			av.visitEnd();
		}
	}

	private static void addAnnotations(FieldVisitor fv, FieldDecl fdecl) {
		for (AnnotationDecl annotation : fdecl.annotations) {
			AnnotationVisitor av = fv.visitAnnotation(annotation.getDescriptor(), true);
			for (Map.Entry<String, ValueHelper> entry : annotation.elements.entrySet()) {
				av.visit(entry.getKey(), entry.getValue().value);
			}
			av.visitEnd();
		}
	}

	private static void addAnnotations(MethodVisitor mv, MethodDecl mdecl) {
		for (AnnotationDecl annotation : mdecl.annotations) {
			AnnotationVisitor av = mv.visitAnnotation(annotation.getDescriptor(), true);
			for (Map.Entry<String, ValueHelper> entry : annotation.elements.entrySet()) {
				av.visit(entry.getKey(), entry.getValue().value);
			}
			av.visitEnd();
		}
		for (int i = 0; i < mdecl.parameters.length; i++) {
			ParameterDecl param = mdecl.parameters[i];
			addAnnotations(mv, param, i);
		}
	}

	private static void addAnnotations(MethodVisitor mv, ParameterDecl decl, int index) {
		for (AnnotationDecl annotation : decl.annotations) {
			AnnotationVisitor av = mv.visitParameterAnnotation(
				index, annotation.getDescriptor(), true);
			for (Map.Entry<String, ValueHelper> entry : annotation.elements.entrySet()) {
				av.visit(entry.getKey(), entry.getValue().value);
			}
			av.visitEnd();
		}
	}


	//<editor-fold desc="code generators" defaultstate="collapsed">
	private static void implementFields(ClassWriter cw, ClassDecl decl) {
		// create a static private field to hold the pointer to our JPClass
		cw.visitField(
			Opcodes.ACC_PRIVATE | Opcodes.ACC_STATIC | Opcodes.ACC_FINAL | Opcodes.ACC_SYNTHETIC,
			JCLASS_FIELD,
			"J",
			null,
			null
		);

		if (!isExtension(decl.base)) {
			// create a private field to hold the pointer to our Python object
			cw.visitField(
				Opcodes.ACC_PRIVATE | Opcodes.ACC_FINAL | Opcodes.ACC_SYNTHETIC,
				INSTANCE_FIELD,
				"J",
				null,
				null
			);
		}

		// Implement fields
		for (FieldDecl fdecl : decl.fields) {
			FieldVisitor fv = cw.visitField(
				fdecl.modifiers, fdecl.name, Type.getDescriptor(fdecl.type), null, fdecl.value);
			addAnnotations(fv, fdecl);
			fv.visitEnd();
		}

		// Initialize the parameter lists
		MethodVisitor mv = cw.visitMethod(Opcodes.ACC_STATIC, "<clinit>", "()V", null, null);
		mv.visitCode();
		String context = CONTEXT_TYPE.getInternalName();
		Type type = decl.getType();
		mv.visitMethodInsn(Opcodes.INVOKESTATIC, context, "getInstance", "()Lorg/jpype/JPypeContext;", false);
		mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL, context, "getTypeManager", "()Lorg/jpype/manager/TypeManager;", false);
		mv.visitLdcInsn(type);
		mv.visitMethodInsn(
			Opcodes.INVOKEVIRTUAL,
			TYPE_MANAGER_TYPE.getInternalName(),
			"findClass",
			"(Ljava/lang/Class;)J",
			false
		);
		mv.visitFieldInsn(Opcodes.PUTSTATIC, type.getInternalName(), JCLASS_FIELD, "J");
		mv.visitInsn(Opcodes.RETURN);
		mv.visitMaxs(1, 1);
		mv.visitEnd();
	}

	private static void implementCtor(ClassWriter cw, ClassDecl cdecl, MethodDecl mdecl) {
		// Copy over exceptions
		String[] exceptions = null;
		if (mdecl.exceptions != null) {
			exceptions = new String[mdecl.exceptions.length];
			for (int i = 0; i < mdecl.exceptions.length; ++i) {
				exceptions[i] = Type.getInternalName(mdecl.exceptions[i]);
			}
		}

		// Start a new method
		MethodVisitor mv =
			cw.visitMethod(mdecl.modifiers, mdecl.name, mdecl.descriptor(), null, exceptions);

		addAnnotations(mv, mdecl);

		mv.visitCode();

		// forward parameters
		mv.visitVarInsn(Opcodes.ALOAD, 0);
		for (ParameterDecl param : mdecl.parameters) {
			mv.visitVarInsn(param.kind.load, param.slot);
		}

		// call super
		mv.visitMethodInsn(
			Opcodes.INVOKESPECIAL,
			Type.getInternalName(cdecl.base),
			"<init>",
			mdecl.descriptor(),
			false
		);

		callPython(mv, cdecl, mdecl);
	}

	private static void implementMethod(ClassWriter cw, ClassDecl cdecl, MethodDecl mdecl) {
		// Copy over exceptions
		String[] exceptions = null;
		if (mdecl.exceptions != null) {
			exceptions = new String[mdecl.exceptions.length];
			for (int i = 0; i < mdecl.exceptions.length; ++i) {
				exceptions[i] = Type.getInternalName(mdecl.exceptions[i]);
			}
		}

		// Start a new method
		MethodVisitor mv =
			cw.visitMethod(mdecl.modifiers, mdecl.name, mdecl.descriptor(), null, exceptions);

		addAnnotations(mv, mdecl);

		// Start the implementation
		mv.visitCode();

		callPython(mv, cdecl, mdecl);
	}
	//</editor-fold>

	private static void callPython(MethodVisitor mv, ClassDecl cdecl, MethodDecl mdecl) {
		// Object _call(long ctx, long id, Object[] args);
		// Place the interpretation information on the stack
		mv.visitFieldInsn(Opcodes.GETSTATIC, cdecl.internalName, JCLASS_FIELD, "J");
		mv.visitLdcInsn(mdecl.id);

		// Create the parameter array
		mv.visitLdcInsn(mdecl.parameters.length + 1);
		mv.visitTypeInsn(Opcodes.ANEWARRAY, "java/lang/Object");
		mv.visitInsn(Opcodes.DUP); // two copies of the array reference
		mv.visitInsn(Opcodes.ICONST_0);
		if (mdecl.isStatic()) {
			mv.visitLdcInsn(cdecl.getType());
		} else {
			mv.visitIntInsn(Opcodes.ALOAD, 0);
		}
		mv.visitInsn(Opcodes.AASTORE);

		// Marshal the parameters
		for (int i = 0; i < mdecl.parameters.length; i++) {
			mv.visitInsn(Opcodes.DUP);
			loadConst(mv, i+1);
			load(mv, mdecl.parameters[i]);
			mv.visitInsn(Opcodes.AASTORE);
		}

		// Call the hook in native
		mv.visitMethodInsn(
			Opcodes.INVOKESTATIC,
			FACTORY_TYPE.getInternalName(),
			"call",
			CALL_DESCRIPTOR,
			false
		);

		// Process the return
		handleReturn(mv, mdecl.ret);

		// fix the stack
		mv.visitMaxs(1, 1);

		// Close the method
		mv.visitEnd();
	}

	private static void loadConst(MethodVisitor mv, int value) {
		switch (value) {
			case 0:
				mv.visitInsn(Opcodes.ICONST_0);
				break;
			case 1:
				mv.visitInsn(Opcodes.ICONST_1);
				break;
			case 2:
				mv.visitInsn(Opcodes.ICONST_2);
				break;
			case 3:
				mv.visitInsn(Opcodes.ICONST_3);
				break;
			case 4:
				mv.visitInsn(Opcodes.ICONST_4);
				break;
			case 5:
				mv.visitInsn(Opcodes.ICONST_5);
				break;
			default:
				mv.visitLdcInsn(value);
				break;
		}
	}

	private static void load(MethodVisitor mv, ParameterDecl param) {
		if (param.kind == TypeKind.OBJECT) {
			mv.visitIntInsn(Opcodes.ALOAD, param.slot);
		} else {
			mv.visitIntInsn(param.kind.load, param.slot);
			mv.visitMethodInsn(
				Opcodes.INVOKESTATIC,
				param.kind.boxedClass,
				"valueOf",
				param.kind.boxDescriptor,
				false
			);
		}
	}

	private static void handleReturn(MethodVisitor mv, ParameterDecl ret) {
		String name;
		String desc;
		int op;

		switch (ret.kind) {
			case OBJECT:
				mv.visitTypeInsn(Opcodes.CHECKCAST, Type.getInternalName(ret.type));
				mv.visitInsn(Opcodes.ARETURN);
				return;
			case VOID:
				mv.visitInsn(Opcodes.RETURN);
				return;
			case BOOL:
				name = "booleanValue";
				desc = "()Z";
				op = Opcodes.IRETURN;
				break;
			case BYTE:
				name = "byteValue";
				desc = "()B";
				op = Opcodes.IRETURN;
				break;
			case CHAR:
				name = "charValue";
				desc = "()C";
				op = Opcodes.IRETURN;
				break;
			case SHORT:
				name = "shortValue";
				desc = "()S";
				op = Opcodes.IRETURN;
				break;
			case INT:
				name = "intValue";
				desc = "()I";
				op = Opcodes.IRETURN;
				break;
			case LONG:
				name = "longValue";
				desc = "()L";
				op = Opcodes.LRETURN;
				break;
			case FLOAT:
				name = "floatValue";
				desc = "()F";
				op = Opcodes.FRETURN;
				break;
			case DOUBLE:
				name = "doubleValue";
				desc = "()D";
				op = Opcodes.DRETURN;
				break;
			default:
				// without the default the compiler thinks some locals are uninitialized
				throw new RuntimeException();
		}

		mv.visitTypeInsn(Opcodes.CHECKCAST, ret.kind.boxedClass);
		mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL, ret.kind.boxedClass, name, desc, false);
		mv.visitInsn(op);
	}
}
