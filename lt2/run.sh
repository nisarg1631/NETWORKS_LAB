rm -f client server
gcc 19CS10070_client.c -o client
gcc 19CS10070_server.c -fsanitize=thread -o server -lpthread