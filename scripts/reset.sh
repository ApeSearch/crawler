rm bloomFilter.bin
rm anchorFiles/*
rm storageFiles/*
rm parsedFiles/*
if [ ! -d VirtualFileSystem/Root/Frontier ]; then
    mkdir -p VirtualFileSystem/Root/Frontier;
else
    rm VirtualFileSystem/Root/Frontier/*
fi
cat /dev/null > VirtualFileSystem/Root/pagesCrawledDONTTOUCH.txt
chmod -x scripts/reset.sh
echo "Please don't rerun again unless you want to remove everything crawled/parsed/indexed"
ulimit -n 2048
rm core
ulimit -c unlimited
