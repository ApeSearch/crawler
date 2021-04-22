while true
do
    ip=$(curl -s icanhazip.com)
    echo $ip
    ./tests/bin/crawler $ip
done