C TCP/IP Server and Client for File Transfer

Works inside of the Servers current directory in the ./files subdirectory -- Will create this directory if it doesn't exist

Client must login with a password which can be changed in the server.c by changing the predefined 'password' variable

Client Commands:
list- List off files in the servers ./files directory
put- Uploads a file from the clients current directory to the servers ./files directory, client can also use absolute file paths to reference files outside of working directory
get- Downloads a file from the servers ./files directory to the clients current directory, will notify client if file does not exist
q- Closes out client session but leaves server listener running
q!- Closes out client session and closes server listener
