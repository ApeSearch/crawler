rm bloomFilter.bin
rm anchorFiles/*
rm storageFiles/*
rm parsedFiles/*
rm VirtualFileSystem/Root/Frontier/*
cp VirtualFileSystem/Root/pagesCrawledDONTTOUCH.txt /dev/null
chmod -x scripts/reset.sh
echo "Please don't rerun again unless you want to remove everything crawled/parsed/indexed"
rm core
