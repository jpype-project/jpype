import helloworld
import gc

def make_garbage():
    """
    Creates objects with cyclic references and allocates large amounts of memory
    to simulate garbage that needs to be cleaned up by the garbage collector.
    """
    # Create a list to hold references
    garbage_list = []

    # Create cyclic references
    for i in range(20):  # Adjust the range to control the amount of garbage
        obj_a = {"name": f"Object_{i}"}
        obj_b = {"ref": obj_a}
        obj_a["ref"] = obj_b  # Create a cycle between obj_a and obj_b
        garbage_list.append(obj_a)  # Add to the list to prevent immediate cleanup

    # Create a large number of objects for memory allocation
    #large_data = [bytearray(1024 * 1024) for _ in range(10)]  # Allocate 10 MB of data

    # Simulate temporary garbage
    temp_garbage = [{"temp": i} for i in range(10)]

    # Drop references to some objects (simulate garbage collection)
    del temp_garbage

    print("Garbage created. Check memory usage or trigger garbage collection.")

    # Return the garbage_list to keep it alive for debugging purposes
    return garbage_list


# Callback function to handle GC events
#def gc_callback(event, details):
#    if event == "start":
#        generation = details.get("generation", "unknown")
#        print(f"GC cycle started for generation {generation}")
#    elif event == "stop":
#        generation = details.get("generation", "unknown")
#        print(f"GC cycle ended for generation {generation}")
#    print(helloworld.inspect_gc_generations())
#
## Enable GC monitoring
#gc.callbacks.append(gc_callback)
#
helloworld.enable()

class A():
    pass

class B():
    def __init__(self):
        self.a = helloworld.Foreign()
        self.b = helloworld.Foreign()
    pass


# Trigger a GC cycle manually
a = A()
b = B()
c = b.b
helloworld.add(a)
helloworld.add(b)
helloworld.add({"a":A()})
#helloworld.add(helloworld.Foreign())
del b

#gc.collect(1)  # Collect generation 0
gc.collect(2)  # Collect generation 0
print("----------")

make_garbage()
gc.collect(2)  # Collect generation 1
print("----------")

#make_garbage()
#gc.collect(2)  # Collect generation 2
#print("----------")

helloworld.disable()
print("Disabled")

gc.collect(2)  # Collect generation 2
# Disable GC monitoring
#gc.callbacks.remove(gc_callback)
