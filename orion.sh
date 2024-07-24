#!/bin/bash

clear
cargo build --verbose
clear
cd target/debug
sudo ./Orion --verbose
cd ..
cd ..
