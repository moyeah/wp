SOURCES = wb_settings_dialog.c \
          wb_downloads_bar.c \
          wb_window.c \
          wb.c
OBJS    = ${SOURCES:.c=.o}
CFLAGS  = $(shell pkg-config webkit2gtk-3.0 --cflags)
LDADD   = $(shell pkg-config webkit2gtk-3.0 --libs)
CC      = gcc
DEBUG   = -g
PACKAGE = wb

all: ${OBJS}
	${CC} ${DEBUG} -o ${PACKAGE} ${OBJS} ${LDADD}

.c.o:
	${CC} ${DEBUG} ${CFLAGS} -c $<

clean:
	rm -f *~ *.o ${PACKAGE}
