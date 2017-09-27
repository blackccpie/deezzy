#!/bin/bash

# Build deezzy repository
cd deezzy
sh build_rpi_gcc6.sh
echo "RUNNING DEEZZY!"
./bin/deezzy

