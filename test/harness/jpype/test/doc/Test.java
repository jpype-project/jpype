/*
 * Attempt at javadoc coverage test.
 */
package jpype.doc;

/**
 * A bunch of random stuff.
 *
 * @author nelson85
 * @see "The Java Programming Language"
 * @see java.lang.Object#wait()
 * @see <a href="spec.html#section">Java Spec</a>
 * @version 1.0
 */
public class Test
{

  public final static int QQ = 1;

  /**
   * This does something special.
   * <p>
   * It is so special we won't tell you.
   * <br>
   * We have our reasons. See <code>a</code> or
   * <pre>a</pre>. So <sup>up</sup> and don't look <sub>down</sub>. Literally
   * {@literal A<B>C}. Don't feel <small>s</small> when you can be <b>bold</b>
   * or <em>em</em> or <i>italics</i>.
   * <p>
   * Use the {@link #methodOne(int, Object) BangBang} method. *.
   * <blockquote><pre>{@code
   *    hey.method(1, SecretMap());
   * }</pre></blockquote>
   * <p>
   * <table style="width:100%" summary="foo">
   * <tr><th>North</th><th>East</th><th>Steps</th></tr>
   * <tr><td>Y</td><td>N</td><td>10</td></tr>
   * <tr><td>N</td><td>Y</td><td>20</td></tr>
   * </table>
   * <p>
   * <dl>
   * <dt>Defines
   * <dd>Something.
   * </dl>
   * <hr>
   * <ol>
   * <li> Item 1
   * <ul>
   * <li> Apples
   * <li> Pears
   * </ul>
   * <li> Item 2
   * </ol>
   *
   * @throws java.lang.NoSuchMethodException if something bad happens.
   * @param i an argument <code>a</code>. {@value #QQ}
   * @param j another argument.
   * @return a secret code.
   * @since 1.5
   *
   */
  public int methodOne(int i, Object j) throws NoSuchMethodException
  {
    return 12345;  // Same as your luggage combination.
  }
}
