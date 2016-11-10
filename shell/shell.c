#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX 256

int main() {
	char *nomearquivoin, *nomearquivoout;
	char aux[MAX];
  	char *argv[MAX];
  	int pid, argc = 0, flage = 0, flagin = 0, flagout = 0;

  while (1) {
  	argc = 0;
  	flage = 0;
  	flagin = 0;
  	flagout = 0;
    printf("> ");    
    scanf("\n%[^\n]s", aux);		//Lendo a string auxiliar
      
    argv[argc] = strtok(aux, " ");
    argc++;
    argv[argc] = strtok(NULL, " ");
    while (argv[argc] != NULL){
    
    	if(argv[argc][0] == '&'){ //condicao &
    		flage = 1;
    		argv[argc] = NULL;
    	}
    	
    	else if(argv[argc][0] == '<'){ //condicao <
			nomearquivoin = strtok(NULL, " ");
    		argv[argc] = NULL;    		
    		argv[argc+1] = NULL;    		
    		flagin = 1;
    	}
    	
    	else if(argv[argc][0] == '>'){ //condicao >
			nomearquivoout = strtok(NULL, " ");
    		argv[argc] = NULL;
    		argv[argc+1] = NULL;    		
    		flagout = 1;
    	}
    	
    	else
    		argc++;
    	argv[argc] = strtok(NULL, " ");
    }    

    if (!strcmp(argv[0], "exit")) {
      exit(EXIT_SUCCESS);
    }

    pid = fork();
    
    if (pid) {		//Se o pid do processo for diferente de 0, espera o comando terminar de executar
			if(flage != 1)
				waitpid(pid, NULL, 0);
    } else {
    	if(flagin == 1)
    		freopen(nomearquivoin, "r", stdin);
    	if(flagout == 1)
    		freopen(nomearquivoout, "w", stdout);    		
      execvp(argv[0], argv);		
      printf("Erro ao executar comando!\n");
      exit(EXIT_FAILURE);
    }
  }
}
