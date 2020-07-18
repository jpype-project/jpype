from pythonforandroid.recipe import CppCompiledComponentsPythonRecipe
from pythonforandroid.toolchain import shprint, current_directory, info
from pythonforandroid.patching import will_build
import sh
from os.path import join


class JPypeRecipe(CppCompiledComponentsPythonRecipe):
    version = '1.0.1'
    url = 'https://github.com/jpype-project/jpype/archive/v{version}.zip'
    name = 'jpype'
    depends = ['setuptools']
    site_packages_name = 'jpype'
    patches = []

    call_hostpython_via_targetpython = False

    # def postbuild_arch(self, arch):
    #      super().postbuild_arch(arch)
    #      info('Copying pyjnius java class to classes build dir')
    #      with current_directory(self.get_build_dir(arch.arch)):
    #          shprint(sh.cp, '-a', join('jnius', 'src', 'org'), self.ctx.javaclass_dir)


recipe = JPypeRecipe()
