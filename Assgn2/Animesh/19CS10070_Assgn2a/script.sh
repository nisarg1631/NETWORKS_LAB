gcc dnsclient.c -o dnsclient;
gcc dnsserver.c -o dnsserver;
if [[ "$1" == "clean" ]]; then
    rm dnsclient dnsserver;
fi