BlueTune Kit
------------

The BlueTune Kit contains all the software packages necessary to build the 
BlueTune framework, plugins, and sample applications.
Each one of those software packages is contained in a directory at the top 
level of the BlueTune Kit distribution.

The file VERSION.txt contains the version information of all the modules
included in this kit.

The top-level modules are:

BlueTune
--------
The BlueTune framework, plugins and applications are located in the BlueTune 
top-level directory. 
Also located there, in BlueTune/README.txt and under the 'BlueTune/Docs' 
sub-directory, are documentation files with more details about how to build 
and use the BlueTune SDK.

Atomix
------
Atomix is a portable C runtime library that includes support for object 
oriented programming with the notion of interfaces. It is used througout
the BlueTune implementation.

Neptune
-------
Neptune is a portable C++ runtime class library. It is used by the BlueTune
implementation in the part of the implementation written in C++.

Melo
----
Melo is a portable AAC decoder. It is used by the AAC Decoder Module included
in the standard BlueTune plugin modules.

Bento4
------
Bento4 is a portable MP4 file format parser. It is used by the MP4 Parser Module
included in the standard BlueTune plugin modules.
