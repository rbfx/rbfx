apt-get update -y
apt-get install wget -y

wget https://packages.microsoft.com/config/ubuntu/20.04/packages-microsoft-prod.deb
dpkg -i packages-microsoft-prod.deb
apt-get install apt-transport-https -y
apt-get update -y


apt-get install dotnet-sdk-6.0 make software-properties-common -y
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
./llvm.sh 14
ln -s /usr/bin/clang-14 /usr/bin/clang
ln -s /usr/bin/clang++-14 /usr/bin/clang++
# add-apt-repository ppa:deadsnakes/ppa -y
# apt install python3.8 -y

apt-get autoremove -y
apt-get clean -y
