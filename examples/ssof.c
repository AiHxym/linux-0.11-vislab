

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


int main(int argc, char const *argv[])
{
	int a;
	a = fac(100000);
	printf("%d\n", a);
	return 0;
}
