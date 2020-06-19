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
package jpype.utf8;

public class Utf8Test
{

  private final static String[] DEFAULT_STRINGS =
  {
    "I can eat glass and it doesn't hurt me.",
    "Je peux manger du verre, \u00E7a ne me fait pas mal.",
    "\u16D6\u16B4 \u16B7\u16D6\u16CF \u16D6\u16CF\u16C1 \u16A7 \u16B7\u16DA\u16D6\u16B1 \u16D8\u16BE \u16A6\u16D6\u16CB\u16CB \u16A8\u16A7 \u16A1\u16D6 \u16B1\u16A7\u16A8 \u16CB\u16A8\u16B1",
    "\u4EBA\u4EBA\u751F\u800C\u81EA\u7531,\u5728\u5C0A\u4E25\u548C\u6743\u5229\u4E0A\u4E00\u5F8B\u5E73\u7B49\u3002\u4ED6\u4EEC\u8D4B\u6709\u7406\u6027\u548C\u826F\u5FC3,\u5E76\u5E94\u4EE5\u5144\u5F1F\u5173\u7CFB\u7684\u7CBE\u795E\u4E92\u76F8\u5BF9\u5F85\u3002",
    "\u4EBA\u4EBA\u751F\u800C\u81EA\u7531\uFE50\u5728\u5C0A\u56B4\u548C\u6B0A\u5229\u4E0A\u4E00\u5F8B\u5E73\u7B49\u3002\u4ED6\u5011\u8CE6\u6709\u7406\u6027\u548C\u826F\u5FC3\uFE50\u4E26\u61C9\u4EE5\u5144\u5F1F\u95DC\u4FC2\u7684\u7CBE\u795E\u4E92\u76F8\u5C0D\u5F85\u3002",
    "\u0623\u0646\u0627 \u0642\u0627\u062F\u0631 \u0639\u0644\u0649 \u0623\u0643\u0644 \u0627\u0644\u0632\u062C\u0627\u062C \u0648 \u0647\u0630\u0627 \u0644\u0627 \u064A\u0624\u0644\u0645\u0646\u064A.",
    "\uD83D\uDE01\uD83D\uDE02\uD83D\uDE03\uD83D\uDE04\uD83D\uDE05\uD83D\uDE06\uD83D\uDE20\uD83D\uDE21\uD83D\uDE22\uD83D\uDE23\uD83D\uDE24\uD83D\uDE25\uD83D\uDE28\uD83D\uDE29\uD83D\uDE2A\uD83D\uDE89\uD83D\uDE8C\uD83D\uDE8F\uD83D\uDE91\uD83D\uDE92\uD83D\uDE93\uD83D\uDE95\uD83D\uDE97\uD83D\uDE99\uD83D\uDE9A\uD83D\uDEA2\uD83D\uDEA4\uD83D\uDEA5\uD83D\uDEA7\uD83D\uDEA8\uD83D\uDEBB\uD83D\uDEBC\uD83D\uDEBD\uD83D\uDEBE\uD83D\uDEC0\uD83C\uDD95\uD83C\uDD96\uD83C\uDD97\uD83C\uDD98\uD83C\uDD99\uD83C\uDD9A\uD83C\uDE01\uD83C\uDE02\uD83C\uDE1A\uD83C\uDE2F\uD83C\uDE39\uD83C\uDE3A\uD83C\uDE50\uD83C\uDE518\u20E39\u20E37\u20E36\u20E31\u20E30"
  };

  private String data;

  /**
   * Dummy: just set a pure ascii string
   */
  public Utf8Test()
  {
    this.data = "Utf8Test pure ASCII";
  }

  /**
   * Instantiate the class with one of the DEFAULT strings. Use the index as
   * reference.
   *
   * @param indx reference to the DEFAULT_STRING
   */
  public Utf8Test(int indx)
  {
    this.data = DEFAULT_STRINGS[Math.abs(indx) % DEFAULT_STRINGS.length];
  }

  /**
   * Instantiate with a user-defined string
   *
   * @param myinput
   */
  public Utf8Test(String myinput)
  {
    if (null == myinput)
    {
      this.data = "NULL INPUT";
    } else
    {
      try
      {
        int indx = Integer.parseInt(myinput);
        this.data = DEFAULT_STRINGS[Math.abs(indx) % DEFAULT_STRINGS.length];
      } catch (NumberFormatException nfe)
      {
        this.data = myinput;
      }
    }
  }

  public void print_system_info()
  {
    System.out.println("----------------------------------------------------------------------------------------");
    System.out.println("JVM: " + System.getProperty("java.vm.name") + ", version: "
            + System.getProperty("java.version") + " (" + System.getProperty("java.vm.version") + ")");
    System.out.println("OS:  " + System.getProperty("os.name") + "-" + System.getProperty("os.arch")
            + ", version: " + System.getProperty("os.version"));
    System.out.println("----------------------------------------------------------------------------------------");
  }

  public void print_to_stdout()
  {
    int nc = (int) this.data.codePoints().count();
    int nb = this.data.getBytes().length;
    System.out.println(String.format("nc = %3d, nb = %3d: (%s)", nc, nb, this.data));
  }

  /*
     * get the string defined by the instantiator
   */
  public String get()
  {
    return this.data;
  }

  /*
     * return true if the string defined by the instantiator equals the default string with given index
   */
  public boolean equalsTo(int indx)
  {
    return DEFAULT_STRINGS[Math.abs(indx) % DEFAULT_STRINGS.length].equals(this.data);
  }

  public static void main(String[] argv)
  {
    Utf8Test jp;

    new Utf8Test().print_system_info();
    if (0 == argv.length)
    {
      new Utf8Test().print_to_stdout();
      for (int i = 0; i < DEFAULT_STRINGS.length; i++)
      {
        new Utf8Test(i).print_to_stdout();
      }
    } else
    {
      new Utf8Test(argv[0]).print_to_stdout();
    }
  }
}
