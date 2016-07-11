

/* mountain.c - Generate the memory mountain. */
/* $begin mountainmain */
#include <stdlib.h>
#include <stdio.h>
#include "fcyc2.h" /* measurement routines */
#include "clock.h" /* routines to access the cycle counter */
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


#define MINBYTES (1 << 11)  /* Working set size ranges from 2 KB */
#define MAXBYTES (1 << 25)  /* ... up to 64 MB */
#define MAXSTRIDE 64        /* Strides range from 1 to 64 elems */
#define MAXELEMS MAXBYTES/sizeof(double)

/* $begin mountainfuns */
double data[MAXELEMS];      /* The global array we'll be traversing */

/* $mountainfuns */
void init_data(double *data, int n);
void test(int elems, int stride);
double run(int size, int stride, double Mhz);
int read_file_line(int line_no);
  
/* $begin mountainmain */
int main()
{
    int size;        /* Working set size (in bytes) */
    int stride;      /* Stride (in array elements) */
    double Mhz;      /* Clock frequency */
    
    init_data(data, MAXELEMS); /* Initialize each element in data */
    Mhz = mhz(0);              /* Estimate the clock frequency */
    
    /* Shown in the text */
    //printf("Clock frequency is approx. %.1f MHz\n", Mhz);
    //printf("Memory mountain (MB/sec)\n");
    
    //printf("\t");
    for (stride = 1; stride <= MAXSTRIDE; stride++)
      ;
      //printf("s%d\t", stride);
    //printf("\n");
    
    for (size = MAXBYTES; size >= MINBYTES; size >>= 1) {
        if (size > (1 << 20))
	  ;
	  //printf("%dm\t", size / (1 << 20));
        else
	  ;
	  //printf("%dk\t", size / 1024);

        for (stride = 1; stride <= MAXSTRIDE; stride++) {
	  //printf("%.1f\t", run(size, stride, Mhz));
	  double throughput = run(size, stride, Mhz);
	  printf("%d %d %.1f\n", size, stride, throughput);
            
        }
        //printf("\n");
    }
    exit(0);
}
/* $end mountainmain */

/* init_data - initializes the array */
void init_data(double *data, int n)
{
    int i;
    
    for (i = 0; i < n; i++)
        data[i] = i;
}

/* $begin mountainfuns */
/*
 * test - Iterate over first "elems" elements of array "data"
 *        with stride of "stride".
 */
void test(int elems, int stride) /* The test function */
{
    int i;
    double result = 0.0;
    volatile double sink;
    int mem = 1;
    
    for (i = 0; i < elems; i += stride) {
      /* if getting the data from memory*/
      if(mem == 1)
	{
	  result += data[i];
	  //printf("%d \t %d \n", i, ret);
	}
      /* else we are getting the data from the disk*/
      else
	{
	  int ret = read_file_line(i);
	  result += ret;
	}

    }
    sink = result; /* So compiler doesn't optimize away the loop */
}

/*
 * run - Run test(elems, stride) and return read throughput (MB/s).
 *       "size" is in bytes, "stride" is in array elements, and
 *       Mhz is CPU clock frequency in Mhz.
 */
double run(int size, int stride, double Mhz)
{
    double cycles;
    int elems = size / sizeof(double);
    
    test(elems, stride);                     /* warm up the cache */       //line:mem:warmup
    cycles = fcyc2(test, elems, stride, 0);  /* call test(elems,stride) */ //line:mem:fcyc
    return (size / stride) / (cycles / Mhz); /* convert cycles to MB/s */  //line:mem:bwcompute
}
/* $end mountainfuns */


int read_file_line(int line_no)
{
  line_no++; // counter 0 offset
  struct stat s;
  char *buf;
  off_t start = -1, end = -1;
  size_t i;
  int ln, fd, ret = 1;
  char out[100];

  if (line_no == 1) start = 0;
  else if (line_no < 1){
    warn("line_no too small");
    return 0; /* line_no starts at 1; less is error */
  }

  line_no--; /* back to zero based, easier */

  //reading fromt he file on disk
  fd = open("input.txt", O_RDONLY);
  fstat(fd, &s);

  /* Map the whole file.  If the file is huge (up to GBs), OS will swap
   * pages in and out, and because search for lines goes sequentially
   * and never accesses more than one page at a time, penalty is low.
   * If the file is HUGE, such that OS can't find an address space to map
   * it, we got a real problem.  In practice one would repeatedly map small
   * chunks, say 1MB at a time, and find the offsets of the line along the
   * way.  Although, if file is really so huge, the line itself can't be
   * guaranteed small enough to be "stored in memory", so there.
   */
  buf = mmap(0, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

  /* optional; if the file is large, tell OS to read ahead */
  madvise(buf, s.st_size, MADV_SEQUENTIAL);

  for (i = ln = 0; i < s.st_size && ln <= line_no; i++) {
    if (buf[i] != '\n') continue;

    if (++ln == line_no) start = i + 1;
    else if (ln == line_no + 1) end = i + 1;
  }

  if (start >= s.st_size || start < 0) {
    warn("file does not have line %d", line_no + 1);
    ret = 0;
  } else {
    // write(STDOUT_FILENO, buf + start, end - start);

    /* this is where I read from the disk */
    memset(out, '\0', sizeof(out));
    strncpy(out,  buf + start, end - start);
    
    //printf("after strncpy %d \n ", atoi(out));
    
  }

  munmap(buf, s.st_size);
  close(fd);

  //return ret;
  return atoi(out);
}
