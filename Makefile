##############################
#
# Libfindf library's Makefile
#
# V1.0.0
#
##############################

# GCC compile flags
GNUCC = gcc
GNUCFLAGS = -O2 -g -std=gnu99 -Wall -Wextra -Wpedantic \
            -Wpointer-arith -Wstrict-prototypes -DQUIET_OPENDIR # when undef produces loads of output.
GNULDFLAGS = -lpthread

# Other compilers will go here


##############################
${CC} = ${GNUCC}                 # Your compiler.
CFLAGS = ${GNUCFLAGS}            # Your compiler's compile flags.
LDFLAGS = ${GNULDFLAGS}          # Your linker's flags.

OBJECTS = libfindf_init.o libfindf_utils.o libfindf_main.o libfindf_sort.o \
          libfindf_Putils.o libfindf_std.o libfindf_adv.o libfindf_fg.o \
          libfindf_regex.o libfindf_re.o

libname = libfindf.so.1.0.0

${libname} : ${OBJECTS} libfindf_private.h findf.h
	${CC} ${CFLAGS} -shared ${OBJECTS} -o ${libname} -Wl,--version-script=${PWD}/libfindf_export.map ${LDFLAGS}

libfindf_init.o : libfindf_init.c libfindf_private.h findf.h
	${CC} ${CFLAGS} -fPIC -c libfindf_init.c ${LDFLAGS}

libfindf_utils.o : libfindf_utils.c libfindf_private.h findf.h
	${CC} ${CFLAGS} -fPIC -c libfindf_utils.c ${LDFLAGS}

libfindf_main.o : libfindf_main.c libfindf_private.h findf.h
	${CC} ${CFLAGS} -fPIC -c libfindf_main.c ${LDFLAGS}

libfindf_sort.o : libfindf_sort.c libfindf_private.h findf.h
	${CC} ${CFLAGS} -fPIC -c libfindf_sort.c ${LDFLAGS}

libfindf_regex.o : libfindf_regex.c libfindf_private.h findf.h
	${CC} ${CFLAGS} -fPIC -c libfindf_regex.c ${LDFLAGS}

libfindf_Putils.o : libfindf_Putils.c libfindf_private.h findf.h
	${CC} ${CFLAGS} -fPIC -c libfindf_Putils.c ${LDFLAGS}

libfindf_std.o : libfindf_std.c libfindf_private.h findf.h
	${CC} ${CFLAGS} -fPIC -c libfindf_std.c ${LDFLAGS}

libfindf_adv.o : libfindf_adv.c libfindf_private.h findf.h
	${CC} ${CFLAGS} -fPIC -c libfindf_adv.c ${LDFLAGS}

libfindf_fg.o : libfindf_fg.c libfindf_private.h findf.h
	${CC} ${CFLAGS} -fPIC -c libfindf_fg.c ${LDFLAGS}

libfindf_re.o : libfindf_re.c libfindf_private.h findf.h
	${CC} ${CFLAGS} -fPIC -c libfindf_re.c ${LDFLAGS}


.PHONY : clean
clean :
	rm -f *.o ${libname}
