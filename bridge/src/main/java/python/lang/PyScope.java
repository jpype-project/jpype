/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package python.lang;

/**
 * Represents a dictionary which acts as a scope for execution and holding variables.
 * 
 * @author nelson85
 */
public interface PyScope
{
    PyObject eval(String source);
    void imports(String module);
    void imports(String module, String as);
}
