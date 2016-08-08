#!/bin/bash

# Careful not to install the library at multiple places by
# say installing it with default parameters, changing the parameters,
# and running the install script again without properly removing the first version.

# Note that there is no uninstall script at the moment.


# File extentions.
obj=".o"
src=".c"
hdr=".h"

# Source file(s) name(s).
FILES=( "libfindf_utils" "libfindf_init" "libfindf_main" "libfindf_Putils" "libfindf_std" "libfindf_adv" "libfindf_fg" "libfindf_sort" )

# Header file(s) name(s).
HFILES=( "findf.h" )

# Header file(s) permissions.
hperm=644

# Shared library name.
libnam="libfindf.so"

# Absolute path, where to install the library.
libdir="/usr/lib/"

# Absolute path, where to remove an existing verison of the library.
oldlib="$libdir$libnam"

# Absolute path, where the library's source is living.
newlib="$PWD/" 

# Absolute path, where the header file will be living.
includedir="/usr/include/"

# Absolute path, where the man-page(s) source is living.
mansrc="$PWD/man_pages_src/"

# Absolute path, CD there before running mandb.
man_install_parent="/usr/local/share/man/"

# Linux Manual section (3) man-page(s)
mp3_install="/usr/local/share/man/man3/"
MP3_NAMES=( "findf_destroy_param.3.gz" "findf_destroy_results.3.gz" "findf_adv.3.gz" "findf_fg.3.gz" "findf.3.gz" "findf_init_param.3.gz" "findf_read_results.3.gz" "SU_strcpy.3.gz" )

# Linux Manual section (7) man-page(s)
mp7_install="/usr/local/share/man/man7/"
MP7_NAMES=( "findf.7.gz" )

# Compiler flags (GCC).
# Careful, these flags are shared between every call to gcc.
# Input them as a string. (Empty string for no extra arguments.)

# Undef QUIET_OPENDIR only for debug purposes and even then, only if you
# really need to see every directory that gets open internaly.
CFLAGS=" -O2 -g -std=gnu99 -fvisibility=hidden -Wall -Wextra -Wpedantic -Wpointer-arith -Wstrict-prototypes -DQUIET_OPENDIR" 

# Libraries to pass to the linker while creating object files.
# Input them as a string (Empty string for no extra libraries.)

CLIBS="-lpthread"



# Remove existing .o files.
for file in ${FILES[@]}
do
    if [ -a "$file$obj" ]
    then
	rm "$file$obj"
	_mesg_="Removing $file$obj"
	printf "%-55s OK\n" "$_mesg_"
    else
	_mesg_="Removing $file$obj"
	printf "%-55s --\n" "$_mesg_"
    fi
done

# Remove the old header file
_mesg_="Removing header file "
for hfile in ${HFILES[@]}
do
    if [ -a "$includedir$hfile" ]
    then
	_mesg_="$_mesg_ $includedir$hfile"
	sudo rm -i "$includedir$hfile"
	if [ $? == 0 ]
	then
	    printf "%-55s OK\n" "$_mesg_"
	else
	    printf "%-55s --\n" "$_mesg_"
	    exit
	fi
    fi
done

_mesg_="Removing $libnam"
# Remove the old shared library in the current directory if it exists.
if [ -a "$libnam" ]
then
    rm "$libnam"
    printf "%-55s OK\n" "$_mesg_"
else
    printf "%-55s --\n" "$_mesg_"
fi

# Copy the header file(s) to the appropriate directory.
for hfile in ${HFILES[@]}
do
    _mesg_="Copy $newlib$hfile to $includedir"
    if [ -a "$includedir$hfile" ]
    then
	printf "%-55s --\n" "$_mesg_"
    else
	sudo cp "$newlib$hfile" "$includedir"
	if [ $? == 0 ]
	then
	    printf "%-55s OK\n" "$_mesg_"
	else
	    printf "%-55s --\n" "$_mesg_"
	    exit
	fi
	_mesg_="Chmod $includedir$hfile permissions to $hperm"
	sudo chmod "$hperm" "$includedir$hfile"
	if [ $? == 0 ]
	then
	    printf "%-55s OK\n" "$_mesg_"
	else
	    printf "%-55s --\n" "%_mesg_"
	    exit
	fi	
    fi
done

# Compile new position independent object files.
for sfile in ${FILES[@]}
do
    if [ -a "$newlib$sfile$src" ]
    then
	cmd=
	_mesg_="Creating $sfile$obj"
	# gcc "$CFLAGS" -fPIC -c "$newlib$sfile$src" "$CLIBS"
	# gcc "$CFLAGS -fPIC -c $newlib$sfile$src $CLIBS"
	gcc $CFLAGS -fPIC -c "$newlib$sfile$src" $CLIBS
	if [ $? == 0 ]
	then
	    printf "%-55s OK\n" "$_mesg_"
	else
	    printf "%-55s --\n" "$_mesg_"
	    exit
	fi
    fi
done


# Create the shared library.
_mesg_="Creating shared library"
cmd="$CFLAGS -shared -o $libnam "
for sfile in ${FILES[@]}
do
    if [ -a "$newlib$sfile$obj" ] 
    then
	cmd="$cmd $sfile$obj"
    fi
done
cmd="$cmd $CLIBS"
# gcc "$cmd"
gcc $cmd
if [ $? != 0 ]
then  
    printf "%-55s --\n" "$_mesg_"
    exit
else
    printf "%-55s OK\n" "$_mesg_"
fi

if [ "${ARGV[1]}" == "-m" ]
then
    # Remove the old man-page(s)
    # Section 3 pages
    # Check if directory exists first.
    if [ -d "$mp3_install" ]
    then
	_mesg_="$mp3_install exists"
	printf "%-55s\n" "$_mesg_"
    else
	sudo mkdir "$mp3_install"
	if [ $? == 0 ]
	then
	    _mesg_="Creating $mp3_install\n"
	    printf "%-55s OK\n" "$_mesg_"
	else
	    _mesg_="Creating $mp3_install\n"
	    printf "%-55s --\n" "$_mesg_"
	    exit
	fi	
    fi
    
    _mesg_="Removing man (3) page"
    for m3page in ${MP3_NAMES[@]}
    do
	new_mesg_="$_mesg_ $m3page"
	if [ -a "$mp3_install$m3page" ]
	then
	    sudo rm -i "$mp3_install$m3page"
	    if [ $? == 0 ]
	    then
		printf "%-55s OK\n" "$new_mesg_"
	    else
		printf "%-55s --\n" "$new_mesg_"
		exit
	    fi
	else
	    printf "%-55s --\n" "$new_mesg_"
	    
	fi
    done
    # Section 7 pages
    # Check if directory exists first.
    if [ -d "$mp7_install" ]
    then
	_mesg_="$mp7_install exists"
	printf "%-55s\n" "$_mesg_"
    else
	sudo mkdir "$mp7_install"
	if [ $? == 0 ]
	then
	    _mesg_=" Creating $mp7_install\n"
	    printf "%-55s OK\n" "$_mesg_"
	else
	    _mesg_="Creating $mp7_install\n"
	    printf "%-55s --\n" "$_mesg_"
	    exit
	fi	
    fi
    _mesg_="Removing man (7) page"
    for m7page in ${MP7_NAMES[@]}
    do
	new_mesg_="$_mesg_ $m7page"
	if [ -a "$mp7_install$m7page" ]
	then
	    sudo rm -i "$mp7_install$m7page"
	    if [ $? == 0 ]
	    then
		printf "%-55s OK\n" "$new_mesg_"
	    else
		printf "%-55s --\n" "$new_mesg_"
		exit
	    fi
	else
	    printf "%-55s --\n" "$new_mesg_"
	fi
    done
    
    # Copy new man-pages
    # Section 3 pages
    _mesg_="Copying new manual page"
    for m3page in ${MP3_NAMES[@]}
    do
	new_mesg_="$_mesg_ $m3page"
	sudo cp "$mansrc$m3page" "$mp3_install"
	if [ $? == 0 ]
	then
	    printf "%-55s OK\n" "$new_mesg_"
	else
	    printf "%-55s --\n" "$new_mesg_"
	    exit
	fi
    done
    
    # Section 7
    _mesg_="Copying new manual page"
    for m7page in ${MP7_NAMES[@]}
    do
	_mesg_="$_mesg_ $m7page"
	sudo cp "$mansrc$m7page" "$mp7_install"
	if [ $? == 0 ]
	then
	    printf "%-55s OK\n" "$_mesg_"
	else
	    printf "%-55s --\n" "$_mesg_"
	    exit
	fi
    done
    
fi

# Remove the old library file
if [ -a "$oldlib" ]
then
    _mesg_="Removing $oldlib"
    sudo rm -i "$oldlib"
    if [ $? == 0 ]
    then
	printf "%-55s OK\n" "$_mesg_"
    else
	printf "%-55s --\n" "$_mesg_"
	exit
    fi
    
    _mesg_="ldconfig [no_param]"
    sudo ldconfig
    if [ $? != 0 ]
    then
	printf"%-55s --\n" "$_mesg_"
	exit
    else
	printf "%-55s OK\n" "$_mesg_"
    fi

else
    
    printf "%-55s --\n" "$_mesg_"
    
fi

_mesg_="Copy the new $PWD/$libnam to /usr/lib"
# Copy the new library to /usr/lib/
sudo cp "$PWD/$libnam" "$libdir"
if [ $? != 0 ]
then
    printf "%-55s --\n" "$_mesg_"
    exit
else
    printf "%-55s OK\n" "$_mesg_"
    
fi

    

_mesg_="Change directory to $libdir"
# Change dir to /usr/lib and run ldconfig
cd "$libdir"
if [ $? == 0 ]
then
    printf "%-55s OK\n" "$_mesg_"
else
    printf "%-55s --\n" "$_mesg_"
    exit
fi


_mesg_="ldconfig [-l] [$libnam]"
# Run the linker to create dependencies for our new library:
sudo ldconfig -l "$libnam"
if [ $? != 0 ]
then
    printf "%-55s --\n" "$_mesg_"
    exit
else
    printf "%-55s OK\n" "$_mesg_"
fi

exit

cd "$man_install_parent"

# Update the Man database
_mesg_="Updating mandb"
sudo mandb
if [ $? == 0 ]
then
    printf "%-55s OK\n" "$_mesg_"
else
    printf "%-55s --\n" "$_mesg_"
    exit
fi
