#include <stdio.h>
#include <unistd.h>

int fac(int a)
{
	int *q;
	q = malloc(sizeof(int)*100);
	q[1] = 10;
	if (a ==1 || a==0)
		return 1;
	else{
		return fac(a-1)+fac(a-2);
	}
}



int main()
{

    int* qq = malloc(sizeof(int)*100);

    int *zhaoyue;
    pid_t fpid;

    int count = 0 ;
    fpid = fork();

    if(fpid<0)
    {
    	printf("error in fork\n");
    }

    if (fpid==0)
    {
    	printf("i am father\n");
    	qq[10] = 55;
    	fac(3);
    	printf("fac father done\n");
    }
    if (fpid>0)
    {
    	printf("i am son %d\n",getpid());

    	qq[11] = 77;
    	zhaoyue = malloc(sizeof(int)*1000);
    	zhaoyue[2] = 3;
    	fac(1000);
    }
  
	printf("over\n");
}

