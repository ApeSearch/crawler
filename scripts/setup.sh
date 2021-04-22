# Initialize for libraries
git submodule update --remote --init
# Initialize for Parser
cd Parser
git submodule update --remote --init
cd ../
echo "./tests/bin/crawler $1" > ./scripts/start.sh
chmod +x ./scripts/start.sh
# Setup make, g++
sudo apt-get install build-essential
sudo apt-get update && sudo apt-get upgrade
sudo apt install build-essential checkinstall zlib1g-dev -y
sudo apt-get install libssl-dev
sudo apt-get install libz-dev
sudo apt autoremove
chmod +x ./scripts/reset.sh
./scripts/reset.sh
make clean; make -j tests/crawler
