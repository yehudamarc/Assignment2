typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
// Struct for signal handler
struct sigaction {
	void (*sa_handler) (int);
	uint sigmask;
};
