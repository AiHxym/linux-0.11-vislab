

#include <stdio.h>
#include <unistd.h>


int main()
{

  


    pid_t fpid;
 	int i;
 	int *q;
 
    fpid = fork();
   
    for(i=1;i<100;i++)
  	
  	{
  		fork();
  	}
  	if (fpid>0)
  	{
  		printf("son\n");

  		q = malloc(sizeof(int)*10000);
  	}

	printf("over\n");
}


