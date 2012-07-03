#! /usr/bin/python

import os

#############################################################
# This tool is used to generate the version info file       #
#############################################################

# get the SVN repo versions
atomix_version   = os.popen('svnversion -n Atomix').readlines()[0]
neptune_version  = os.popen('svnversion -n Neptune').readlines()[0]
melo_version     = os.popen('svnversion -n Melo').readlines()[0]
bento4_version   = os.popen('svnversion -n Bento4').readlines()[0]
bluetune_version = os.popen('svnversion -n BlueTune').readlines()[0]

print "BlueTune Kit Version 1.4"
print "------------------------"
print 
print "This kit is composed of modules with the following SVN repository versions:"
print 
print "BlueTune:", bluetune_version
print "Atomix:  ",   atomix_version
print "Neptune: ",  neptune_version
print "Melo:    ",     melo_version
print "Bento4:  ",   bento4_version
