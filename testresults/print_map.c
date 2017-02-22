#define _GNU_SOURCE
#include <sys/sysinfo.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <asm/prctl.h>
#include <sys/prctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/resource.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/types.h>
#define CACHE_LINE_SIZE 64
static int opt_random_access=1;
///////////////////////////////////////////////////////////////
// Simple, fast random number generator, here so we can observe it using profiler
long x = 1, y = 4, z = 7, w = 13;

long simplerand(void) {
	long t = x;
	t ^= t << 11;
	t ^= t >> 8;
	x = y;
	y = z;
	z = w;
	w ^= w >> 19;
	w ^= t;
	return w;
}

// p points to a region that is 1GB (ideally)
void do_mem_access(char* p, int size) {
        int i, j, count, outer, locality;
   int ws_base = 0;
   int max_base = ((size / CACHE_LINE_SIZE) - 512);
        for(outer = 0; outer < (1<<20); ++outer) {
      long r = simplerand() % max_base;
      // Pick a starting offset
      if( opt_random_access ) {
         ws_base = r;
      } else {
         ws_base += 512;
         if( ws_base >= max_base ) {
            ws_base = 0;
         }
      }
      for(locality = 0; locality < 16; locality++) {
         volatile char *a;
         char c;
         for(i = 0; i < 512; i++) {
            // Working set of 512 cache lines, 32KB
            a = p + ws_base * CACHE_LINE_SIZE + i * CACHE_LINE_SIZE;
            if((i%8) == 0) {
               *a = 1;
            } else {
               c = *a;
            }
         }
      }
   }
}

long get_mem_size(){
  struct sysinfo info;
  int syserr = sysinfo(&info);
  if (syserr){
    printf("Error calling sysinfo\n");
  }
  return (long) info.totalram;
}

int compete_for_memory(void* unused) {
   long mem_size = get_mem_size();
   int page_sz = sysconf(_SC_PAGE_SIZE);
   printf("Total memsize is %3.2f GBs\n", (double)mem_size/(1024*1024*1024));
   fflush(stdout);
   char* p = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                  MAP_NORESERVE|MAP_PRIVATE|MAP_ANONYMOUS, -1, (off_t) 0);
   if (p == MAP_FAILED){
      perror("Failed anon MMAP competition\n");
   }
   int i = 0;
   while(1) {
      volatile char *a;
      long r = simplerand() % (mem_size/page_sz);
      char c;
      if( i >= mem_size/page_sz ) {
         i = 0;
      }
      // One read and write per page
      //a = p + i * page_sz; // sequential access
      a = p + r * page_sz;
      c += *a;
      if((i%8) == 0) {
         *a = 1;
      }
      i++;
   }
   return 0;
}


static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
  {
  int ret;
  ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                group_fd, flags);
  return ret;
}

int arch_prctl(int code, unsigned long addr){
  return syscall(SYS_arch_prctl, code, addr);
}

int main(int argc, char* argv[])
{
  printf("--------------------------------------------\n"); 
  int anon, map_shared, map_populate, mset, msink;
  sscanf(argv[1], "%d", &anon);
  sscanf(argv[2], "%d", &map_shared);
  sscanf(argv[3], "%d", &map_populate);
  sscanf(argv[4], "%d", &mset);
  sscanf(argv[5], "%d", &msink);
  sscanf(argv[6], "%d", &opt_random_access);
  
  // read fs and gs registers
  unsigned long fs, gs;
  int err;
  err = arch_prctl(ARCH_GET_FS, (unsigned long) &fs);
  if (err){
    printf("ERROR ON GET_FS\n");
  }
  err = arch_prctl(ARCH_GET_GS, (unsigned long) &gs);
  if (err){
    printf("ERROR ON GET_GS\n");
  }
  //printf("arch_prctl GET_FS: %lx\n", fs);
  //printf("arch_prctl GET_GS: %lx\n", gs);
  // view memory footprint
  char c;
  FILE *fptr;
  fptr = fopen("/proc/self/maps", "r");
  if (fptr) {
    while ((c = getc(fptr)) != EOF)
    //    putchar(c);
    fclose(fptr);
  }  

  // perf event stuff
  // create buffer
  int one_gb = 1073741824;
  char* buff;
  int fd;
  if (anon){
    fd = -1;
  } else {
    fd = open("one_gb.txt", O_RDWR);
  }
  int flag;
  if (map_shared){
    flag = MAP_SHARED;
  } else {
    flag = MAP_PRIVATE;
  }
  if (anon){
    flag = flag | MAP_ANONYMOUS;
  }
  if (map_populate){
    flag = flag | MAP_POPULATE;
  }
  buff = (char *) mmap(NULL, one_gb, PROT_READ|PROT_WRITE, flag, fd, 0);
  if ((long unsigned)buff==-1){
    printf("Error using mmap: %d\n", errno);
    printf("%s\n", strerror(errno));
  }
  if (mset){
    memset(buff, 0, one_gb);
  }
  if (msink){
    int err = msync(buff, one_gb, MS_SYNC);
    if (err){
      printf("Error using msync: %d\n", errno);
    }
  }
  // set up perf event measurement objects
  struct perf_event_attr pe;
  /* 1,2,3 are l1d access on read, write, and prefetch
     4,5,6 were lid miss on read, write, prefetch
     7,8,9 were dtlb miss on read, write, prefetch - some counters unsupported 
  */
  int fd1, fd3, fd4, fd6, fd7;
  long long val1, val3, val4, val6, val7;
  // set up perf_event_attr struct
  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_HW_CACHE;
  pe.size = sizeof(struct perf_event_attr);
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  // 1st event
  pe.config = PERF_COUNT_HW_CACHE_L1D | 
              (PERF_COUNT_HW_CACHE_OP_READ << 8) | 
              (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
  fd1 = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd1 == -1){
    fprintf(stderr, "Error opening fd1 %llx\n", pe.config);
  }
  // 3rd event
  pe.config = PERF_COUNT_HW_CACHE_L1D |
              (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) | 
              (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16); 
  fd3 = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd3 == -1){
    fprintf(stderr, "Error opening fd3 %llx\n", pe.config);
  }
  // 4th event
  pe.config = PERF_COUNT_HW_CACHE_L1D |
              (PERF_COUNT_HW_CACHE_OP_READ << 8) |
              (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
  fd4 = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd4 == -1){
    fprintf(stderr, "Error opening fd4 %llx\n", pe.config);
  }
  // 6th event
  pe.config = PERF_COUNT_HW_CACHE_L1D |
              (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
              (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
  fd6 = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd6 == -1){
    fprintf(stderr, "Error opening fd6 %llx\n", pe.config);
  }
  // 7th event
  pe.config = PERF_COUNT_HW_CACHE_DTLB |
              (PERF_COUNT_HW_CACHE_OP_READ << 8) |
              (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
  fd7 = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd7 == -1){
    fprintf(stderr, "Error opening fd7 %llx\n", pe.config);
  }
  // clean cache (64KB x 3 set associative)
  //printf("Clearing cache\n");
  char* wasted_space;
  wasted_space = (char *) malloc(64*1000*3);
  char x;
  for(int i=0; i<64*1000*3; i++){
    x = wasted_space[i];
  }
  pid_t childPID;
  //printf("bout ta fork\n");
  fflush(stdout);
  childPID = fork();
  //printf("forked\n");
  fflush(stdout);
  if (childPID == -1){
    printf("Fork failed, quitting!!!!!!\n");
    return 1;
  }
  int cpu_num = 0;
  if (childPID != 0){
    cpu_num = 1;
  }
  // set cpu affinity
  //printf("Setting cpu affinity\n");
  cpu_set_t mask;
  CPU_ZERO(&mask);  // no cpus
  CPU_SET(cpu_num, &mask);  // add cpu zero
  int result = sched_setaffinity(0, sizeof(mask), &mask);  // apply mask
  if (result){
    printf("Error setting affinity code %d\n", result);
  }
  // if child, just go be difficult, parent will kill
  if (childPID==0){
    compete_for_memory(buff);
    // child never gets here bc of infinite loop + kill
  }
  printf("childPID:%d\n", childPID);
  fflush(stdout);
  // start counters
  
  // start timers for rusage call later  
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);

  //printf("starting perf counters\n");
  ioctl(fd1, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd3, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd4, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd6, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd7, PERF_EVENT_IOC_RESET, 0);

  ioctl(fd1, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fd3, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fd4, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fd6, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(fd7, PERF_EVENT_IOC_ENABLE, 0);

  // instrumented code
  do_mem_access(buff, one_gb);
  // ends here
  ioctl(fd1, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fd3, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fd4, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fd6, PERF_EVENT_IOC_DISABLE, 0);
  ioctl(fd7, PERF_EVENT_IOC_DISABLE, 0);

  //printf("ending instrumented stuff\n");
  read(fd1, &val1, sizeof(long long));
  read(fd3, &val3, sizeof(long long));
  read(fd4, &val4, sizeof(long long));
  read(fd6, &val6, sizeof(long long));
  read(fd7, &val7, sizeof(long long));
  
  printf("L1 Data Cache Read Accesses: %lld\n", val1);
  //printf("L1 Data Cache Write Accesses: %lld\n", val2);
  //printf("L1 Data Cache Prefetch Accesses: %lld\n", val3);
  printf("L1 Data Cache Read Misses: %lld\n", val4);
  //printf("L1 Data Cache Prefetch Misses: %lld\n", val6);
  printf("DTLB Read Misses: %lld\n\n", val7);

  // print resource usage
  int rus_exit_code = getrusage(RUSAGE_SELF, &usage);
  if (rus_exit_code) printf("GETRUSAGE ERROR, EXIT CODE %d\n", rus_exit_code);
  printf("User CPU Seconds: %ld\n", usage.ru_utime.tv_sec);
  printf("User CPU uSeconds: %ld\n", usage.ru_utime.tv_usec);
  printf("System CPU Seconds: %ld\n", usage.ru_stime.tv_sec);
  printf("System CPU uSeconds: %ld\n", usage.ru_stime.tv_usec);
  //printf("Max Resident Set Size: %ld\n", usage.ru_maxrss);
  //printf("Integral Shared Mem Size: %ld\n", usage.ru_ixrss);
  //printf("Integral Unshared Mem Size: %ld\n", usage.ru_idrss);
  //printf("Integral Unshared Stack Size: %ld\n", usage.ru_isrss);
  printf("Page Reclaims (Soft Page Faults): %ld\n", usage.ru_minflt);
  printf("Hard Page Faults: %ld\n", usage.ru_majflt);
  printf("Swaps: %ld\n", usage.ru_nswap);
  //printf("Block Input Ops: %ld\n", usage.ru_inblock);
  //printf("Block Output Ops: %ld\n", usage.ru_oublock);
  //printf("IPC Msgs Sent: %ld\n", usage.ru_msgsnd);
  //printf("IPC Msgs Rcvd: %ld\n", usage.ru_msgrcv);
  //printf("Signals Rcvd: %ld\n", usage.ru_nsignals);
  printf("Voluntary Context Switches: %ld\n", usage.ru_nvcsw);
  printf("Involuntary Context Switches: %ld\n", usage.ru_nivcsw);
  kill(childPID, SIGKILL);
  err = arch_prctl(ARCH_GET_FS, (unsigned long) &fs);
  if (err){
    printf("ERROR ON GET_FS\n");
  }
  err = arch_prctl(ARCH_GET_GS, (unsigned long) &gs);
  if (err){
    printf("ERROR ON GET_GS\n");
  }
  //printf("arch_prctl GET_FS: %lx\n", fs);
  //printf("arch_prctl GET_GS: %lx\n", gs);
  if (munmap(buff, one_gb) == -1) {
    perror("Error un-mmapping the file");
  }
  if (!anon){
    close(fd);
  }
  printf("---------------------------------------------\n");
}
