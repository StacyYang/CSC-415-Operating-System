#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
//You may also add more include directives as well.



int main(int argc, char** argv)
{
	/* printf("%d", argc); */
	if(argc == 3){
		printf("%s, This program has been written by %s!\n", argv[1], argv[2]);
	}else if(argc == 4){
		printf("%s, This program has been written by %s %s!\n", argv[1], argv[2], argv[3]);
	}
	else if(argc > 4 || argc == 1 || argc == 2){
		printf("Invalid Arguments number.\n");
	}else{
		printf("Invalid Arguments: None are given.");
	}

	return 0;
}