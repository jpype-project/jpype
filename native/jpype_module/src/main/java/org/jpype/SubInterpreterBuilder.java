// --- file: org/jpype/SubInterpreterBuilder.java ---
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
package org.jpype;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.Reader;
import java.io.Writer;
import java.util.Arrays;
import java.util.EnumSet;
import java.util.function.Supplier;

/**
 * Configures and launches {@link SubInterpreter} instances, modeled on
 * {@link java.lang.ProcessBuilder}: a mutable, reusable object you
 * configure with setters, then call {@link #start} on (repeatedly, if
 * desired) to launch independently-configured instances.
 *
 * <p>
 * The seven {@code PyInterpreterConfig} flags are held as an
 * {@link EnumSet} of {@link Option} rather than seven booleans, mutated via
 * {@link #with} / {@link #without}. Non-flag settings (stdio redirection)
 * get ordinary setters, reusing {@link Interpreter}'s existing
 * {@code setOutput}/{@code setError}/{@code setInput}.</p>
 *
 * <p>
 * A bare {@code new SubInterpreterBuilder()} defaults to the safest legal
 * combination - own GIL, own obmalloc, {@code check_multi_interp_extensions}
 * enabled (see {@link #ownGil}) - rather than the least restrictive one.
 * Use {@link #elevated} to opt into the shared-GIL/shared-obmalloc
 * combination when that isolation is too restrictive for what needs to be
 * imported or shared.</p>
 */
public class SubInterpreterBuilder
{
  /**
   * The seven {@code PyInterpreterConfig} boolean fields, plus
   * {@code OWN_GIL} (which selects {@code PyInterpreterConfig_OWN_GIL} vs.
   * the default shared GIL).
   */
  public enum Option
  {
    USE_MAIN_OBMALLOC,
    ALLOW_FORK,
    ALLOW_EXEC,
    ALLOW_THREADS,
    ALLOW_DAEMON_THREADS,
    CHECK_MULTI_INTERP_EXTENSIONS,
    OWN_GIL
  }

  // Safest legal combination: own GIL, own obmalloc arena, fork/exec/daemon
  // threads disallowed. Equivalent to what ownGil() used to build explicitly
  // - now the default, since this API has never shipped and there is no
  // prior behavior to stay compatible with.
  private final EnumSet<Option> options = EnumSet.of(Option.OWN_GIL,
          Option.CHECK_MULTI_INTERP_EXTENSIONS, Option.ALLOW_THREADS);

  private Object out;
  private Object err;
  private Object in;

  /**
   * Enables the given options.
   *
   * @return this builder, for chaining.
   */
  public SubInterpreterBuilder with(Option... opts)
  {
    options.addAll(Arrays.asList(opts));
    return this;
  }

  /**
   * Disables the given options.
   *
   * @return this builder, for chaining.
   */
  public SubInterpreterBuilder without(Option... opts)
  {
    options.removeAll(Arrays.asList(opts));
    return this;
  }

  /**
   * @return this builder, for chaining.
   * @see Interpreter#setOutput(OutputStream)
   */
  public SubInterpreterBuilder setOutput(OutputStream out)
  {
    this.out = out;
    return this;
  }

  /**
   * @return this builder, for chaining.
   * @see Interpreter#setOutput(Writer)
   */
  public SubInterpreterBuilder setOutput(Writer out)
  {
    this.out = out;
    return this;
  }

  /**
   * @return this builder, for chaining.
   * @see Interpreter#setError(OutputStream)
   */
  public SubInterpreterBuilder setError(OutputStream err)
  {
    this.err = err;
    return this;
  }

  /**
   * @return this builder, for chaining.
   * @see Interpreter#setError(Writer)
   */
  public SubInterpreterBuilder setError(Writer err)
  {
    this.err = err;
    return this;
  }

  /**
   * @return this builder, for chaining.
   * @see Interpreter#setInput(InputStream)
   */
  public SubInterpreterBuilder setInput(InputStream in)
  {
    this.in = in;
    return this;
  }

  /**
   * @return this builder, for chaining.
   * @see Interpreter#setInput(Reader)
   */
  public SubInterpreterBuilder setInput(Reader in)
  {
    this.in = in;
    return this;
  }

  /**
   * A builder configured for genuine PEP 684 isolation: its own GIL and its
   * own obmalloc arena, with {@code check_multi_interp_extensions} enabled
   * (required whenever {@code use_main_obmalloc} is off - see
   * {@link #start}). Requires every extension module the subinterpreter
   * imports, including {@code _jpype}, to be multi-phase-init-safe (true
   * since {@code plan/MultiPhaseInit.md}).
   *
   * <p>
   * This is the safest legal combination and is also what a bare
   * {@code new SubInterpreterBuilder()} builds - this method exists purely
   * so call sites can say so explicitly instead of relying on an invisible
   * default.</p>
   */
  public static SubInterpreterBuilder ownGil()
  {
    return new SubInterpreterBuilder();
  }

  /**
   * A builder configured for the less-restrictive, shared-GIL /
   * shared-obmalloc combination: the subinterpreter runs under the main
   * interpreter's GIL and allocator instead of its own, but can still
   * import single-phase-init extension modules that
   * {@code check_multi_interp_extensions} would otherwise reject. Use this
   * when the default {@link #ownGil} isolation is too restrictive for what
   * the subinterpreter needs to import or share.
   */
  public static SubInterpreterBuilder elevated()
  {
    return new SubInterpreterBuilder()
            .with(Option.USE_MAIN_OBMALLOC)
            .without(Option.OWN_GIL, Option.CHECK_MULTI_INTERP_EXTENSIONS);
  }

  /**
   * Validates the current option set, then launches a new,
   * independently-configured {@link SubInterpreter} and wires up any
   * configured stdio streams on it.
   *
   * @throws IllegalStateException if the option set is not a legal
   * {@code PyInterpreterConfig} combination.
   */
  public SubInterpreter start()
  {
    validate();

    SubInterpreter sub = new SubInterpreter();
    sub.start(options.contains(Option.USE_MAIN_OBMALLOC),
            options.contains(Option.ALLOW_FORK),
            options.contains(Option.ALLOW_EXEC),
            options.contains(Option.ALLOW_THREADS),
            options.contains(Option.ALLOW_DAEMON_THREADS),
            options.contains(Option.CHECK_MULTI_INTERP_EXTENSIONS),
            options.contains(Option.OWN_GIL));

    if (out instanceof OutputStream)
      sub.setOutput((OutputStream) out);
    else if (out instanceof Writer)
      sub.setOutput((Writer) out);
    if (err instanceof OutputStream)
      sub.setError((OutputStream) err);
    else if (err instanceof Writer)
      sub.setError((Writer) err);
    if (in instanceof InputStream)
      sub.setInput((InputStream) in);
    else if (in instanceof Reader)
      sub.setInput((Reader) in);

    return sub;
  }

  /**
   * Adapts this builder to a {@link Supplier}, for callers that need a
   * lazy source of independently-configured instances (e.g. a worker pool
   * pulling a fresh subinterpreter per task) rather than launching
   * immediately.
   *
   * <p>
   * Repeated {@code get()} calls launch independent instances that share
   * this builder's current option set and stdio stream <em>references</em>
   * (not copies) - if two instances are given the same stream and write to
   * it concurrently, their output will interleave, exactly as if
   * {@link #start} had been called twice directly.</p>
   */
  public Supplier<SubInterpreter> asSupplier()
  {
    return this::start;
  }

  /**
   * Enforces the one hard cross-field rule CPython's
   * {@code init_interp_settings} (Python/pylifecycle.c) applies:
   * {@code use_main_obmalloc=0} requires
   * {@code check_multi_interp_extensions=1} ("per-interpreter obmalloc
   * does not support single-phase init extension modules").
   */
  private void validate()
  {
    if (!options.contains(Option.USE_MAIN_OBMALLOC)
            && !options.contains(Option.CHECK_MULTI_INTERP_EXTENSIONS))
    {
      throw new IllegalStateException(
              "illegal SubInterpreterBuilder configuration: disabling "
              + "USE_MAIN_OBMALLOC requires CHECK_MULTI_INTERP_EXTENSIONS "
              + "to be enabled (per-interpreter obmalloc does not support "
              + "single-phase init extension modules)");
    }
  }
}
