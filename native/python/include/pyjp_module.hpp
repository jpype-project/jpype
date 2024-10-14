#pragma once

#include "Python.h" // IWYU pragma: keep

extern void PyJPArray_initType(PyObject* module);
extern void PyJPBuffer_initType(PyObject* module);
extern void PyJPClass_initType(PyObject* module);
extern void PyJPField_initType(PyObject* module);
extern void PyJPMethod_initType(PyObject* module);
extern void PyJPMonitor_initType(PyObject* module);
extern void PyJPProxy_initType(PyObject* module);
extern void PyJPObject_initType(PyObject* module);
extern void PyJPNumber_initType(PyObject* module);
extern void PyJPClassHints_initType(PyObject* module);
extern void PyJPPackage_initType(PyObject* module);
extern void PyJPChar_initType(PyObject* module);
extern void PyJPValue_initType(PyObject* module);
