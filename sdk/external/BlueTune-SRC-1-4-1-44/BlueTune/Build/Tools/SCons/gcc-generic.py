import os

def generate(env, gcc_cross_prefix=None, gcc_extra_options='', gcc_strict=True, gcc_stop_on_warning=None):
    ### the environment provides default values for the parameters set to None
    if gcc_stop_on_warning == None: gcc_stop_on_warning = env['stop_on_warning']
    
    ### compiler flags
    if gcc_strict:
        env.AppendUnique(CCFLAGS = ['-pedantic', '-Wall', '-W', '-Wno-long-long'])
        env.AppendUnique(CFLAGS  = ['-Wmissing-prototypes', '-Wmissing-declarations'])
    else:
        env.AppendUnique(CCFLAGS = ['-Wall'])
    
    env.AppendUnique(CPPFLAGS = ['-D_REENTRANT'])
    
    if env['build_config'] == 'Debug':
        env.AppendUnique(CCFLAGS = ['-g'])
    else:
        env.AppendUnique(CCFLAGS = ['-O3', '-fomit-frame-pointer', '-ffunction-sections', '-fdata-sections'])
    
    if gcc_stop_on_warning:
        env.AppendUnique(CCFLAGS = ['-Werror'])
        
    if gcc_cross_prefix:
        env['ENV']['PATH'] += os.environ['PATH']
        env['AR']     = gcc_cross_prefix+'-ar'
        env['AS']     = gcc_cross_prefix+'-as'
        env['RANLIB'] = gcc_cross_prefix+'-ranlib'
        env['CPP']    = gcc_cross_prefix+'-cpp'
        env['CC']     = gcc_cross_prefix+'-gcc ' + gcc_extra_options
        env['CXX']    = gcc_cross_prefix+'-g++ ' + gcc_extra_options
        env['LINK']   = gcc_cross_prefix+'-g++ -Wl,-gc-sections ' + gcc_extra_options
