#!/bin/bash

# Adjust these to your own system needs.
header_dir="/usr/include/"          # Change to your own needs.
lib_dir="/usr/lib/"                 # Change to your own needs.


# Unless you're maintaining/developping Libfindf, don't change these.
header_perm=644       # rw-r--r--
header_name="findf.h"               
lib_name="libfindf.so.1.0.0"
lib_link_name="libfindf.so"


# Send a copy of the public header file to its destination.
if [ -a "$header_dir$header_name" ]
then
    sudo rm "$header_dir$header_name"
fi
sudo cp $header_name $header_dir

# Remove old files if they exists.
make clean

# Create the library.
make

# Remove the old library if it exists.
if [ -a "$lib_dir$lib_name" ]
then
    sudo rm "$lib_dir$lib_name" "$lib_dir$lib_link_name"
    sudo ldconfig
fi

# Copy the new library to its destination.
sudo cp "$PWD/$lib_name" $lib_dir
cd $lib_dir
sudo ln -s $lib_name $lib_link_name
sudo ldconfig -l $lib_name
