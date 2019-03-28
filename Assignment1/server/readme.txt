commands used to add the users

sudo useradd -m -d /home/sushant/Desktop/nss/Assignment1/server/slash/home/user1 user1
sudo passwd user1

sudo useradd -m -d /home/sushant/Desktop/nss/Assignment1/server/slash/home/user2 user2
sudo passwd user2

sudo useradd -m -d /home/sushant/Desktop/nss/Assignment1/server/slash/home/user3 user3
sudo passwd user3

sudo useradd fakeroot
sudo passwd fakeroot

sudo usermod -aG sudo fakeroot

// commands to test
slash/bin/fput_encrypt slash/home/user1/try2.txt 
slash/bin/fget_decrypt slash/home/user1/try2.txt


slash/bin/fget slash/home/user1/try2.txt
slash/bin/fput slash/home/user1/try2.txt

syscalls:
getpwuid
getgrgid
getpwuid
readdir
getuid
getgid
geteuid
getegid
seteuid
stat
