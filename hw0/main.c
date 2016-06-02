#include <stdio.h>
#include <sys/resource.h>

int main()
{
    struct rlimit pro; 
	struct rlimit stack;
	struct rlimit filesize;
	getrlimit(RLIMIT_STACK,&stack);
	getrlimit(RLIMIT_NPROC,&pro);
	getrlimit(RLIMIT_NOFILE,&filesize);
    printf("stack size: %ld\n", stack.rlim_cur);
    printf("process limit: %ld\n", pro.rlim_cur);
    printf("max file descriptors: %ld\n", filesize.rlim_cur);
    return 0;
}
