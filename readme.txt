There are two programs.
index.cpp - This is the index server program.
peer.cpp - This is the peer (client-server) program.

Syntax to compile files

$make all 
This will compile the source code.

To run Index server
./index.o
There will be a message saying index server is running

For the peer program.
first run the program in registration mode
$./peer.o r [files folder] [server ip address] [client ip address]
example : ./peer.o r files/ 192.168.4.29 192.168.4.30

After running the peer in registration mode now you can run in peer mode
In peer mode you need to provide a file list which contains the files for download

$./peer.o b [filelist] [client ip address]

NOTE: Once the server thread begins to run it waits for user input to start downloading the files provided in filelist.
This was done so that once all the peer servers are started we can download the file from the client.

Or else it results in error because the file was not present in client.

Interactive Mode
$./peer.o i [client ip address]
