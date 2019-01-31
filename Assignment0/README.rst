
Multi User Remote File System
**********

**This is a rudimentary remote file system shell running on Multi-threaded TCP server.**


.. contents:: **Contents of this document**
   :depth: 2


Get Started
===========

Starting server
---------------
Run the following command on shell to start the server when inside server/ dir

.. code:: shell

  make runserver
  
  
Starting client
---------------
Step 1.
Run the following to start a client.


.. code:: shell

  make runclient

 
Step 2.
Enter the server IP Address

Step 3.
Authenticate the user by typing in username (password not required)

Now the user is presented with a shell. Supported commands are mentioned below.

Commands Supported
==================
cd
--

.. code:: shell

  cd [dir]
  
This helps the user to change the current working directory. A user can change directory to any existing directory dir. dir can be absolute or relative path.

ls
--

.. code:: shell

  ls [dir]
  
List the contents of dir or cwd if dir is not given. A user can do ls only if it is the owner of the dir or is a member of the group of the dir. dir can be absolute or realtive path.
  
  
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
  
Get the contents of file. A user can read the file only if it is atleast the owner of file or belongs to the group of the file.
  

fput
----

.. code:: shell

  fput <file>
  
Create file or append to file if it is already created. A user can create file only if it is the owner of the parent dir. If the file is already created then append mode is started. A user can append to file only if it is the owner of the file. file can be absolute or realtive path.
If it is a new file, user is then prompted for the **owner** and **group** of the file. If nothing passed, default grp and owner associations are inherited from parent dir.
User is finally prompted for the input text to append to file. User can finish appending to file by typing **end** in a newline.


Project Dir Structure
=====================

The server's directory is maintained as shown.

server/
    etc/
        passwd
    slash/
        home/
          u1/
          u2/
          .
          .
          .
    makefile
    client
    server
    client_16103.c
    server_16103.c
    

/etc/passwd stores the user and group associations
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

- root is the owner and group of **home** directory
- the group and user associations are stored in **server/etc/passwd**. The username and groups are hardwired from this file. Each line of the file contains entry for a user. Names are separated by " " (single blank space), where first name is the username and subsequent names in the line are groups of the user
- **/home/ui** directory has ui itself as the owner and group
- a user can be in maximum 10 groups
- set of group names are same as set of user names
- cd behind /server/slash/home is not allowed


Bugs defended
=============

- multiple sessions for a user is not allowed
- paths entered as arguments are validated
- server and client may exit abruptly and this is gracefully handled on both sides
- wrong commands, arguments or inputs are gracefully handled
- users entering wrong credential are not allowed to connect
- only limited number of users can connect at a time



Developed by Sushant Kumar Singh
