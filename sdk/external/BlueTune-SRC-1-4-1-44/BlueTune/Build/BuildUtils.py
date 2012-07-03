from SCons.Script import *
from glob import glob
import imp

#######################################################
# reusable functions and data structures
#######################################################

### Map used to map directories that lie outside of the
### SCons directory hierarchy (ex: thirdparty code bases)
### Use the function MapSourceDir() to add an entry to this map
SourceDirMap = {}

### Default build environment. It can be changed by calling
### SetDefaultEnv()
DefaultEnv = Environment()

### Set the global variable DefaultEnv
def SetDefaultEnv(env):
    global DefaultEnv
    DefaultEnv = env
    
### Add and entry to the SourceDirMap() map
def MapSourceDir(key, val):
    SourceDirMap[key] = val
    
### Load an scons tool configuration for a specific configuration
def LoadTool(name, env, **kw):
    config_path = GetBuildPath('#/Build/Tools/SCons')
    file, path, desc = imp.find_module(name, [config_path])
    module = imp.load_module(name, file, path, desc)
    module.generate(env, **kw)
    
### Merge several lists into one, eliminating duplicate items
def MergeListsUnique(lists):
    result = []
    for list in lists:
        for item in list:
            if item not in result: result.append(item)
    return result

### Return a list of name of files in a directory that match one or more patterns  
def GlobSources(directory, patterns, excluded_files=[]):
    root = GetBuildPath(GetDirPath(directory))
    files = []
    for pattern in Split(patterns):
        files += glob(root+'/'+pattern)
    return [directory+'/'+os.path.basename(x) for x in  files if os.path.basename(x) not in excluded_files]

### Return the scons-style directory path for a directory name, with support
### for the global source directory map
def GetDirPath(directory):
    if directory.startswith('/'): return directory
    for key in SourceDirMap.keys():
        if directory.startswith(key):
            return SourceDirMap[key]+directory[len(key):]
    return '#/'+directory

### Construct a list of module names with all the modules
### passed as input and their dependencies (and so on recursively)
### The dependency order is not important
def FlattenIncludeDeps(modules):
    deps = []
    for module in modules:
        module_obj = Modules[module]
        if module_obj.name not in deps: deps.append(module_obj.name)
        for dep in module_obj.flat_include_deps:
            if dep not in deps: deps.append(dep)
    return deps

### Construct a list of module names with all the modules
### passed as input and their dependencies (and so on recursively)
### The dependency order is preserved
def FlattenLinkDeps(modules):
    deps = []
    for module in modules:
        module_obj = Modules[module]
        module_deps = [module]+module_obj.flat_link_deps
        deps = [dep for dep in deps if dep not in module_deps]+module_deps
    return deps

### Construct a list of module names with all the modules
### passed as input and their dependencies (and so on recursively)
### The dependency order is preserved
def FlattenObjectDeps(modules):
    deps = []
    for module in modules:
        module_obj = Modules[module]
        module_deps = [module]+module_obj.flat_object_deps
        deps = [dep for dep in deps if dep not in module_deps]+module_deps
    return deps

### Return the list of include directories necessary when depending on
### the modules passed as the input parameter.
def GetIncludeDirs(modules):
    deps = MergeListsUnique([Modules[mod].flat_include_deps for mod in modules])
    return MergeListsUnique([Modules[mod].include_dirs for mod in modules+deps])

### Return the list of libraries necessary when depending on
### the modules passed as the input parameter.
def GetLibs(modules):
    deps = MergeListsUnique([Modules[mod].flat_link_deps for mod in modules])
    return MergeListsUnique([Modules[mod].libs for mod in modules+deps])

### Return the list of library directories necessary when depending on
### the modules passed as the input parameter.
def GetLibDirs(modules):
    deps = MergeListsUnique([Modules[mod].flat_link_deps for mod in modules])
    return MergeListsUnique([Modules[mod].lib_dirs for mod in modules+deps])

### Return the list of objects for a list of modules and their dependents 
### (and so on recursively), only if the module is of type 'Object'
def GetObjects(modules):
    deps = MergeListsUnique([Modules[mod].flat_object_deps for mod in modules])
    return MergeListsUnique([Modules[mod].objects for mod in modules+deps])

### Return the list of products for a list of modules and their dependents 
### (and so on recursively)
def GetProducts(modules):
    deps = MergeListsUnique([Modules[mod].flat_link_deps for mod in modules])
    return MergeListsUnique([Modules[mod].product for mod in modules+deps])

### Return the scons nodes corresponding to module names
def GetModuleSconsNodes(modules):
    return [Modules[module].nodes for module in modules]

###########################################################
# The Module class is the base class for all modules.
# It encapsulates the module properties, such as the libs
# to link with, library paths, objects, dependents, etc...
###########################################################
Modules = {}
class Module:
    def __init__(self, name,
                 module_type                    = None,
                 chained_link_only_deps         = [], 
                 chained_link_and_include_deps  = [],
                 chained_include_only_deps      = [],
                 include_dirs                   = [],
                 libs                           = [],
                 lib_dirs                       = []):
        self.name                          = name
        self.module_type                   = module_type
        self.chained_link_only_deps        = Split(chained_link_only_deps)
        self.chained_link_and_include_deps = Split(chained_link_and_include_deps)
        self.chained_include_only_deps     = Split(chained_include_only_deps)
        self.libs                          = Split(libs)
        self.lib_dirs                      = [GetDirPath(dir) for dir in Split(lib_dirs)]
        self.include_dirs                  = Split(include_dirs)
        self.objects                       = []
        Modules[name]                      = self        

        self.flat_include_deps = FlattenIncludeDeps(self.GetIncludeDeps())
        self.flat_link_deps    = FlattenLinkDeps(self.GetLinkDeps())
        self.flat_object_deps  = FlattenObjectDeps(self.GetObjectDeps())
            
    def GetLinkDeps(self):
        return self.chained_link_and_include_deps+self.chained_link_only_deps
    
    def GetObjectDeps(self):
        return [dep for dep in self.chained_link_and_include_deps+self.chained_link_only_deps if Modules[dep].module_type == 'Objects']

    def GetIncludeDeps(self):
        return self.chained_link_and_include_deps+self.chained_include_only_deps
    
#######################################################
# The CompiledModule class declares a code module built from source files.
# module_type: string, selects the type of module to build
# 'Objects' compiles sources into object files suitable for building a shared or static library
# 'Executable' compiles sources into an executable program
#
# source_root: directory relative to which the source file names are given. An empty string
# is the default and means that source file names are absolute names.
#
# build_source_dirs: list of one or more directories in which to look for source files, relative to
# source_root
#
# build_source_patterns: list of shell glob pattern that selects the names of the source files to compiler
# (within the build_source_dirs)
#
# build_source_files: an optional list of other files to build specified as a list of (dirname,pattern) tuples
#
# build_include_dirs: list of directories, relative to source_root, to add to the include search path when
# compiling the sources. 
#
# build_include_deps: dependencies that are needed for the include path when building this module
# but not when building its dependents.
#
# excluded_files: list of files 
#
# export_build_source_dirs: boolean flag that indicates if this module's build_source_dirs will be part of
# its dependts' include search path.
#
# exported_include_dirs: list of directories that will be added to this module's dependents' include search path.
# All those directories will also be part of the include search path when building this module.
#
# chained_link_and_include_deps: dependencies that are needed to build this module and that
# its dependents will need to include and link with.
#
# chained_link_only_deps: dependencies that are needed to build this module and that
# its dependents will need to link with, but not include.
#
# chained_include_only_deps: dependencies that are needed to build this module and that
# its dependents will need to include but not link with.
#
# extra_cpp_defines: list of preprocessor symbols or tuples of (symbol,value) that
# will be passed to the compiler as extra preprocessor definitions.
#
# extra_libs: list of extra libraries that this module needs when linked.
#
# extra_lib_dirs: list of directories to add to the library search path when
# linking this module.
#
# force_non_shared: force the compiler to build non-shared objects. Only use this
# if building shared objects is not working because of compiler problems or
# dues to assembler routines not working correctly.
#
# environment: SCons environment object with the build context/options for this module. By default,
# the default environment set by calling SetDefaultEnv()
#
#######################################################
class CompiledModule(Module):
    def __init__(self, name, 
                 module_type                   = 'Objects',
                 source_root                   = '',
                 build_source_dirs             = ['.'], 
                 build_source_patterns         = ['*.c', '*.cpp'], 
                 build_source_files            = [],
                 build_include_dirs            = [],
                 build_include_deps            = [],
                 excluded_files                = [],
                 export_build_source_dirs      = True,
                 exported_include_dirs         = [],
                 chained_link_and_include_deps = [], 
                 chained_link_only_deps        = [],
                 chained_include_only_deps     = [],
                 extra_cpp_defines             = [],
                 extra_libs                    = [],
                 extra_lib_dirs                = [],
                 object_name_map               = {},
                 force_non_shared              = False,
                 environment                   = None):

        # build_source_dirs are relative to source_root
        if build_source_dirs:
            build_source_dirs = [source_root+'/'+directory for directory in Split(build_source_dirs)]        
        
        # create the superclass and store this new object in the module dictionary
        Module.__init__(self, 
                        name,
                        module_type,
                        chained_link_and_include_deps = chained_link_and_include_deps, 
                        chained_link_only_deps        = chained_link_only_deps,
                        chained_include_only_deps     = chained_include_only_deps,
                        include_dirs                  = Split(exported_include_dirs)+
                                                        (export_build_source_dirs and Split(build_source_dirs) or []),
                        libs                          = extra_libs,
                        lib_dirs                      = extra_lib_dirs)

        # setup the build environment        
        env = environment or DefaultEnv
   
        # for each source dir to build, create a BuildDir
        # to say where we want the object files to be built,
        # and compute the list of source files to build
        sources = []
        for directory in build_source_dirs:
            env.BuildDir(directory, GetDirPath(directory), duplicate=0)
            sources += GlobSources(directory, build_source_patterns, excluded_files)
            
        # add cherry-picked files
        for entry in build_source_files:
            if type(entry) is tuple:
                (directory,pattern) = entry
            else:
                directory = entry
                pattern = build_source_files[entry]
                
            if directory.startswith('/'):
                directory_path = directory[1:]
            else:
                directory_path = source_root+'/'+directory
            env.BuildDir(directory_path, GetDirPath(directory_path), duplicate=0)
            sources += GlobSources(directory_path, pattern)

        # check that the source list is not empty
        if len(sources) == 0 and build_source_dirs:
            raise 'Module '+name+' has no sources, build_source_dirs='+str(build_source_dirs)
        
        # calculate our build include path
        my_inc_dirs  = build_include_dirs+build_source_dirs+exported_include_dirs+GetIncludeDirs(build_include_deps)
        dep_inc_dirs = GetIncludeDirs(self.chained_link_and_include_deps +
                                      self.chained_include_only_deps     +
                                      self.chained_link_only_deps)
        cpp_path = [GetDirPath(dir) for dir in my_inc_dirs+dep_inc_dirs]
        if env.has_key('CPPPATH'): cpp_path = env['CPPPATH']+cpp_path
        cpp_path += ['#/../../../include']
        cpp_path += ['#/../../../os/kernel-2.6.32/arch/arm/mach-gpl32900/include']
        cpp_path += ['#/../../../os/kernel-2.6.32/arch/arm/mach-spmp8050/include']
        # compute preprocessor definitions and include path
        cpp_defines = extra_cpp_defines
        if env.has_key('CPPDEFINES'): cpp_defines = env['CPPDEFINES']+cpp_defines
        
        # compile the sources
        if force_non_shared:
            generator = env.StaticObject
        else:
            generator = env.SharedObject
        objects = []
        for x in sources:
            if x in object_name_map:
                object_name = object_name_map[x]
                objects.append(generator(target=object_name, source=x, CPPPATH=cpp_path, CPPDEFINES=cpp_defines))
            else:
                objects.append(generator(source=x, CPPPATH=cpp_path, CPPDEFINES=cpp_defines))
        if module_type == 'Objects':
            self.nodes   = []
            self.objects = objects
        elif module_type == 'Executable':
            link_deps = self.flat_link_deps
            libs = GetProducts(link_deps)
            if env.has_key('LIBS'): libs += env['LIBS']
            libs += GetLibs(link_deps)
            libs += self.libs
	    libs += ['gpacodec','mp3dec','oggdec','aacdec','ra8lbrdec','wmadec']
            lib_path = env.has_key('LIBPATH') and env['LIBPATH'] or []
            lib_path += GetLibDirs(self.GetLinkDeps())
	    lib_path +=self.lib_dirs	
	    lib_path += ['#/../../../lib']
            self.nodes = env.Program(target=name.lower(), source=objects, LIBS=libs, LIBPATH=lib_path)
            self.objects = []
        else:
            raise Exception('Unknown Module Type')
            
        self.product = self.nodes   
        env.Alias(name, self.nodes)

############################################################################
# Subclass of CompiledModule used when building an executable.
# This this type of module is terminal (no other module will depend on it),
# the constructor ommits the parameters related to chained dependencies.
# The other construction parameters have the same meaning as the ones
# for the parent class.
############################################################################
class ExecutableModule(CompiledModule):
    def __init__(self, name,
                 source_root           = '',
                 build_source_dirs     = ['.'], 
                 build_source_patterns = ['*.c', '*.cpp'], 
                 build_source_files    = [],
                 build_include_dirs    = [],
                 build_include_deps    = [],
                 excluded_files        = [],
                 extra_libs            = [],
                 extra_lib_dirs        = [],
                 extra_cpp_defines     = [],
                 link_and_include_deps = [],
                 include_only_deps     = [],
                 environment           = None):
        CompiledModule.__init__(self, name,
                                module_type                   = 'Executable',
                                source_root                   = source_root,
                                build_source_dirs             = build_source_dirs, 
                                build_source_patterns         = build_source_patterns, 
                                build_source_files            = build_source_files,
                                build_include_dirs            = build_include_dirs,
                                chained_link_only_deps        = link_and_include_deps,
                                chained_include_only_deps     = include_only_deps,
                                excluded_files                = excluded_files,
                                extra_cpp_defines             = extra_cpp_defines,
                                extra_libs                    = extra_libs,
                                extra_lib_dirs                = extra_lib_dirs,
                                environment                   = environment)
    
############################################################################
# Subclass of Module used when building a shared library.
#
# library_name: name to used for the shared library, if it needs to be
# different from the module name.
#
# anchor_module: name of a module that will be used as an 'anchor', which is
# a set of object files that are given to the linker to produce the shared
# library. The reason for using an anchor is that most gnu linkers are not
# capable of producing a library from other libraries. This trick uses the
# object files from the 'anchor' as the input to the linker, and passes
# other dependent objects libraries.
# NOTE: it is important to choose the anchor well, as the linker will
# only include in the final library symbols that are referenced from one
# of the objects in the anchor.
#
# link_deps: list of modules to link with when linking the library.
#
# exported_include_dirs: same as for the Module() constructor.
#
############################################################################
class SharedLibraryModule(Module):
    def __init__(self, name,
                 library_name          = None,
                 anchor_module         = '',
                 link_deps             = [],
                 exported_include_dirs = [],
                 environment           = None) :
        Module.__init__(self,
                        name,
                        module_type  = 'SharedLibrary',
                        include_dirs = exported_include_dirs)
        
         # setup the environment        
        env = environment or DefaultEnv
        
        # compute the list of objects to link
        objects = Modules[anchor_module].objects
        all_deps = [anchor_module]+link_deps
        libs = GetProducts(all_deps)+GetLibs(all_deps)
        lib_path = env.has_key('LIBPATH') and env['LIBPATH'] or []
        lib_path += GetLibDirs(all_deps)
        
        if library_name:
            self.nodes = env.SharedLibrary(target=library_name, SHLIBPREFIX='', source=objects, LIBS=libs, LIBPATH=lib_path)
        else:
            self.nodes = env.SharedLibrary(target=name, source=objects, LIBS=libs, LIBPATH=lib_path)
        
        ### we must use just the basename of the shared library here so that the dynamic linker won't
        ### try to keep the build-time root-relative path name of the lib when linking
        self.libs     = [str(self.nodes[0])[len(env['LIBPREFIX']):-len(env['SHLIBSUFFIX'])]] ## strip the lib prefix and suffix
        self.lib_dirs = [''] # this will expand to 'Build/Targets/'+env['target']+'/'+env['build_config']]
        self.product  = []
        env.Alias(name, self.nodes)

############################################################################
# Subclass of Module used when building a static library.
#
# The library will include all the object files of the modules on
# which it depends (and their dependent objects, recursively).
#
# library_name: name to used for the shared library, if it needs to be
# different from the module name.
#
############################################################################
class StaticLibraryModule(Module):
    def __init__(self,
                 name,
                 module_type                   = 'StaticLibrary',
                 library_name                  = None,
                 chained_link_and_include_deps = [], 
                 chained_link_only_deps        = [],
                 chained_include_only_deps     = [],
                 environment                   = None) :
        Module.__init__(self, name,
                        chained_link_only_deps        = chained_link_only_deps,
                        chained_include_only_deps     = chained_include_only_deps,
                        chained_link_and_include_deps = chained_link_and_include_deps)
   
        # setup the environment        
        env = environment or DefaultEnv
        
        # compute the list of objects to link
        objects   = GetObjects(chained_link_only_deps+chained_link_and_include_deps)
        self.libs = GetLibs(   chained_link_only_deps+chained_link_and_include_deps)
        
        self.nodes = env.StaticLibrary(target=(library_name and library_name or name), source=objects)
        self.product = self.nodes
        env.Alias(name, self.nodes)
        
#####################################################################
# Exports
#####################################################################
__all__ = ['LoadTool', 'CompiledModule', 'SharedLibraryModule', 'StaticLibraryModule', 'ExecutableModule', 'MapSourceDir', 'SetDefaultEnv', 'GetModuleSconsNodes']
