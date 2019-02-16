
ACL Enabled File System
**********

.. contents:: **Contents of this document**
   :depth: 2


Get Started
===========

Starting server
---------------
Step 1.
Run the following command on shell to compile the binaries from inside of server/slash/bin dir

.. code:: shell

  make
  
 
Step 2.
Supported commands can be run from server directory in following manner

.. code:: shell

  slash/bin/ls <dir>
  
 

Supported commands are mentioned below.

Commands Supported
==================

getacl
------

.. code:: shell

  getacl <file>
  
List the acls for the file. A user can getacl only if it is the owner or root
 
 
setacl
------

.. code:: shell

  setacl <-m|-x> <acl entry> <file>
  
set the acls for the file. A user can setacl only if it is the owner or root. **acl entry** is similar to the actual acl entries in linux. Example of acl entry is **u:user1:rw-**
-m option is to moodify the acl entry and -x option is for deleting the acl entry. 
 
ls
--

.. code:: shell

  ls <dir>
  
List the contents of dir. A user can do ls only if it has the read access for that directory in acls 
  
create_dir
----------

.. code:: shell

  create_dir <dir>
  
Create a directory dir. A user can create dir only if it is the owner of the parent dir. dir can be absolute or realtive path.
User is then prompted for the **owner** and **group** of the dir. If nothing passed, default grp and owner associations are inherited from parent dir.


fget
----

.. code:: shell

  fget <file>
  
Get the contents of file. A user can read the file only if it has read access for the file.
  

fput
----

.. code:: shell

  fput <file>
  
Create file or append to file if it is already created. A user can create file only if it has write access of the parent dir. If the file is already created then append mode is started. A user can append to file only if it is the owner of the file. file can be absolute or realtive path. User can finish appending to file by typing **end** in a newline.

  

do_exec
-------

.. code:: shell

  do_exec <file>
  
Run the file with permissions of the owner.


Project Dir Structure
=====================

The server's directory is maintained as shown.

server/
    slash/
        etc/
          passwd
        bin/
          ls
          ls.c
          getacl
          getacl.c
          setacl
          setacl.c
          do_exec
          do_exec.c
          .
          .
          .
        home/
          u1/
          u2/
          .
          .
          .
    

slash/etc/passwd stores the user and group associations
slash/home/ui is the home directory for ith user


Security Rules
==============

- a user is given write access to a dir or file only if it is the owner
- a user is given read access to a dir or file only if is is the owner or is member of the group of corresponding file or dir
- by default users are denied connection or any access if not authenticated
- a user can be in multiple groups
- a file or dir can have only one owner and only one group



Assumptions
============

- root is the owner and group of **slash, home** directory
- the group and user associations are stored in **server/etc/passwd**. The username and groups are hardwired from this file. Each line of the file contains entry for a user. Names are separated by " " (single blank space), where first name is the username and subsequent names in the line are groups of the user
- **/home/ui** directory has ui itself as the owner and group
- a user can be in maximum 10 groups
- set of group names are same as set of user names
- only absolute paths can be entered, with base directory as **server/**


Bugs and Errors defended
=============

- paths entered as arguments are validated
- wrong arguments or inputs for acls, etc. are gracefully handled
- making acl entry for non existent users is not allowed
- access behind slash is not allowed
- handling overwriting of directory that is already created



Developed by Sushant Kumar Singh
