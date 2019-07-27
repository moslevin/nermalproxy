#!/bin/bash

BLOCK_LISTS="ads crypto drugs fraud fakenews gambling malware phishing piracy porn proxy ransomware redirect scam spam torrent tracking facebook youtube"

SOURCE_URL="https://blocklist.site/app/dl"

for BLOCK_LIST in ${BLOCK_LISTS}; do
    echo "Getting list: ${BLOCK_LIST}"
    wget ${SOURCE_URL}/${BLOCK_LIST}
    mv ./${BLOCK_LIST} ./build/${BLOCK_LIST}.list
done

