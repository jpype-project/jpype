import sys
import os
import sysconfig
import platform

gcv = sysconfig.get_config_var

def get_lib():
    # Windows library location logic
    if os.name == 'nt':
        v = gcv('VERSION').replace('.', '')
        # Usually in the same folder as the executable (BINDIR)
        return os.path.join(gcv('BINDIR'), f'python{v}.dll')
    
    # Unix library location logic
    # LIBRARY is the static lib, LDLIBRARY is the shared lib
    return os.path.join(gcv('LIBDIR'), gcv('LDLIBRARY'))

# Attempt to load JPype to get internal paths
try:
    import _jpype
    jp_file = _jpype.__file__
    jp_ver = _jpype.__version__
except ImportError:
    jp_file = "NOT_FOUND"
    jp_ver = "NOT_FOUND"

def clean(path):
    if path is None:
        return ''
    # Convert backslashes to forward slashes to avoid Java Property escape issues
    return str(path).replace('\\', '/')

# Clean version
sys_paths = [clean(p) for p in sys.path if p]

# The output is formatted specifically for Java's Properties.load()
print(f'python.config.home={clean(sys.prefix)}')
print(f'python.config.base_home={clean(sys.base_prefix)}')
print(f'python.config.path={os.pathsep.join(sys_paths)}')
print(f'python.lib={clean(get_lib())}')
print(f'jpype.lib={clean(jp_file)}')
print(f'jpype.version={jp_ver}')

# Architecture string for pip self-healing logic
arch = f'{platform.system().lower()}_{platform.machine().lower()}'
print(f'jpype.arch={arch}')
