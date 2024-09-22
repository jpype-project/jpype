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

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

import org.jpype.JPypeContext;
import org.jpype.asm.Type;
import org.jpype.manager.TypeManager;

/**
 *
 * @author nelson85
 */
public class MethodDecl {

	public final String name;
	public final Parameter ret;
	public final Parameter[] parameters;
	public final Class<?>[] exceptions;
	public final int modifiers;
	public Method method;
	public long retId;
	public long[] parametersId;
	public String parametersName;
	public final long id;

	public MethodDecl(String name, Class<?> ret, Class<?>[] params, Class<?>[] exc, int mods, int id) {
		this.name = name;
		if (ret == null) {
			ret = Void.TYPE;
		}
		this.ret = new Parameter(ret, -1);
		int slot = Modifier.isStatic(mods) ? 0 : 1;
		this.parameters = new Parameter[params.length];
		for (int i = 0; i < params.length; i++) {
			Parameter param = new Parameter(params[i], slot);
			this.parameters[i] = param;
			slot = param.getNextSlot();
		}
		this.exceptions = exc;
		this.modifiers = mods;
		this.id = id;
	}

	boolean matches(Method m) {
		if (!m.getName().equals(name)) {
			return false;
		}

		Class<?>[] param2 = m.getParameterTypes();
		if (param2.length != parameters.length) {
			return false;
		}

		for (int i = 0; i < param2.length; i++) {
			if (param2[i] != parameters[i].type) {
				return false;
			}
		}

		// The JVM does not care about exceptions at runtime.
		// They still need to be added to the method declaration in case the user
		// has a situation where it's needed for some framework.

		if (ret.kind == TypeKind.OBJECT) {
			return m.getReturnType().isAssignableFrom(ret.type);
		}

		return ret.type == m.getReturnType();
	}

	void bind(Method m) {
		this.method = m;
	}

	void resolve() {
		TypeManager typemanager = JPypeContext.getInstance().getTypeManager();
		retId = typemanager.findClass(ret.type);
		this.parametersId = new long[this.parameters.length];
		for (int i = 0; i < this.parameters.length; ++i) {
			this.parametersId[i] = typemanager.findClass(parameters[i].type);
		}
	}

	String descriptor() {
		StringBuilder sb = new StringBuilder();
		sb.append('(');
		for (Parameter param : this.parameters) {
			sb.append(Type.getDescriptor(param.type));
		}
		sb.append(')');
		sb.append(Type.getDescriptor(ret.type));
		return sb.toString();
	}

}
