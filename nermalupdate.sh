#!/bin/bash

####
### This script creates a set of blocklists from dat  available at blocklist.site.
### These individual blocklists can be used to build access policies for the proxy,
### specified in the nermalproxy config file
####

BLOCK_LISTS="ads crypto drugs fraud fakenews gambling malware phishing piracy porn proxy ransomware redirect scam spam torrent tracking facebook youtube"

SOURCE_URL="https://blocklist.site/app/dl"

for BLOCK_LIST in ${BLOCK_LISTS}; do
    echo "Getting list: ${BLOCK_LIST}"
    wget ${SOURCE_URL}/${BLOCK_LIST}
    mv ./${BLOCK_LIST} ./build/${BLOCK_LIST}.list
done

