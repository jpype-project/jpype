/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package python.lang;

/**
 *
 * @author nelson85
 */
public class Test
{
    
    public void examine(PyObject obj)
    {
        System.out.println("class "+ obj.getClass());
        System.out.println("object "+(obj instanceof PyObject));
        System.out.println("tuple "+(obj instanceof PyTuple));
        System.out.println("dict "+(obj instanceof PyDict));
        System.out.println("list "+(obj instanceof PyList));
        System.out.println("str "+(obj instanceof PyString));
        System.out.println("type "+(obj instanceof PyType));
    }
}
