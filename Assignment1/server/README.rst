
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

  sudo make
  
 

Step 2.
Initialise the default permission for server directory

.. code:: shell

  sudo sh init.sh
  
 

Step 3.
Supported commands can be run from server directory in following manner

.. code:: shell

  slash/bin/ls slash/home
  
 

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
  
Get the contents of file. A user can read the file only if it has read access for the file. Also verify the HMAC if present.

fput
----

.. code:: shell

  fput <file>
  
Create file or append to file if it is already created. A user can create file only if it has write access of the parent dir. If the file is already created then append mode is started. A user can append to file only if it is the owner of the file. file can be absolute or realtive path. User can finish appending to file by typing **end** in a newline. Also create a file.sign storing HMAC of file.

  

do_exec
-------

.. code:: shell

  do_exec <file>
  
Run the file with permissions of the owner.


fput_encrypt
------------

.. code:: shell

  fput_encrypt <file>
  
Put encrypted content to file. It uses owner's password from shadow file to generate key and iv using SHA1 digest.It uses AES_256 encryption algorithm. Only the owner can append to a encrypted file.



fget_decrypt
------------

.. code:: shell

  fget_decrypt <file>
  
Get encrypted content from file in plaintext. It uses owner's password from shadow file to generate key and iv using SHA1 digest and decrypts the encrypted content. Only the owner can decrypt from a encrypted file, others would get error. If a file is already in plaintext then it is not decrypted.



fsign
------

.. code:: shell

  fsign <file>
  
Create HMAC of file and store in file.sign. It uses SHA1 digest. Anyone with read access for file and write access in corresponding directory can sign a file. Key and IV of derived from owner's password are used.



fverify
---------

.. code:: shell

  fverify <file>
  
Verify HMAC of the file by comparing from file.sign. If corresponding .sign does not exist then show error. Key and IV of derived from owner's password are used.


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


Assumptions
============

- root is the owner and group of **slash, home** directory
- the group and user associations are stored in **server/etc/passwd**. The username and groups are hardwired from this file. Each line of the file contains entry for a user. Names are separated by " " (single blank space), where first name is the username and subsequent names in the line are groups of the user
- **/home/ui** directory has ui itself as the owner and group
- a user can be in maximum 10 groups
- set of group names are same as set of user names
- only absolute paths can be entered
- commands are run with base directory as **server/**
- initially there are 3 users: user1, user2, user3 and fakeroot with password same as username
- fget outputs the result even if the HMAC validation fails but it reports that validation failed


Bugs and Errors defended
=============

- paths entered as arguments are validated
- wrong arguments or inputs for acls, etc. are gracefully handled
- making acl entry for non existent users is not allowed
- access behind slash is not allowed
- handling overwriting of directory that is already created
-graceful handling for decrypting corrupted file
-fput_encrypt call on encrypted file and on plaintext file is handled
-fget_decrypt call on encrypted file and on plaintext file is handled


Developed by Sushant Kumar Singh
