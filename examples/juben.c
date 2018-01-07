
#include <stdio.h>
#include <unistd.h>


int main()
{

    int* qq = malloc(sizeof(int)*100);


    pid_t fpid;

    int count = 0 ;
    fpid = fork();

    if(fpid<0)
    {
    	printf("error in fork\n");
    }

    if (fpid==0)
    {
    	printf("i am \n");
    	qq[10] = 55;
    }
    if (fpid>0)
    {
    	qq[11] = 77;
    	while(1)
    	{
    	 	int * zhaoyue = malloc(sizeof(int)*100000);
		}
    }
  
	printf("over\n");
}


