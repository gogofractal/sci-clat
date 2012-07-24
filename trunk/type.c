#include "type.h"

int isNum(char str[]){
	int num = strlen(str);
	int i = 0;
	for(; i < num; i++){
		if(i == 0 && (str[i] == '-' || str[i] == '+'))
			continue;
		if(!isdigit(str[i])){
			return 0;
		}
	}
	return 1;
}

char* ltrim(char *s){
    while(isspace(*s)) s++;
    return s;
}

char* rtrim(char *s){
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}

char* trim(char *s){
    return rtrim(ltrim(s));
}

int addslahses(char* in, int dim, char* out){
    int length = strlen(in);
    int i = 0;
    int j = 0;
    bzero(out, dim);

    for(; i < length; i++, j++){
        if(in[i] != '\''){
            out[j] = in[i];
        }else{
            out[j] = '\'';
            j++;
            out[j] = in[i];
        }
    }
    if(j < dim){//correcto
        return 0;
    }else{//error
        return 1;
    }
}

void md5_sum(unsigned char* in, int inDIM, int outDIM, char* out) {
    char aux[10] = {0};
    // vaciamos la cadena de salida
    bzero(out, outDIM);
    // obtenemos su valor en hexa
    int i;
    for(i=0; i < inDIM; i++) {
        bzero(aux, 10);
        sprintf(aux, "%02x", in[i]);
        strcat(out, aux);
    }
}
