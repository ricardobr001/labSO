
/* ==========================================================================
 * Aluno: Leonardo Zaccarias            RA: 620491
 * Aluno: Ricardo Mendes Leal Junior    RA: 562262
 * ========================================================================== */

/*
 * RSFS - Really Simple File System
 *
 * Copyright © 2010 Gustavo Maciel Dias Vieira
 * Copyright © 2010 Rodrigo Rocco Barbieri
 *
 * This file is part of RSFS.
 *
 * RSFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "disk.h"
#include "fs.h"

#define CLUSTERSIZE 4096


//cada leitura preenche 256 posicoes na fat
//cada posicao no FAT representa 1 agrupamento de 8 setores
//cada setor tem tamanho de 512 bytes
//cada chamada da funcao le/escreve 1 setor
//a cada leitura, são preenchidos 256 indices da FAT

unsigned short fat[65536];

typedef struct {
       char used;
       char name[25];
       unsigned short first_block;
       int size;
} dir_entry;

dir_entry dir[128];

typedef struct {
  int estado;
  char cluster[CLUSTERSIZE+1];
  int bloco_atual;
  int indice_relativo;
  int indice_tamanho;
} Arquivo;

Arquivo lista[128];		//lista de arquivos

//PARCIAL
int fs_init() {
	int formatado = 1, i, j = 0;
	char *buffer;

	for (i = 0 ; i < 128 ; i++){	//Colocando no vetor que todos os arquivos estão fechados
		lista[i].estado = 0;
    lista[i].indice_relativo = 0;
    lista[i].bloco_atual = 0;
    lista[i].indice_tamanho = 0;
    lista[i].cluster[0] = '\0';
	}

	buffer = (char *) fat;
	//Le FAT do disco q está nos primeiros 32 agrupamentos (32*8 setores)
	for (i = 0; i < 32*8; i++){				//percorre o inicio do disco onde a fat esta armazenada
		if(!bl_read(i, &buffer[i*512])){		//lê 512 bytes dos disco
			printf("Não foi possível efetuar a leitura da tabela FAT\n");
			return 0;						//retorna 0 se nao for possivel ler do disco
		}
	}

	//Le o diretorio do disco
	buffer = (char *) dir;
	for (i = 32*8; i < 33*8; i++){		//Le o agrupamento 32 do disco, o qual esta gravado o direitorio
		if(!bl_read(i, &buffer[j*512])){				//e grava no vetor dir
			printf("Não foi possível efetuar a leitura do diretório\n");
			return 0;
		}
		j++;
	}

	//verifica se está formatado
	for (i = 0; i < 32; i++){
		if (fat[i] != 3){		//se os 32 primeiros agrupamentos não forem da FAT (3), o disco nao esta formatado
			formatado = 0;
			break;
		}
	}

	//formata se necessario
	if(!formatado)
		if(!fs_format())
			return 0;		//se nao conseguiu formatar, retorna 0

	return 1;	//retorna 1 se foi inicializado
}

int fs_format() {
	int i, j = 0;
	char *buffer;

	//formata a fat
	for (i = 0; i < 32; i++)		//reserva os primeiros 32 agrupamentos para armazenamento da fat
		fat[i] = 3;
	fat[32] = 4;					//reserva o agrupamento de indice 32 para o diretorio
	for (i = 33; i < 65536; i++)	//marca todos os agrupamentos como lires
		fat[i] = 1;

	//formata o dir
	for (i = 0; i < 128; i++){
		dir[i].used = 0;
	}

	//grava FAT formatada no disco
	buffer = (char *) fat;
	for (i = 0; i < 32*8; i++){
		if(!bl_write(i, &buffer[i*512])){
			printf("Não foi possível gravar a formatação da FAT\n");
			return 0;						//retorna 0 se nao for possivel gravar no disco
		}
	}

	//grava o dir
	buffer = (char *) dir;
	for (i = 32*8; i < 33*8; i++){		//Le o agrupamento 32 do disco, o qual esta gravado o direitorio
		if(!bl_write(i, &buffer[j*512])){		//e grava no vetor dir
			printf("Não foi possível gravar o diretório\n");
			return 0;
		}
		j++;
	}
	return 1;
}

int fs_free() {
	int i, tam = 0;
	//percorre a fat contando os espaços vazios
	for (i = 33; i < 65536; i++){
		if(fat[i] == 1)
			tam++;
	}
	tam = tam * 4096;		//cada posicao livre na FAT representa 1 agrupamento livre = 8 setores = 4096 bytes livres no disco (?)
	return tam;
}

int fs_list(char *buffer, int size) {
	int i;
	char aux[100];
	buffer[0] = '\0';

	for (i = 0; i < 128; i++){				//percorre o diretorio
		if(dir[i].used == 1){				//copia informações para o buffer dos arquivos usados
			sprintf(aux, "%s\t\t%d\n", dir[i].name, dir[i].size);
			strcat(buffer, aux);
		}
	}

  return 1;
}

int fs_create(char* file_name) {
	int i, posicao_livre = -1, j;
	char *buffer;

	for (i = 0; i < 128; i++){				//percorre o diretorio verificando se o nome ja existe e localiza um espaço livre no diretorio
		if(strcmp(file_name, dir[i].name) == 0 && dir[i].used == 1){
			printf("Ja existe arquivo com o nome %s\n", file_name);
			return 0;
		}
		if (posicao_livre == -1 && dir[i].used == 0)
			posicao_livre = i;
	}

	if (posicao_livre == -1){
		printf("Diretorio cheio!\n");
		return 0;
	}

	//insere novo arquivo no diretorio da memoria principal
	dir[posicao_livre].used = 1;
	strcpy(dir[posicao_livre].name, file_name);
	dir[posicao_livre].size = 0;

	//procura por espaço livre na FAT
	buffer = (char *) fat;
	for (i = 33; i < 65536; i++){
		if (fat[i] == 1){
			fat[i] = 2;					//atualiza FAT da memória principal
			dir[posicao_livre].first_block = i;			//define o primeiro bloco na fat do novo arquivo

			for(i = 0 ; i < 32*8 ; i++){		//Atualizando a FAT inteira no disco
				if(!bl_write(i, &buffer[i*512])){
					printf("Nao foi possivel escrever FAT no disco!\n");
					return 0;
				}
			}

			buffer = (char *) dir;

			for(j = 0 ; i < 33*8 ; i++, j++){
				if (!bl_write(i, &buffer[j*512])){			//atualiza dir no disco
					printf("Nao foi possivel escrever dir no disco!\n");
					return 0;
				}
			}

			break;
		}
	}

	return 1;
}

int fs_remove(char *file_name) {
	int i, proximo, j = 0;
	char *buffer;

	for (i = 0; i < 128; i++){				//percorre o diretorio verificando se o nome ja existe e localiza um espaço livre no diretorio
		if(strcmp(file_name, dir[i].name) == 0){
			break;
		}
	}

	if (i == 128){
		printf("Arquivo nao existe!\n");
		return 0;
	}

	//marca o arquivo como nao usado no diretorio da memoria principal
	dir[i].used = 0;
	i = dir[i].first_block;
	buffer = (char *) fat;

	while(fat[i] != 2){		//marca na fat da memória principal
		proximo = fat[i];
		fat[i] = 1;				//atualiza FAT na memoria principal
		i = proximo;
	}

	fat[i] = 1;				//atualiza FAT na memoria principal

	for(i = 0 ; i < 32*8 ; i++){		//Atualizando a FAT inteira no disco
		if(!bl_write(i, &buffer[i*512])){
			printf("Nao foi possivel escrever FAT no disco!\n");
			return 0;
		}
	}

	buffer = (char *) dir;

	for(j = 0 ; i < 33*8 ; i++, j++){
		if (!bl_write(i, &buffer[j*512])){			//atualiza dir no disco
			printf("Nao foi possivel escrever dir no disco!\n");
			return 0;
		}
	}

	return 1;
}

//FINAL

int fs_open(char *file_name, int mode) {
	int i;

	if(mode == FS_R){	//Abre para leitura
		for (i = 0; i < 128; i++){				//percorre o diretorio buscando o arquivo
			if(strcmp(file_name, dir[i].name) == 0){		//Se encontrar o arquivo com o mesmo nome
        lista[i].estado = 1;
        lista[i].bloco_atual = dir[i].first_block;
				return i;		//Retorna sua posição no diretório
			}
		}

		if (i == 128){
			printf("Arquivo nao existe!\n");
			return -1;
		}
	}

	else if (mode == FS_W){	//Abre para escrita
    for (i = 0; i < 128; i++){				//percorre o diretorio buscando o arquivo
      if(strcmp(file_name, dir[i].name) == 0){
        if(!fs_remove(file_name)){
          printf("Erro ao sobrescrever o arquivo %s\n", file_name);
          return -1;
        }

        if(!fs_create(file_name)){		//Se não conseguiu criar o arquivo, gera um erro
          printf("Erro ao criar o arquivo %s\n", file_name);
          return -1;
        }

        for (i = 0 ; i < 128 ; i++){		//Buscando o novo arquivo criado
          if(strcmp(file_name, dir[i].name) == 0){		//Se encontrar o arquivo com o mesmo nome
            lista[i].estado = 1;
            lista[i].bloco_atual = dir[i].first_block;
            return i;		//Retorna sua posição no diretório
          }
        }
      }
    }

		if (i == 128){
			if(!fs_create(file_name)){		//Se não conseguiu criar o arquivo, gera um erro
				printf("Erro ao criar o arquivo %s\n", file_name);
				return -1;
			}

			for (i = 0 ; i < 128 ; i++){		//Buscando o novo arquivo criado
				if(strcmp(file_name, dir[i].name) == 0){		//Se encontrar o arquivo com o mesmo nome
					lista[i].estado = 1;
          lista[i].bloco_atual = dir[i].first_block;
					return i;		//Retorna sua posição no diretório
				}
			}
		}
	}

  printf("Modo de abertura não reconhecido\n");
	return -1;
}

int fs_close(int file) {
  int i, j;
  char *stream;

	if(lista[file].estado){		//Se o arquivo estiver aberto, fecha ele
    for(i = 0 ; i < 8 ; i++){   //Gravando no disco os dados do arquivo que ainda não foram gravados
      if(!bl_write(lista[file].bloco_atual*8 + i, &lista[file].cluster[i*512])){
        printf("Não foi possível efetuar a escrita");
        return -1;
      }
    }

    stream = (char *) fat;

    for(i = 0 ; i < 32*8 ; i++){		        //Atualizando a FAT inteira no disco
      if(!bl_write(i, &stream[i*512])){
        printf("Nao foi possivel escrever FAT no disco!\n");
        return 0;
      }
    }

    stream = (char *) dir;

    for(j = 0 ; i < 33*8 ; i++, j++){
      if (!bl_write(i, &stream[j*512])){			//atualiza dir no disco
        printf("Nao foi possivel escrever dir no disco!\n");
        return 0;
      }
    }

    lista[file].estado = 0;
    lista[file].bloco_atual = 0;
    lista[file].indice_relativo = 0;
    lista[file].indice_tamanho = 0;
		return 1;
	}

	printf("Erro ao fechar o arquivo, o arquivo já se encontra fechado\n");		//Se não o arquivo já está fechado
	return 0;
}

int fs_write(char *buffer, int size, int file) {
	int i, j;
  char *stream;

  dir[file].size += size;

	if(lista[file].estado == 1){
    if(lista[file].indice_relativo + size < CLUSTERSIZE){
  		strcat(lista[file].cluster, buffer);
      lista[file].indice_relativo += size;

      for(i = 0 ; i < size ; i++){    //Limpando o buffer
        buffer[i] = '\0';
      }
    }
    else{
      strncat(lista[file].cluster, buffer, (CLUSTERSIZE - lista[file].indice_relativo));    //6 pois sempre colocamos no cluster de 10 em 10 bytes

      for(i = 0 ; i < size ; i++){    //Limpando o buffer
        buffer[i] = '\0';
      }

      for(i = 0 ; i < 8 ; i++){   //Descarregando o cluster no disco
        if(!bl_write(lista[file].bloco_atual*8 + i, &lista[file].cluster[i*512])){
          printf("Não foi possível efetuar a escrita");
          return -1;
        }
      }

      for(i = 0 ; i < CLUSTERSIZE+1 ; i++){    //Limpando o cluster
        lista[file].cluster[i] = '\0';
      }

      strcat(lista[file].cluster, &buffer[CLUSTERSIZE - lista[file].indice_relativo]);
      lista[file].indice_relativo = CLUSTERSIZE - lista[file].indice_relativo;

      //encontra posicao livre na fat
      for (i = 33; i < 65536; i++){
        if(fat[i] == 1){
          fat[lista[file].bloco_atual] = i;   //Atualiza o inode na fat
          lista[file].bloco_atual = i;		//cresce o arquivo na FAT
          fat[i] = 2;
          break;
        }
      }

      stream = (char *) fat;

      for(i = 0 ; i < 32*8 ; i++){		        //Atualizando a FAT inteira no disco
    		if(!bl_write(i, &stream[i*512])){
    			printf("Nao foi possivel escrever FAT no disco!\n");
    			return 0;
    		}
    	}

    	stream = (char *) dir;

    	for(j = 0 ; i < 33*8 ; i++, j++){
    		if (!bl_write(i, &stream[j*512])){			//atualiza dir no disco
    			printf("Nao foi possivel escrever dir no disco!\n");
    			return 0;
    		}
    	}
    }
	}
  else {
		printf("O arquivo não está aberto\n");
		return -1;
	}

	return size;
}

int fs_read(char *buffer, int size, int file) {
	int i, j, contador = 0;

  if(lista[file].estado == 1){
    for(i = 0 ; i < size ; i++){    //Limpando o buffer
      buffer[i] = '\0';
    }

    for(i = 0; i < size; i++){
      if (lista[file].indice_relativo + size > CLUSTERSIZE){   //Se encher o cluster da estrutura, carrega novo cluster
        lista[file].bloco_atual = fat[lista[file].bloco_atual];   //Atualizar o bloco atual da estrutura para o próximo bloco do arquivo da fat
        for(j = 0 ; j < 8 ; j++){   //carrega novo cluster
          if(!bl_read(lista[file].bloco_atual*8 + j, &lista[file].cluster[j*512])){
            printf("Não foi possível efetuar a leitura\n");
            return -1;
          }
        }
        lista[file].indice_relativo = 0;    //Zerando o índice relativo
      }

      if(lista[file].indice_tamanho == dir[file].size){   //Se o tamanho do indice_tamanho for igual ao tamanho do arquivo
        return contador;    //Retorna a quantidade de bytes lidos
      }

      buffer[i] = lista[file].cluster[lista[file].indice_relativo];     //Le para o buffer o conteudo do cluster
      lista[file].indice_relativo++;    //Andando os indices
      lista[file].indice_tamanho++;
      contador++;
    }

    return size;
  }

	printf("O arquivo não está aberto\n");
	return -1;
}
