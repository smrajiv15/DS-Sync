#!bin/sh

echo "Enter the Server IP:"
read server

echo "Slave2 started ..."

./slave2 $server
