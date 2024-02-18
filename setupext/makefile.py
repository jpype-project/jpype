

class Makefile:
    compiler_type = "unix"

    def __init__(self, actual):
        self.actual = actual
        self.compile_command = None
        self.compile_pre = None
        self.compile_post = None
        self.objects = []
        self.sources = []

    def captureCompile(self, x):
        command = x[0]
        x = x[1:]
        includes = [i for i in x if i.startswith("-I")]
        x = [i for i in x if not i.startswith("-I")]
        i0 = None
        i1 = None
        for i, v in enumerate(x):
            if v == '-c':
                i1 = i
            elif v == '-o':
                i0 = i
        pre = set(x[:i1])
        post = x[i0 + 2:]

        self.compile_command = command
        self.compile_pre = pre
        self.compile_post = post
        self.includes = includes
        self.sources.append(x[i1 + 1])

    def captureLink(self, x):
        self.link_command = x[0]
        x = x[1:]
        i = x.index("-o")
        self.library = x[i + 1]
        del x[i]
        del x[i]
        self.objects = [i for i in x if i.endswith(".o")]
        self.link_options = [i for i in x if not i.endswith(".o")]
        u = self.objects[0].split("/")
        self.build_dir = "/".join(u[:2])

    def compile(self, *args, **kwargs):
        self.actual.spawn = self.captureCompile
        rc = self.actual.compile(*args, **kwargs)
        return rc

    def _need_link(self, *args):
        return True

    def link_shared_object(self, *args, **kwargs):
        self.actual._need_link = self._need_link
        self.actual.spawn = self.captureLink
        rc = self.actual.link_shared_object(*args, **kwargs)
        self.write()
        return rc

    def detect_language(self, x):
        return self.actual.detect_language(x)

    def write(self):
        print("Write makefile")
        library = os.path.basename(self.library)
        link_command = self.link_command
        compile_command = self.compile_command
        compile_pre = " ".join(list(self.compile_pre))
        compile_post = " ".join(list(self.compile_post))
        build = self.build_dir
        link_flags = " ".join(self.link_options)
        includes = " ".join(self.includes)
        sources = " \\\n     ".join(self.sources)
        with open("Makefile", "w") as fd:
            print("LIB = %s" % library, file=fd)
            print("CC = %s" % compile_command, file=fd)
            print("LINK = %s" % link_command, file=fd)
            print("CFLAGS = %s %s" % (compile_pre, compile_post), file=fd)
            print("INCLUDES = %s" % includes, file=fd)
            print("BUILD = %s" % build, file=fd)
            print("LINKFLAGS = %s" % link_flags, file=fd)
            print("SRCS = %s" % sources, file=fd)
            print("""
all: $(LIB)

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
#build/src/jp_thunk.cpp: $(call rwildcard,native/java,*.java)
#	python setup.py build_thunk

DEPDIR = build/deps
$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%.cpp=$(DEPDIR)/%.d)

deps: $(DEPFILES)

%/:
	echo $@

$(DEPDIR)/%.d: %.cpp 
	mkdir -p $(dir $@)
	$(CC) $(INCLUDES) -MT $(patsubst $(DEPDIR)%,'$$(BUILD)%',$(patsubst %.d,%.o,$@)) -MM $< -o $@

OBJS = $(addprefix $(BUILD)/, $(SRCS:.cpp=.o))


$(BUILD)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


$(LIB): $(OBJS)
	$(LINK) $(OBJS) $(LINKFLAGS) -o $@


-include $(DEPFILES)
""", file=fd)

