#!bin/sh

echo "Enter Slave1 IP:"
read slave1

echo "Enter Slave2 IP:"
read slave2

echo "Enter the Server IP:"
read server

echo "Master started...."

./master $slave1 $slave2 $server
