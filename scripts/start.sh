while true
do
    ip=$(curl -s icanhazip.com)
    echo $ip
    ./bin/crawler $ip
done
