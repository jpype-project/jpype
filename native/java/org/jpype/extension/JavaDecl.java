package org.jpype.extension;

import java.util.Collections;
import java.util.List;

abstract class JavaDecl {
	List<AnnotationDecl> annotations = Collections.emptyList();

	public void setAnnotations(List<AnnotationDecl> annotations) {
		this.annotations = annotations;
	}
}
