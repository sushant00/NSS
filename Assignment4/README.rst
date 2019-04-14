
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


Creating RSA Key and certificate
------------------------------
Following commands can be used to create public, private keys and a self signed certificate. These are stored in a directory 'rsa' inside home directory of each user

.. code:: shell

	sudo openssl req -x509 -newkey rsa:1024 -keyout user_private.pem -out user_cert.pem
	
	sudo openssl rsa -in user_private.pem -pubout -out user_public.pem



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
  
Broadcasts msg to everyone, excluding the sender. The broadcasting is encrypted using the established shared key


create_group
------------

.. code:: shell

  create_group [user1] [user2 ...]
  
Creates a group and adds specified users to it. Calling user is added by default. UserIDs are passed to add the corresponding users. A group id (same as group name) is returned to everyone added in a group. A user need not be online.


group_invite
------------

.. code:: shell

  group_invite <gid> <uid>
  
Sends an invite to user with user id uid, for coming in group gid. Sender must be in the group to send request to other user. Assumes uid user is online.


group_invite_accept
-------------------

.. code:: shell

  group_invite_accept <gid>
  
Accept an invite to grp gid. A user added only if he was invited earlier.


request_public_key
-------------------

.. code:: shell

  request_public_key <uid>
  
Sends the request for public key of user uid. A request can be any number of time, even if the user already has received the public key.


send_public_key
-------------------

.. code:: shell

  send_public_key <uid>
  
Sends the response public key to user uid. A response can be sent any number of time, even if the user already has sent the public key.


write_group
-------------------

.. code:: shell

  write_group <gid> <msg>
  
write encrypted msg to everyone in group gid. Assumes diffie hellman key exchange is done.


list_user_files
-------------------

.. code:: shell

  list_user_files <uid>
  
Lists the file in home directory of user uid, that have read access for calling user. The read access is checked from the actual linux permissions/ACLs. Does not lists folders or hidden files.


Project Dir Structure
=====================

The server's directory is maintained as shown.

server/
    makefile
    client
    server
    client_16103.c
    server_16103.c
    utils.c
    


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
- There can be at max 5 users in a group, and there can be at max 4 groups
- a user can be added to group only by an invitation or at the time of group creation
- group ids and group names are same
- Server has the public key of every client (in server/rsa directory)


Bugs defended / Extra Features
==============================

- multiple sessions for a user is not allowed
- a user cannot pass wrong uids to create_group
- same user cannot be added multiple times to the same group
- wrong commands, arguments or inputs are gracefully handled, e.g. group_invite is both required args are checked
- In group_invite_accept, it is checked if accepting was invited
- users entering wrong credential are not allowed to connect
- server and client may exit abruptly and this is gracefully handled on both sides
- only limited number of users can connect at a time
- client checks the nonce recieved, and userid of chat server as in NS protocol



Developed by Sushant Kumar Singh
