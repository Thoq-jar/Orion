# Orion

Orion is a fast file search engine that can search any nook and cranny of your computers drive.

**Run this to build and run Orion:**
```bash
clear
cargo build --verbose
clear
cd target/debug
sudo ./Orion --verbose
cd ..
cd ..
```

[Thoq-License](https://raw.githubusercontent.com/Thoq-jar/Thoq-License/main/License)

# Instalaltion
```bash
cd ~
mkdir .otemp
cd .otemp
rm -rf Orion
git clone https://github.com/Thoq-jar/Orion.git
cd Orion
cargo build --verbose
cd target/debug
sudo mv Orion /usr/local/bin/
cd ~
rm -rf .otemp
```
