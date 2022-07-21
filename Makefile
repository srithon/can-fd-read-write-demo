CC=gcc

all: reader writer

reader: reader_writer.c
	$(CC) -DRUN_READER reader_writer.c -o reader

writer: reader_writer.c
	$(CC) -DRUN_WRITER reader_writer.c -o writer

