

#include <stdio.h>
#include <unistd.h>


int main()
{

  


    pid_t fpid;
    int i;
 	
    int* q ;
    q = malloc(sizeof(int)*10000);
    fpid = fork();
   
    for(i=1;i<10;i++)
  	
  	{
  		fpid = fork();
      q[1] = fpid;
      printf("afeafea\n");
  	}
    
    while(1)
    {
      i++;
      if(i%1000000000==0)
      {
        printf("%d\n",q[1] );
      }
    }
  	if (fpid>0)
  	{
  		printf("son\n");
  		
      q[1] = fpid;
  	}
    if(fpid==0)
    {
      
      q[1] = 5;
    }

	printf("over\n");
}


