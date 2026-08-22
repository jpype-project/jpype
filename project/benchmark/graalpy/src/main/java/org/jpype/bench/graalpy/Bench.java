package org.jpype.bench.graalpy;

import java.nio.file.Path;
import org.graalvm.polyglot.Context;
import org.graalvm.polyglot.Source;
import org.graalvm.python.embedding.GraalPyResources;

/**
 * Launcher mirroring this repo's jep benchmarks: a Java process embeds the
 * guest language (GraalPy here, CPython-in-JVM for jep) and runs a .py
 * script file passed as the first argument. allowAllAccess grants the
 * script's `import java; java.type(...)` calls access to whatever is on
 * this JVM's own classpath (DeepBench via test/classes + test/harness),
 * the same classpath-based wiring jep's benchmarks use.
 */
public class Bench {
    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("usage: Bench <script.py> [script args...]");
            System.exit(2);
        }
        Path script = Path.of(args[0]);
        try (Context context = GraalPyResources.contextBuilder()
                .allowAllAccess(true)
                .arguments("python", args)
                .build()) {
            context.eval(Source.newBuilder("python", script.toFile()).build());
        }
    }
}
