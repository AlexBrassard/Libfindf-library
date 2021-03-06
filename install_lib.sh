#!/bin/bash

#
#
#
#  Libfindf.so  -  Library and man-page installation script.
#  v2.0.0
#
#
# Usage:
#  Run the script with no arguments to install the library only.
#  Run the script with [-m] to install the library's man-pages only.
#  Run the script with [-wm] to install both the library and the man-pages.
#
#

# Adjust these to your own system needs.
header_dir="/usr/include/"
lib_dir="/usr/lib/"
man_dir="/usr/local/share/man/man3/"



# Unless you're maintaining/developping Libfindf, don't change these.
header_perm=644       # rw-r--r--
header_name="findf.h"               
lib_name="libfindf.so.1.0.0"
lib_link_name="libfindf.so"
install_dir=`echo $PWD/`
ARG1=$1




function install_man() {
    cd $PWD/man_pages_src 
    rm -f *.gz
    # Compress the man-pages.
    for mfile in $PWD/*
    do
	if [ -a $mfile ]
	then
	    gzip -c $mfile > "$mfile.3.gz" # Hardcode ".3" suffix for now.
	fi
    done
    
    # Check if man_dir exists, if not create it.
    if [ ! -d $man_dir ]
    then
	sudo mkdir $man_dir
    fi

    sudo cp *.3.gz $man_dir
    cd $man_dir
    # Update the database.
    sudo mandb
    cd $install_dir
}

if [ $ARG1 == "-h" ]
then
    printf "Usage: install_lib.sh [-h|-m|-wm]\n\n\tRun with no argument to install the library only.\n"
    printf "\t-h \tThis message.\n"
    printf "\t-m \tInstall man-pages only.\n"
    printf "\t-wm\tInstall both the library and the man-pages.\n\n"
    exit
fi
if [ $ARG1 == "-m" ]
then
    install_man
    exit
fi

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
fi
if [ -a "$lib_dir$lib_link_name" ]
then
    sudo rm "$lib_dir$lib_link_name"
fi
cd $lib_dir
sudo ldconfig

# Copy the new library to its destination.
sudo cp "$install_dir/$lib_name" $lib_dir
sudo ln -s $lib_name $lib_link_name
sudo ldconfig -l $lib_name

if [ $ARG1 == "-wm" ]
then
    install_man
fi

exit 0
