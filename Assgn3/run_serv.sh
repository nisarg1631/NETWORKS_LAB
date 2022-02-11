if [[ "$1" == "clean" ]]; then
    rm client server;
    else
    gcc ftpC.c -o server;
    ./server;
fi