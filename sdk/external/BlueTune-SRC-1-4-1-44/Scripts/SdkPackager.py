#! /usr/bin/python

########################################################################
#      
#      Packaging Script for the BlueTune SDK
#
#      Original author:  Gilles Boccon-Gibod
#
#      Copyright (c) 2003-2007 by Axiomatic Systems LLC. 
#       All rights reserved.
#
#######################################################################

import sys
import os
import shutil
import glob
import datetime
import fnmatch
import zipfile
import re

#############################################################
# Constants
#############################################################
BLUETUNE_KIT_VERSION = '1-4-1'

#############################################################
# GetSvnRevision
#############################################################
def GetSvnRevision():
    cmd = 'svn info'
    revision = 0
    lines = os.popen(cmd).readlines()
    for line in lines:
        if line.startswith('Revision: '):
            revision = line[10:] 
    if revision == 0:
        raise "unable to obtain SVN revision"
    return revision.strip()

#############################################################
# File Copy
#############################################################
def CopyFiles(file_patterns, configs=[''], rename_map={}):
    for config in configs:
    	if len(config): print config+':'
        for (source,pattern,dest) in file_patterns:
            print source, pattern
            source_dir = BLUETUNE_KIT_HOME+'/'+source+'/'+config
            file_list = glob.glob(source_dir+'/'+pattern)
            for file in file_list:
                dest_dir = SDK_ROOT+'/'+dest+'/'+config
                if not os.path.exists(dest_dir):
                    os.makedirs(dest_dir)
                filename = os.path.basename(file)
                if rename_map.has_key(filename):
                    dest_name = dest_dir+'/'+rename_map[filename]
                else:
                    dest_name = dest_dir
                #print 'COPY: '+file+' -> '+dest_name
                if os.path.isfile(file): shutil.copy2(file, dest_name)
        
#############################################################
# ZIP support
#############################################################
def ZipDir(top, archive, dir) :
    #print 'ZIP: ',top,dir
    entries = os.listdir(top)
    for entry in entries: 
        path = os.path.join(top, entry)
        if os.path.isdir(path):
            ZipDir(path, archive, os.path.join(dir, entry))
        else:
            zip_name = os.path.join(dir, entry)
            #print 'ZIP: adding '+path+' as '+zip_name
            archive.write(path, zip_name)

def ZipIt(basename, dir) :
    path = basename+'/'+dir
    zip_filename = path+'.zip'
    print 'ZIP: '+path+' -> '+zip_filename
   
    if os.path.exists(zip_filename):
        os.remove(zip_filename)

    archive = zipfile.ZipFile(zip_filename, "w", zipfile.ZIP_DEFLATED)
    if os.path.isdir(path):
        ZipDir(path, archive, dir)
    else:
        archive.write(path)
    archive.close()
    
#############################################################
# Main
#############################################################
# parse the command line
BLUETUNE_KIT_HOME = os.path.abspath(os.path.dirname(__file__))+'/..'

# hardcode the target for now
SDK_TARGET=sys.argv[1]

# ensure that BLUETUNE_KIT_HOME has been set and exists
if not os.path.exists(BLUETUNE_KIT_HOME) :
    print 'ERROR: BLUETUNE_KIT_HOME ('+BLUETUNE_KIT_HOME+' does not exist'
    sys.exit(1)
else :
    print 'BLUETUNE_KIT_HOME = ' + BLUETUNE_KIT_HOME

# compute paths
SDK_REVISION = GetSvnRevision()
SDK_NAME='BlueTune-SDK-'+BLUETUNE_KIT_VERSION+'-'+SDK_REVISION+'_'+SDK_TARGET
SDK_BUILD_ROOT=BLUETUNE_KIT_HOME+'/SDK'
SDK_ROOT=SDK_BUILD_ROOT+'/'+SDK_NAME
SDK_TARGET_DIR='BlueTune/Build/Targets/'+SDK_TARGET
SDK_TARGET_ROOT=BLUETUNE_KIT_HOME+'/'+SDK_TARGET_DIR
            
# platform stuff
if sys.platform == 'cygwin':
    EXEC_EXT = '.exe'
else:
    EXEC_EXT = ''
    
print SDK_NAME

# remove any previous SDK directory
if os.path.exists(SDK_ROOT):
    shutil.rmtree(SDK_ROOT)
    
# copy single-config files
single_config_files = [
    ('BlueTune/Source/BlueTune', '*.h','include'),
    ('BlueTune/Source/Core', '*.h','include'),
    ('BlueTune/Source/Decoder', '*.h','include'),
    ('BlueTune/Source/Player', '*.h','include'),
    ('BlueTune/Source/Plugins', '*/*.h','include'),
    ('BlueTune/Source/Plugins', '*/*/*.h','include')
]
CopyFiles(single_config_files)

# copy multi-config files
multi_config_files = [
    (SDK_TARGET_DIR,'btplay'+EXEC_EXT,'Targets/'+SDK_TARGET+'/bin'),
    (SDK_TARGET_DIR,'btcontroller'+EXEC_EXT,'Targets/'+SDK_TARGET+'/bin'),
    (SDK_TARGET_DIR,'btremote'+EXEC_EXT,'Targets/'+SDK_TARGET+'/bin'),
    (SDK_TARGET_DIR,'*.exe','Targets/'+SDK_TARGET+'/bin'),
    (SDK_TARGET_DIR,'*.a','Targets/'+SDK_TARGET+'/lib'),
    (SDK_TARGET_DIR,'*.lib','Targets/'+SDK_TARGET+'/lib'),
]
CopyFiles(multi_config_files, configs=['Debug','Release'])

# remove any previous zip file
ZipIt(SDK_BUILD_ROOT, SDK_NAME)
