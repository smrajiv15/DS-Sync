#!bin/sh

echo "Enter the Server IP:"
read server

echo "Slave1 started ..."

./slave1 $server
