#ifndef IO_H
#define IO_H

// read 1/2 bytes 
unsigned char insb(unsigned short port);
unsigned short insw(unsigned short port);

// write 1/2 bytes
void outb(unsigned short port, unsigned char val);
void outw(unsigned short port, unsigned short val);


#endif // IO_H