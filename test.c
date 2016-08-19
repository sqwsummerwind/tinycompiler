#include <stdio.h>
#include <stdlib.h>

int main(){
	int num1;
	int num2;
	int sum;
	char *src;
	src = "%d + %d = %d\n";
	num1 = 4;
	num2 = 6;

	sum = num1 + num2;
	printf(src, num1, num2, sum);
	
	return 0;

}
