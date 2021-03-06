#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "misc.h"

void __rtinfo_internal_diep(char *str) {
	perror(str);
	exit(EXIT_FAILURE);
}

long long sum_line(char *line) {
	long long sum = 0;
	short i = 0;
	
	/* Skipping head spaces */
	while(*(line + i) && isspace(*(line + i)))
		i++;
	
	/* Calculating */
	while(*(line + i)) {
		sum += atoll(line + (i++));
		
		while(*(line + i) && !isspace(*(line + i)))
			i++;
	}
	
	return sum;
}

char *skip_until_colon(char *str) {
	while(*str) {
		if(*str == ':') {
			while(*++str == ' ');
			return str;
			
		} else str++;
	}

	return NULL;
}

char *index_string(char *str, int index) {
	short i = 0, j = 0;
	char *line = str;
	
	while(*line && isspace(*line))
		line++;
	
	for(i = 0; i < index; i++) {
		/* Skipping non-spaces */
		while(*(line + j) && !isspace(*(line + j)))
			j++;
		
		/* Skipping contingnous spaces */
		while(*(line + j) && isspace(*(line + j)))
			j++;
	}
	
	return line + j;
}

size_t field_length(char *str, int index) {
	char *init;
	int i = 0;
	
	init = index_string(str, index);
	while(init[i] && !isspace(init[i]))
		i++;
	
	return i;
}

long long indexll(char *line, int index) {
	short i = 0, j = 0;
	
	for(i = 0; i < index; i++) {
		/* Skipping non-spaces */
		while(*(line + j) && !isspace(*(line + j)))
			j++;
		
		/* Skipping contingnous spaces */
		while(*(line + j) && isspace(*(line + j)))
			j++;
	}
	
	return (*(line + j)) ? atoll(line + j) : 0;
}

char *file_get(char *filename, char *data, size_t size) {
	FILE *fp;
	
	fp = fopen(filename, "r");
	if(!fp) {
		perror(filename);
		return NULL;
	}
	
	if(!fgets(data, size, fp)) {
		fprintf(stderr, "Cannot read %s\n", filename);
		return NULL;
	}
	
	fclose(fp);
	
	return data;
}
