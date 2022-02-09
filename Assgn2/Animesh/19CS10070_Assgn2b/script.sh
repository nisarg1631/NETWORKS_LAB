gcc dnsclient.c -o dnsclient;
gcc new_dnsclient.c -o new_dnsclient;
gcc new_dnsserver.c -o new_dnsserver;
if [[ "$1" == "clean" ]]; then
    rm dnsclient new_dnsserver new_dnsclient;
fi