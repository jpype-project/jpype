/* 
 * Cross-Language Garbage Collection (CLGC) Demonstration Module
 *
 * This module addresses the challenges of garbage collection across languages,
 * specifically between Python and Java, by introducing hooks into Python's
 * generation 2 garbage collector. It demonstrates how Python can efficiently
 * manage cross-language references through a process called "internalization."
 *
 * The implementation assumes a "double weak" referencing system, where each
 * language holds its own strong references to its objects. Weak references are
 * used to communicate when foreign objects are no longer needed, allowing them
 * to be dropped. Internalization further enhances this by replacing strong
 * references with Python-side graph portions, enabling normal garbage
 * collection.
 *
 * Internalization Process:
 * ========================
 * Internalization is the process by which Python identifies isolated objects
 * connected to foreign systems, delegates ownership of their lifecycle to the
 * foreign system, and ensures proper cleanup of cross-language references. This
 * ensures Python no longer holds responsibility for keeping foreign objects
 * alive, allowing the foreign system to manage their lifecycle efficiently.
 *
 * Mechanics:
 * ----------
 * 1. **Detection of Isolated Python Objects:** 
 *    - During Python's garbage collection traversal, the `ReferenceManager`
 *      identifies Python objects that are not connected to other Python objects
 *      except through the `ReferenceManager`.
 *    - These objects are considered "isolated" from Python's perspective and
 *      are candidates for internalization.
 *
 * 2. **Depth-First Search (DFS):**
 *    - The `ReferenceManager` uses Python's traversal system to perform a
 *      depth-first search (DFS) on the reference graph rooted at the isolated
 *      Python object.
 *    - The DFS explores all references originating from the Python object to
 *      determine its relationship with foreign objects (e.g., Java objects).
 *
 * 3. **Discovery of Java References:**
 *    - During the traversal, if the `ReferenceManager` encounters a reference
 *      to a Java object:
 *      - It checks whether the Java object has already been visited by Java's
 *        garbage collector during its cycle.
 *      - If the Java object **has not been visited**, it means the Java object
 *        is still reachable from Python but not actively held by Java.
 *
 * 4. **Delegating Ownership to Java:**
 *    - If an unvisited Java object is discovered during the DFS:
 *      - The relationship between the Python object and the Java object is sent
 *        to the Java side.
 *      - On the Java side:
 *        - The Java object is removed from the **strong global reference list**,
 *          meaning Java no longer holds the object alive directly.
 *        - The Java object is now held alive only by the weak reference
 *          originating from Python.
 *
 * 5. **Handling Cases with No Java References:**
 *    - If the depth-first search does not discover any Java references:
 *      - The Python object is added to the `foreign_list` in the
 *        `ReferenceManager`.
 *      - The Python object will remain on the `foreign_list` until Java breaks
 *        its weak link to the object.
 *      - Since no Java references were found, there cannot be a cross-language
 *        reference loop involving this Python object.
 *
 * Key Scenarios:
 * --------------
 * - **Scenario 1:** Python Object References a Java Object
 *   - The Python object is isolated except for its reference to the Java object.
 *   - The DFS discovers the Java object, which has not been visited by Java's
 *     garbage collector.
 *   - The relationship is delegated to Java, and the Java object is removed
 *     from the strong global reference list.
 *   - **Outcome:** Python no longer holds responsibility for the Java object,
 *     and the Java object is managed by Java.
 *
 * - **Scenario 2:** Python Object Does Not Reference Any Java Object
 *   - The Python object is isolated.
 *   - The DFS does not discover any Java references.
 *   - The Python object is added to the `foreign_list` and waits for Java to
 *     break its weak link.
 *   - **Outcome:** The Python object remains on the `foreign_list` until Java
 *     determines it is no longer needed.
 *
 * - **Scenario 3:** Cross-Language Reference Loop
 *   - The DFS discovers a loop involving both Python and Java objects.
 *   - The relationship is delegated to Java, and Java assumes ownership of the
 *     loop.
 *   - **Outcome:** Python no longer holds responsibility for the loop, and Java
 *     manages its lifecycle.
 *
 * Advantages:
 * -----------
 * - **Efficient Garbage Collection:** Delegating ownership of cross-language
 *   relationships to the foreign system reduces Python's involvement in
 *   managing foreign objects.
 * - **Breaks Reference Loops:** Internalization ensures that reference loops
 *   involving both Python and Java are broken, preventing memory leaks.
 * - **Optimized Resource Management:** Objects are cleaned up promptly when no
 *   references exist on either side.
 *
 * Edge Cases:
 * -----------
 * - **Case 1:** Java Object Referenced by Multiple Python Objects
 *   - If multiple Python objects reference the same Java object, the Java
 *     object will remain alive until all Python references are removed.
 * - **Case 2:** Weak Links
 *   - If the Java object is held alive only by a weak link from Python, it will
 *     be garbage collected by Java once Python removes its reference.
 * - **Case 3:** Synchronization Delays
 *   - If Python and Java garbage collection cycles are not synchronized, there
 *     may be slight delays in cleanup. This is not critical but could be
 *     optimized.
 *
 * Python GC Phases:
 * -----------------
 * - **Phase 1: subtract_refs**
 *   - Decreases reference counts for objects in the collection set. If an
 *     object's reference count drops to zero, it is considered unreachable.
 *   - The `arg` parameter in the traversal callback (`visit_decref`) represents
 *     the parent object being traversed.
 *
 * - **Phase 2: move_reachable**
 *   - Identifies all reachable objects starting from the roots and moves them,
 *     along with their dependencies, into the reachable list.
 *   - The `arg` parameter in this phase typically represents the new list of
 *     reachable items (`PyGC_Head*`).
 *
 * - **Phase 3 (Debugging Enabled):**
 *   - Exploits properties of Python's GC to pivot the Sentinel object to the
 *     back of the GC list. All objects afterward must be owned by a foreign
 *     object or collected. At this point, the structure and links between
 *     objects can be analyzed.
 *
 * Implementation Notes:
 * ---------------------
 * - Internalization is only possible on the Python side because Java lacks a
 *   traversal mechanism to discover relationships.
 * - Proper synchronization between Python and Java garbage collection processes
 *   is essential for efficient cleanup.
 * - Ensure the `is_broken` function is implemented correctly and efficiently,
 *   as it plays a critical role in determining the lifecycle of objects.
 *
 * Future Considerations:
 * ----------------------
 * - A future PEP could define the minimal API required to support CLGC:
 *   1. A method to report the type of GC being executed at the start of the
 *      cycle.
 *   2. Callbacks at each major phase to allow state alterations.
 *   3. Methods to check object states during each phase, accounting for Python
 *      objects potentially existing in multiple states within the same phase.
 *
 * This module simulates interactions with Java objects and demonstrates how
 * Python's GC can be extended for cross-language garbage collection.
 */
// Define Py_BUILD_CORE to access internal headers
#define Py_BUILD_CORE

#include <Python.h>
#include <frameobject.h>
#include <internal/pycore_gc.h>      // Internal header for GC structures
#include <internal/pycore_interp.h>  // Internal header for interpreter state

clgcfunc reference_manager = NULL;

/*******************************************************************************/
// This section show an API that Python can implement to facilitate CLGC

// (internal) Needed for the DFS.  
#define AS_GC(o) ((PyGC_Head *)(o)-1)
#define PREV_MASK_COLLECTING _PyGC_PREV_MASK_COLLECTING


/**
 * Macro: PyGC_VISIT_DFS
 * ---------------------
 * Implements a depth-first search (DFS) traversal for garbage collection. This
 * macro temporarily modifies the `_gc_prev` field of a garbage collection
 * header (`PyGC_Head`) to mark it as visited during the traversal, then calls
 * the object's `tp_traverse` method to recursively visit its references.
 *
 * Parameters:
 * - `op`: A pointer to the Python object being visited (`PyObject*`).
 * - `visit`: The callback function to be applied during traversal (`visitproc`).
 * - `arg`: Additional arguments passed to the callback function.
 *
 * Usage:
 * - This macro is used in the internal garbage collection process to analyze
 *   object references and determine reachability.
 */
#define PyGC_VISIT_DFS(op, visit, arg) \
  { AS_GC(op)->_gc_prev ^= PREV_MASK_COLLECTING; \
    tp->tp_traverse(op, visit, arg); \
    AS_GC(op)->_gc_prev |= PREV_MASK_COLLECTING; }
 
/**
 * Typedef: clgcfunc
 * ------------------
 * Defines the function signature for a callback used as a reference manager
 * during Python's garbage collection process. This callback is invoked at
 * various phases of the generation 2 garbage collection cycle to manage foreign
 * objects and their references.
 *
 * Function Signature:
 * - `int (*clgcfunc)(int phase, visitproc visit, void* args);`
 *
 * Parameters:
 * - `phase`: Indicates the current phase of the garbage collection process.
 *   - `0`: A new garbage collection cycle is beginning.
 *   - `1`: The `decrefs` phase is complete, and objects with zero external
 *     references are subject to collection. Foreign objects should be visited
 *     at this phase to treat them as normal objects.
 *   - `2`: The reachability analysis is complete. Objects not yet reachable
 *     will be collected. Foreign objects still needed should be recovered at
 *     this phase.
 *   - `3`: The garbage collection cycle is completed.
 * - `visit`: A callback function (`visitproc`) used for traversing object
 *   references during the garbage collection process.
 * - `args`: Additional arguments passed to the callback function, typically
 *   used for context or state management.
 *
 * Returns:
 * - `0` on success.
 * - Non-zero values can be used to indicate errors or specific conditions
 *   during the garbage collection process.
 *
 * Usage:
 * - Implement this function type to define a custom reference manager for
 *   Python's garbage collector. The reference manager should handle foreign
 *   object tracking and cleanup during the specified GC phases.
 *
 */
typedef int (*clgcfunc)(int phase, visitproc visit, void* args);

/**
 * Function: PyGC_IsReachable
 * --------------------------
 * Determines whether a given Python object is reachable at the end of the
 * garbage collection reachability phase.
 *
 * Parameters:
 * - `obj`: A pointer to the Python object (`PyObject*`) being checked.
 *
 * Returns:
 * - `1` if the object is reachable.
 * - `0` if the object is not reachable.
 *
 * Notes:
 * - This function should only be called at the end of the reachability phase
 *   (phase 2). Calling it at any other time during the GC cycle will produce
 *   undefined or unexpected results.
 *
 * Usage:
 * - Use this function to verify whether an object has been marked as reachable
 *   during garbage collection.
 */
int PyGC_IsReachable(PyObject *obj);

/**
 * Function: PyGC_InstallReferenceManager
 * --------------------------------------
 * Installs a custom reference manager for the Python interpreter. The reference
 * manager integrates with Python's garbage collector to track and manage
 * foreign objects during a generation 2 garbage collection cycle.
 *
 * Parameters:
 * - `manager`: A callback function (`clgcfunc`) that will be invoked during
 *   different phases of the garbage collection process. The callback function
 *   signature is:
 *   `int manager(int phase, visitproc visit, void* args)`
 *   - `phase`: Indicates the current phase of the garbage collection process.
 *   - `visit`: A callback function used for traversal during the GC process.
 *   - `args`: Additional arguments passed to the callback function.
 *
 * Returns:
 * - `0` on success.
 * - `-1` if a reference manager is already installed.
 *
 * Notes:
 * - Only one reference manager can be installed at a time. Attempting to
 *   install a second reference manager will result in a runtime error.
 * - The reference manager is responsible for ensuring proper tracking and
 *   cleanup of foreign objects during garbage collection.
 *
 * Usage:
 * - Use this function to integrate custom foreign object tracking into Python's
 *   garbage collector.
 */
int PyGC_InstallReferenceManager(clgcfunc manager)
{
    if (reference_manager != NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Only one reference manager allowed");
        return -1;
    }
    reference_manager = manager;
    return 0;
}

static int reference_manager_trigger(int phase, visitproc visit, void *args)
{
    if (reference_manager == NULL)
        return 0;
    return reference_manager(phase, visit, args);
}



/*******************************************************************************/
// This section shows how the API is used to implement CLGC

/* 
 * ForeignReference Structure
 *
 * Represents a reference to a foreign object (e.g., Java object) held by Python.
 * This structure is used to manage cross-language references.
 */
typedef struct ForeignReference {
    struct ForeignReference* previous;  // Previous reference in the list
    struct ForeignReference* next;      // Next reference in the list
    PyObject* local_object;             // Pointer to the local Python object (strong reference)
    void* remote_object;                // Pointer to the remote object (generic type)
} ForeignReference;

/* Static variables for CLGC state */
static PyObject* sentinel_instance = NULL;  // Sentinel object for GC tracking
static ForeignReference references;         // List of foreign references
static int renew = 0;
static int skip = 0;

/* 
 * Initialize the foreign reference list.
 * The list is circular and starts with a dummy head node.
 */
static void reference_init(ForeignReference* head) {
    head->next = head;
    head->previous = head;
}

/* 
 * Insert a new item into the foreign reference list.
 */
static void reference_insert(ForeignReference *head, ForeignReference* item) {
    ForeignReference* next = head->next;
    head->next = item;
    item->next = next;
    item->previous = head;
    next->previous = item;
}

/* 
 * Remove an item from the foreign reference list.
 */
static void reference_remove(ForeignReference* item) {
    if (item->previous == NULL || item->next == NULL) return;
    ForeignReference* prev = item->previous;
    ForeignReference* next = item->next;
    prev->next = next;
    next->previous = prev;
    item->previous = NULL;
    item->next = NULL;
}

/* 
 * Visit all references in the foreign reference list.
 * This is used during GC traversal to ensure proper cleanup.
 */
static int visit_references(visitproc visit, void* arg) {
    ForeignReference* current = references.next;
    while (current != &references) {
        PyObject* op = current->local_object;
        Py_VISIT(op);
        current = current->next;
    }
    return 0;
}

/* 
 * ForeignObject Type
 *
 * Represents a foreign object held by Python. This type is used to simulate
 * interactions with Java objects in the CLGC process.
 */
typedef struct {
    PyObject_HEAD
} ForeignObject;

/* 
 * Free function for ForeignObject.
 * Ensures proper cleanup of foreign objects during GC.
 */
static void Foreign_free(void* obj) {
    PyTypeObject *type = Py_TYPE(obj);
    if (type->tp_flags & Py_TPFLAGS_HAVE_GC)
        PyObject_GC_Del(obj);
    else
        PyObject_Free(obj);
}

/* 
 * Renew the lease for a foreign object.
 * This is called when the foreign object is still reachable from Python.
 */
static void foreign_renew(ForeignObject* self) {
    printf("renew %p\n", self);
}

/* 
 * Skip renewal for a foreign object.
 * This is called when the foreign object is no longer reachable from Python.
 */
static void foreign_skip(ForeignObject* self) {
    printf("skip %p\n", self);
}

/* 
 * Traverse function for ForeignObject.
 * Handles GC traversal for foreign objects during specific GC phases.
 */
static int Foreign_traverse(ForeignObject* self, visitproc visit, void* arg) {
    if (renew) foreign_renew(self);
    if (skip) foreign_skip(self);
    return 0;
}

/* 
 * ForeignObject type definition.
 */
static PyTypeObject ForeignType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "helloworld.Foreign",
    .tp_basicsize = sizeof(ForeignObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_free = Foreign_free,
    .tp_traverse = (traverseproc) Foreign_traverse,
    .tp_new = PyType_GenericNew,
};


/* 
 * Internalization Functions
 *
 * These functions handle the process of internalization, where Python identifies isolated
 * objects connected to foreign systems and delegates ownership of their lifecycle to the foreign system.
 */

/* 
 * Start internalization for a Python object.
 * This function is a stub for interacting with Java during the internalization process.
 */
static void internalize_start(PyObject *obj, void* arg) {
    printf(" %p ::", obj);
}

/* 
 * Add a Python object to the internalization process.
 * This function is a stub for interacting with Java during the internalization process.
 */
static void internalize_add(PyObject* obj, void* arg) {
    printf(" %p", obj);
}

/* 
 * End internalization for a Python object.
 * This function is a stub for interacting with Java during the internalization process.
 */
static void internalize_end(PyObject* obj, void* arg) {
    printf("\n");
}

/* 
 * Perform a depth-first search (DFS) using Python's traversal mechanism.
 * This function analyzes relationships between foreign incoming references and foreign outgoing ones.
 */
static int internalize_trace(PyObject *op, void *arg) {
    if (PyGC_IsReachable(op))
        return 0;

    // Check if the object is a foreign reference
    PyTypeObject* tp = Py_TYPE(op);
    if (tp->tp_free == Foreign_free) {
        internalize_add(op, arg);
        return 0;
    }

    // Perform DFS traversal for objects bound for garbage collection
    PyGC_VISIT_DFS(op, internalize_trace, arg);
    return 0;
}

/* 
 * Analyze linkages between foreign references and Python objects.
 * This function is called at the end of the GC process to discover reference loops.
 */
static void internalize_analyze() {
    ForeignReference* current = references.next;
    while (current != &references) {
        PyObject* op = current->local_object;

        // Perform DFS traversal to find reference loops
        internalize_start(op, NULL);
        internalize_trace(op, NULL);
        internalize_end(op, NULL);
        current = current->next;
    }
}

/** 
 * Here is a sample of how the reference manager hooks are used.
 */
int ReferenceManager_trigger(int phase, visitproc visit, void* args)
{
    printf("trigger %d\n", phase);

    // A new GC cycle is beginning
    if (phase == 0)
    {
        renew = 0;
        skip = 0;
        return 0;
    }

    // decref phase is completing
    if (phase == 1)
    {
        visit_references(visit, args);

        // Any reachable foreign object should renew or request a lease.
        renew = 1;
        return 0;
    }

    // reachable analysis is completing
    if (phase == 2)
    {
        renew = 0;

        // Analyze reachablity and inform Java of disconnected segments.
        internalize_analyze();

        skip = 1;

        // Add our references to the reachability to keep them alive until Java terminates them
        visit_references(visit, args);
        return 0;
    }

    // gc cycle is ended.
    if (phase == 3)
    {
        // Notify Java that new leases are in force.
        renew = 0;
        skip = 0;
    }
    return 0;
}


//*******************************************************************************************
// The next section is the guts of implementing API without support from Python

// Macros for accessing GC internals
#define GC_NEXT _PyGCHead_NEXT
#define GC_PREV _PyGCHead_PREV
#define GEN_HEAD(gcstate, n) (&(gcstate)->generations[n].head)
#define PREV_MASK_COLLECTING _PyGC_PREV_MASK_COLLECTING
#define NEXT_MASK_UNREACHABLE (1)

/* 
 * Utility functions for GC operations 
 */

// Check if a GC object is currently being collected
static inline int gc_is_collecting(PyGC_Head *g) {
    return (g->_gc_prev & PREV_MASK_COLLECTING) != 0;
}

// Get the reference count of a GC object
static inline Py_ssize_t gc_get_refs(PyGC_Head *g) {
    return (Py_ssize_t)(g->_gc_prev >> _PyGC_PREV_SHIFT);
}

// Append a node to a GC list
static inline void gc_list_append(PyGC_Head *node, PyGC_Head *list) {
    PyGC_Head *last = (PyGC_Head *)list->_gc_prev;
    _PyGCHead_SET_PREV(node, last);
    _PyGCHead_SET_NEXT(last, node);
    _PyGCHead_SET_NEXT(node, list);
    list->_gc_prev = (uintptr_t)node;
}

// Remove a node from its current GC list
static inline void gc_list_remove(PyGC_Head *node) {
    PyGC_Head *prev = GC_PREV(node);
    PyGC_Head *next = GC_NEXT(node);
    _PyGCHead_SET_NEXT(prev, next);
    _PyGCHead_SET_PREV(next, prev);
    node->_gc_next = 0;  /* Object is not currently tracked */
}

// Check if an object is garbage-collectable
static inline int _PyObject_IS_GC(PyObject *obj) {
    return (PyType_IS_GC(Py_TYPE(obj)) && 
            (Py_TYPE(obj)->tp_is_gc == NULL || Py_TYPE(obj)->tp_is_gc(obj)));
}

typedef struct _gc_runtime_state PyGCState;
static PyGCState* get_gc_state()
{
    // Access the interpreter state
    PyInterpreterState* interp = PyInterpreterState_Get();
    if (!interp) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get interpreter state");
        return NULL;
    }

    // Access the garbage collector state
    PyGCState* gc_state = &interp->gc;
    if (!gc_state) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get garbage collector state");
        return NULL;
    }
    return gc_state;
}

// Implement the reachablity function
int PyGC_IsReachable(PyObject *obj)
{   
    // Non GC objects are always reachable
    if (!_PyObject_IS_GC(obj)) return 1;

    // Objects that have already been collected are reachable
    PyGC_Head *gc = AS_GC(obj);
    if (!gc_is_collecting(gc)) return 1;

    // Objects that were moved to the younger list are reachable
    const Py_ssize_t gc_refs = gc_get_refs(gc);
    if (gc_refs == 1) 
        return 1;
    
    return 0;
}

static int generation = -1;                 // Current GC generation
static int phase = -1;                      // Current GC phase
static void* magic;                         // Magic identifier for private classes

/* 
 * Sentinel Object
 *
 * The Sentinel object is used to track the GC process and perform specific actions during
 * different phases of garbage collection. It acts as a pivot point for GC operations.
 */

/* 
 * Assert that the Sentinel class is private.
 * This prevents unauthorized creation of Sentinel objects.
 */
static int assert_private(void* args) {
    if (args != magic) {
        PyErr_SetString(PyExc_TypeError, "This class is private");
        return -1;
    }
    return 0;
}

/* 
 * Pivot Object
 *
 * The Pivot object is used internally by the Sentinel to manipulate the GC process.
 */
typedef struct {
    PyObject_HEAD
    void *sentinel;  // Pointer to the Sentinel object
} PivotObject;

/* 
 * Traverse function for PivotObject.
 * Handles GC traversal for Pivot objects during specific GC phases.
 */
static int Pivot_traverse(PivotObject* self, visitproc visit, void* arg) {
    if (phase <= 0) return 0;

    if (phase == 1) {
        PyGC_Head* reachable = (PyGC_Head*) arg;

        // Move the Sentinel object to the end of the GC list
        PyGC_Head* gc2 = AS_GC(self->sentinel);
        gc_list_remove(gc2);
        gc_list_append(gc2, reachable);
        gc2->_gc_prev = 6;
    }

    return 0;
}

/* 
 * Initialize the Pivot object.
 */
static int Pivot_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    return assert_private(args);
}

/* 
 * PivotObject type definition.
 */
static PyTypeObject PivotType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "helloworld.Pivot",
    .tp_basicsize = sizeof(PivotObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc) Pivot_traverse,
    .tp_new = PyType_GenericNew,
    .tp_init = Pivot_init,
};

/* 
 * Sentinel Object
 */
typedef struct {
    PyObject_HEAD
    PivotObject* pivot;  // Pointer to the Pivot object
} SentinelObject;

/* 
 * Initialize the Sentinel object.
 */
static int Sentinel_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    if (assert_private(args) == -1) return -1;

    SentinelObject *sentinel = (SentinelObject *)self;

    // Create a new PivotObject instance
    PyObject *pivot_instance = PyObject_CallObject((PyObject *)&PivotType, args);
    if (!pivot_instance) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create PivotObject");
        return -1;
    }

    // Link the Sentinel and Pivot objects
    sentinel->pivot = (PivotObject *)pivot_instance;
    sentinel->pivot->sentinel = self;
    return 0;
}

/* 
 * Traverse function for SentinelObject.
 * Handles GC traversal for Sentinel objects during specific GC phases.
 */
static int Sentinel_traverse(SentinelObject* self, visitproc visit, void* arg) {
    if (phase < 0) {
        Py_VISIT(self->pivot);
        return 0;
    }

    PyGC_Head *gc = AS_GC(self);

    if (phase == 0) {
        Py_VISIT(self->pivot);
        reference_manager_trigger(1, visit, arg);

        // Here is where end of decref phase begins
        phase++;
        return 0;
    }

    // Check if we are at the end of the reachable list
    PyGC_Head* reachable = arg;

    if (phase == 1 && GC_PREV(reachable) != gc) {
        if (gc_is_collecting(AS_GC(self->pivot))) {
            Py_VISIT(self->pivot);
            return 0;
        }

        // Move the pivot point to the back of the list
        //   This would be in trouble if pivot was the node before us, but that isn't possible unless we were already last
        PyGC_Head* gc2 = AS_GC(self->pivot);
        gc_list_remove(gc2);
        gc_list_append(gc2, reachable);
        gc2->_gc_prev = 6;
        return 0;
    }

    if (phase == 1) {
        // Foreign objects now see phase 2, meaning they won't renew their least
        reference_manager_trigger(2, visit, arg);
        phase++;
        if (gc_is_collecting(AS_GC(self->pivot)))
            Py_VISIT(self->pivot);
        return 0;
    }

    if (phase == 2) {
        Py_VISIT(self->pivot);
    }

    return 0;
}

/* 
 * SentinelObject type definition.
 */
static PyTypeObject SentinelType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "helloworld.Sentinel",
    .tp_basicsize = sizeof(SentinelObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc) Sentinel_traverse,
    .tp_new = PyType_GenericNew,
    .tp_init = Sentinel_init,
};

/* 
 * GC Monitoring Functions
 *
 * These functions enable and disable monitoring of Python's garbage collection process.
 * They use Python's `gc.callbacks` mechanism to hook into GC events.
 */

/* 
 * Callback function for GC events.
 * This function is called during GC cycles to track the current generation and phase.
 */
static PyObject* gc_event_callback(PyObject* self, PyObject* args) {
    const char* event;  // Event type ("start" or "stop")
    generation = -1;    // Reset generation
    PyGCState* gc_state = get_gc_state();
    if (!gc_state) return NULL;

    PyObject* details;  // Details dictionary

    // Parse the arguments: a string (event) and a dictionary (details)
    if (!PyArg_ParseTuple(args, "sO", &event, &details)) {
        return NULL;  // Return NULL on parsing failure
    }

    if (!PyDict_Check(details)) {
        PyErr_SetString(PyExc_TypeError, "Details argument must be a dictionary");
        return NULL;
    }

    // Extract the "generation" value from the details dictionary
    PyObject* generation_obj = PyDict_GetItemString(details, "generation");
    if (generation_obj != NULL) {
        generation = PyLong_AsLong(generation_obj);
    }

    // Print messages based on the event type
    if (strcmp(event, "start") == 0) {
        if (generation == 2)
            phase = 0;
        printf("GC cycle started for generation %d\n", generation);
    } else if (strcmp(event, "stop") == 0) {
        if (phase != -1)  {
            reference_manager_trigger(3, NULL, NULL);
        }

        printf("GC cycle ended for generation %d\n", generation);
        phase = -1;

        // At this point, we tell Java to drop the old leases and start with the new ones.
    } else {
        PyErr_SetString(PyExc_ValueError, "Invalid event type. Must be 'start' or 'stop'");
        return NULL;
    }

    // Special handling for generation 2
    if (phase == 0) {
        // Place the sentinel in the last gc spot so that we can mark the change of phases.
        PyGC_Head* gc = AS_GC(sentinel_instance);
        gc_list_remove(gc);
        gc_list_append(gc, GEN_HEAD(gc_state, 1));
        reference_manager_trigger(0, NULL, NULL);
    }

    Py_RETURN_NONE;  // Return None to indicate successful execution
}

// Global variable to store the callback function
static PyMethodDef my_method_def = {
    "callback",                   // Name of the method
    gc_event_callback,            // Function pointer to the callback implementation
    METH_VARARGS,                 // Method accepts a variable number of arguments
    "Callback for gc"             // Documentation string for the method
};

static PyObject* gc_event_callback_function = NULL;  // GC event callback function

/* 
 * Enable GC monitoring.
 * This function adds the callback function to Python's `gc.callbacks` list.
 */
static PyObject* enable_gc_monitoring(PyObject* self, PyObject* args) {
    // Import the `gc` module
    PyObject* gc_module = PyImport_ImportModule("gc");
    if (!gc_module) {
        PyErr_SetString(PyExc_ImportError, "Failed to import gc module");
        return NULL;
    }

    // Get the `callbacks` attribute from the `gc` module
    PyObject* gc_callbacks = PyObject_GetAttrString(gc_module, "callbacks");
    Py_DECREF(gc_module);  // Release the reference to the gc module
    if (!gc_callbacks) {
        PyErr_SetString(PyExc_AttributeError, "Failed to get gc.callbacks");
        return NULL;
    }

    // Ensure `gc.callbacks` is a list
    if (!PyList_Check(gc_callbacks)) {
        Py_DECREF(gc_callbacks);
        PyErr_SetString(PyExc_TypeError, "gc.callbacks is not a list");
        return NULL;
    }

    // Check if the callback is already stored
    if (gc_event_callback_function != NULL) {
        Py_DECREF(gc_callbacks);
        PyErr_SetString(PyExc_RuntimeError, "GC monitoring is already enabled");
        return NULL;
    }

    // Wrap the internal `gc_event_callback` function as a Python callable
    gc_event_callback_function = PyCFunction_New(&my_method_def, NULL);
    if (!gc_event_callback_function) {
        Py_DECREF(gc_callbacks);
        PyErr_SetString(PyExc_RuntimeError, "Failed to create callable for gc_event_callback");
        return NULL;
    }

    // Append the callback to the `gc.callbacks` list
    if (PyList_Append(gc_callbacks, gc_event_callback_function) < 0) {
        Py_DECREF(gc_callbacks);
        Py_DECREF(gc_event_callback_function);
        gc_event_callback_function = NULL;  // Reset the global variable
        PyErr_SetString(PyExc_RuntimeError, "Failed to append callback to gc.callbacks");
        return NULL;
    }

    Py_DECREF(gc_callbacks);  // Release the reference to gc.callbacks
    Py_RETURN_NONE;  // Return None to indicate success
}

/* 
 * Disable GC monitoring.
 * This function removes the callback function from Python's `gc.callbacks` list.
 */
static PyObject* disable_gc_monitoring(PyObject* self, PyObject* args) {
    // Import the `gc` module
    PyObject* gc_module = PyImport_ImportModule("gc");
    if (!gc_module) {
        PyErr_SetString(PyExc_ImportError, "Failed to import gc module");
        return NULL;
    }

    // Get the `callbacks` attribute from the `gc` module
    PyObject* gc_callbacks = PyObject_GetAttrString(gc_module, "callbacks");
    Py_DECREF(gc_module);  // Release the reference to the gc module
    if (!gc_callbacks) {
        PyErr_SetString(PyExc_AttributeError, "Failed to get gc.callbacks");
        return NULL;
    }

    // Ensure `gc.callbacks` is a list
    if (!PyList_Check(gc_callbacks)) {
        Py_DECREF(gc_callbacks);
        PyErr_SetString(PyExc_TypeError, "gc.callbacks is not a list");
        return NULL;
    }

    // Check if the callback is stored
    if (gc_event_callback_function == NULL) {
        Py_DECREF(gc_callbacks);  // Release the reference to gc.callbacks
        PyErr_SetString(PyExc_RuntimeError, "GC monitoring is not enabled");
        return NULL;
    }

    // Find and remove the callback from the `gc.callbacks` list
    Py_ssize_t index = PySequence_Index(gc_callbacks, gc_event_callback_function);
    if (index == -1) {
        Py_DECREF(gc_callbacks);
        PyErr_SetString(PyExc_ValueError, "Callback not found in gc.callbacks");
        return NULL;
    }

    if (PySequence_DelItem(gc_callbacks, index) < 0) {
        Py_DECREF(gc_callbacks);
        PyErr_SetString(PyExc_RuntimeError, "Failed to remove callback from gc.callbacks");
        return NULL;
    }

    Py_DECREF(gc_callbacks);  // Release the reference to gc.callbacks
    Py_DECREF(gc_event_callback_function);  // Release the reference to the callback
    gc_event_callback_function = NULL;  // Reset the global variable

    Py_RETURN_NONE;  // Return None to indicate success
}

/* 
 * Add a reference to the foreign reference list.
 * This simulates a Java-requested reference.
 */
static PyObject* reference_add(PyObject* self, PyObject* arg) {
    ForeignReference* reference = (ForeignReference*)malloc(sizeof(ForeignReference));
    reference->local_object = arg;
    reference->remote_object = NULL;
    Py_INCREF(arg);
    reference_insert(&references, reference);
    printf("ADD %p\n", arg);
    Py_RETURN_NONE;  // Return None to indicate success
}

/* 
 * Method definitions for the module.
 */
static PyMethodDef HelloWorldMethods[] = {
    {"enable", enable_gc_monitoring, METH_VARARGS, "Enable clgc"},
    {"disable", disable_gc_monitoring, METH_NOARGS, "Disable clgc"},
    {"callback", gc_event_callback, METH_VARARGS, "Monitors the gc process"},
    {"add", reference_add, METH_O, "Simulate Java requested reference"},
    {NULL, NULL, 0, NULL}  // Sentinel
};

/* 
 * Module definition.
 */
static struct PyModuleDef helloworldmodule = {
    PyModuleDef_HEAD_INIT,
    "helloworld",  // Name of the module
    "A module with GC cycle monitoring",  // Module documentation
    -1,  // Size of per-interpreter state or -1 if state is global
    HelloWorldMethods
};

/* 
 * Initialization function for the module.
 */
PyMODINIT_FUNC PyInit_helloworld(void) {
    PyObject* m;

    // Initialize the foreign reference list
    reference_init(&references);
    generation = -1;
    phase = -1;

    // Initialize the types
    if (PyType_Ready(&SentinelType) < 0) return NULL;
    if (PyType_Ready(&ForeignType) < 0) return NULL;
    if (PyType_Ready(&PivotType) < 0) return NULL;

    // Create the module
    m = PyModule_Create(&helloworldmodule);
    if (!m) return NULL;

    // Add the Sentinel type to the module
    Py_INCREF(&SentinelType);
    PyModule_AddObject(m, "Sentinel", (PyObject*)&SentinelType);

    Py_INCREF(&ForeignType);
    PyModule_AddObject(m, "Foreign", (PyObject*)&ForeignType);

    Py_INCREF(&PivotType);
    PyModule_AddObject(m, "Pivot", (PyObject*)&PivotType);

    // Create a magic tuple that allows us (and only us) to create Sentinel
    PyObject* args = PyTuple_New(0);
    magic = args;

    // Create static instances of Sentinel
    sentinel_instance = PyObject_CallObject((PyObject*)&SentinelType, args);
    if (!sentinel_instance) {
        Py_DECREF(m);
        Py_DECREF(args);
        return NULL;
    }

    Py_DECREF(args);
    Py_INCREF(sentinel_instance);
    PyModule_AddObject(m, "sentinel", sentinel_instance);

    PyGC_InstallReferenceManager(ReferenceManager_trigger);

    return m;
}

