if [[ "$1" == "clean" ]]; then
    rm server client;
    else
    gcc ftpC.c -o client;
    ./client;
fi