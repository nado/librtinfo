/*
 * cpu statistics usage support for librtinfo
 * Copyright (C) 2012  DANIEL Maxime <root@maxux.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "misc.h"
#include "rtinfo.h"

rtinfo_cpu_t * rtinfo_init_cpu() {
	FILE *fp;
	char data[256];
	rtinfo_cpu_t *cpu;
	unsigned int count;
	
	if(!(fp = fopen(LIBRTINFO_CPU_FILE, "r")))
		diep(LIBRTINFO_CPU_FILE);

	/* Counting number of cpu availble */
	count = 0;
	while(fgets(data, sizeof(data), fp) != NULL) {
		/* Checking cpu line */
		if(strncmp(data, "cpu", 3) != 0)
			break;
		
		count++;
	}
	
	fclose(fp);
	
	/* Allocating */
	if(!(cpu = (rtinfo_cpu_t*) malloc(sizeof(rtinfo_cpu_t))))
		return NULL;
	
	cpu->count = count;
	
	if(!(cpu->nodes = (rtinfo_cpu_node_t *) calloc(cpu->count, sizeof(rtinfo_cpu_node_t))))
		return NULL;
	
	return cpu;
}

void rtinfo_free_cpu(rtinfo_cpu_t *cpu) {
	free(cpu->nodes);
	free(cpu);
}

/* For each CPU, save old values, write on the info_cpu_node_t current value read from CPU_FILE */
rtinfo_cpu_t * rtinfo_get_cpu(rtinfo_cpu_t *cpu) {
	FILE *fp;
	char data[256];
	unsigned short i = 0, j = 0;

	if(!(fp = fopen(LIBRTINFO_CPU_FILE, "r")))
		diep(LIBRTINFO_CPU_FILE);

	while(fgets(data, sizeof(data), fp) != NULL && j < cpu->count) {
		/* Checking cpu line */
		if(strncmp(data, "cpu", 3) != 0)
			break;
		
		/* Searching first number */
		while(!isspace(*(data + i)))
			i++;
		
		/* Saving previous data */
		cpu->nodes[j].previous = cpu->nodes[j].current;
		
		// cpu0    53464243 2698605 20822211 79462066 16726825 3778099 4582652 0 0 0
		// [name]  [.......................] [ idle ] [............................]
		// [name]  [..................... total cpu time ..........................]
		
		cpu->nodes[j].current.total = sum_line(data + i);
		cpu->nodes[j].current.idle  = indexll(data, 4);

		j++;
	}

	fclose(fp);
	
	return cpu;
}

rtinfo_cpu_t *rtinfo_usage_cpu(rtinfo_cpu_t *cpu) {
	unsigned int i, idle, cputime;
	
	// cpu sage: 100 * (delta cpu time - delta idle time) / delta cpu time
	for(i = 0; i < cpu->count; i++) {
		// skipping if idle has not changed
		if(cpu->nodes[i].current.total == cpu->nodes[i].previous.total) {
			cpu->nodes[i].usage = 0;
			continue;
		}
			
		// part 1: (delta cpu time)
		cputime = cpu->nodes[i].current.total - cpu->nodes[i].previous.total;
		
		// part 2: (delta cpu time - delta idle time)
		idle = cputime - (cpu->nodes[i].current.idle - cpu->nodes[i].previous.idle);

		// final: 100 * (delta cpu time - delta idle time) / delta cpu time
		cpu->nodes[i].usage = 100 * (idle / cputime);
	}
		
	return cpu;
}
