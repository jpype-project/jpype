package jpype.utf8;

public class Utf8Test {

    private final static String[] DEFAULT_STRINGS = {
            "I can eat glass and it doesn't hurt me.",
            "Je peux manger du verre, ça ne me fait pas mal.",
            "ᛖᚴ ᚷᛖᛏ ᛖᛏᛁ ᚧ ᚷᛚᛖᚱ ᛘᚾ ᚦᛖᛋᛋ ᚨᚧ ᚡᛖ ᚱᚧᚨ ᛋᚨᚱ",
            "人人生而自由,在尊严和权利上一律平等。他们赋有理性和良心,并应以兄弟关系的精神互相对待。",
            "人人生而自由﹐在尊嚴和權利上一律平等。他們賦有理性和良心﹐並應以兄弟關係的精神互相對待。",
            "أنا قادر على أكل الزجاج و هذا لا يؤلمني.",
            "😁😂😃😄😅😆😠😡😢😣😤😥😨😩😪🚉🚌🚏🚑🚒🚓🚕🚗🚙🚚🚢🚤🚥🚧🚨🚻🚼🚽🚾🛀🆕🆖🆗🆘🆙🆚🈁🈂🈚🈯🈹🈺🉐🉑8⃣9⃣7⃣6⃣1⃣0"
    };

    private String data;

    /**
     * Dummy: just set a pure ascii string
     */
    public Utf8Test() {
        this.data = "Utf8Test pure ASCII";
    }

    /**
     * Instantiate the class with one of the DEFAULT strings. Use the index as reference.
     * @param indx reference to the DEFAULT_STRING
     */
    public Utf8Test(int indx) {
        this.data = DEFAULT_STRINGS[Math.abs(indx) % DEFAULT_STRINGS.length];
    }

    /**
     * Instantiate with a user-defined string
     * @param myinput
     */
    public Utf8Test(String myinput) {
        if (null == myinput) {
            this.data = "NULL INPUT";
        } else {
            try {
                int indx = Integer.parseInt(myinput);
                this.data = DEFAULT_STRINGS[Math.abs(indx) % DEFAULT_STRINGS.length];
            } catch (NumberFormatException nfe) {
                this.data = myinput;
            }
        }
    }

    public void print_system_info() {
        System.out.println("----------------------------------------------------------------------------------------");
        System.out.println("JVM: " + System.getProperty("java.vm.name") + ", version: " +
                System.getProperty("java.version") + " (" + System.getProperty("java.vm.version") + ")");
        System.out.println("OS:  " + System.getProperty("os.name") + "-" + System.getProperty("os.arch") +
                ", version: " + System.getProperty("os.version"));
        System.out.println("----------------------------------------------------------------------------------------");
    }

    public void print_to_stdout() {
        int nc = (int) this.data.codePoints().count();
        int nb = this.data.getBytes().length;
        System.out.println(String.format("nc = %3d, nb = %3d: (%s)",nc,nb,this.data));
    }

    /*
     * get the string defined by the instantiator
     */
    public String get() {
        return this.data;
    }

    /*
     * return true if the string defined by the instantiator equals the default string with given index
     */
    public boolean equalsTo(int indx) {
        return DEFAULT_STRINGS[Math.abs(indx) % DEFAULT_STRINGS.length].equals(this.data);
    }

    public static void main(String[] argv) {
        Utf8Test jp;

        new Utf8Test().print_system_info();
        if (0 == argv.length) {
            new Utf8Test().print_to_stdout();
            for (int i=0; i < DEFAULT_STRINGS.length; i++) {
                new Utf8Test(i).print_to_stdout();
            }
        } else {
            new Utf8Test(argv[0]).print_to_stdout();
        }
    }
}
