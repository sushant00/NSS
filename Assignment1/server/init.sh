#!/bin/bash

chown -R fakeroot:fakeroot slash/bin
chmod -R 766 slash/bin
chmod -R u+s slash/bin

chown root:root slash
chown root:root slash/home
chown root:root slash/bin
chown root:root slash/etc

# sudo slash/bin/setacl -m u::rwx slash/home/user1
# sudo slash/bin/setacl -m g::r-x slash/home/user1
# sudo slash/bin/setacl -m o::r-x slash/home/user1

# sudo slash/bin/setacl -m u::rwx slash/home/user2
# sudo slash/bin/setacl -m g::r-x slash/home/user2
# sudo slash/bin/setacl -m o::r-x slash/home/user2

# sudo slash/bin/setacl -m u::rwx slash/home/user3
# sudo slash/bin/setacl -m g::r-x slash/home/user3
# sudo slash/bin/setacl -m o::r-x slash/home/user3
