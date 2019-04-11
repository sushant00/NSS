
Multi User IRC chat Client and Server
**********

**This is a IRC chat app running on Multi-threaded TCP server.**


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
Authenticate the user by typing in username (password not required)

Now the user is presented with a shell. Supported commands are mentioned below.

Commands Supported
==================
who
---

.. code:: shell

  who
  
Shows name and uid of logged in users. Groups are not shown.


write_all
---------

.. code:: shell

  write_all <msg>
  
Broadcasts msg to everyone, including the sender. The broadcasting is encrypted using the established shared key


Project Dir Structure
=====================

The server's directory is maintained as shown.

server/
    makefile
    client
    server
    client_16103.c
    server_16103.c
    


Security Rules
==============

- a user is given write access to a dir or file only if it is the owner
- a user is given read access to a dir or file only if is is the owner or is member of the group of corresponding file or dir
- by default users are denied connection or any access if not authenticated
- a user can be in multiple groups
- a file or dir can have only one owner and only one group



Assumptions
============

- The IP address and port number of KDC, server is fixed
- the clients need to know and enter their 4 digit userid
- a 6 digit nonce is generated automatically and added to the msg
- 00 uid is reserved for chat server
- the files listed or shared should be in users home directory only
- the iv is generated from the key
- max password length for a user is 512 characters
- client currently reads the password from shadow file, may be changed to enter the password

Bugs defended / Extra Features
==============================

- multiple sessions for a user is not allowed
- server and client may exit abruptly and this is gracefully handled on both sides
- wrong commands, arguments or inputs are gracefully handled
- users entering wrong credential are not allowed to connect
- only limited number of users can connect at a time
- server can inform its exit to client
- client checks the nonce recieved, and userid of chat server as in NS protocol


Developed by Sushant Kumar Singh
