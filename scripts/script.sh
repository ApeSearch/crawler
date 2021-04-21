#!/bin/bash

cp /dev/null output.txt
for number in {1..100}
do
    echo "$number "
    ./tests/bin/test_Frontier
done
exit