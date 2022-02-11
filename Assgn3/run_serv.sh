if [[ "$1" == "clean" ]]; then
    rm client server;
    else
    gcc ftpS.c -o server;
    ./server;
fi