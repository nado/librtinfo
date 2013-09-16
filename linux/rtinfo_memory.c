/*
 * system memory usage support for librtinfo
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
#include <unistd.h>
#include "misc.h"
#include "rtinfo.h"

rtinfo_memory_t * rtinfo_get_memory(rtinfo_memory_t *memory) {
	FILE *fp;
	char data[32], missing;
	unsigned int _memfree = 0, _buffers = 0, _cached = 0;
	unsigned int _swapfree = 0;

	if(!(fp = fopen(LIBRTINFO_MEMORY_FILE, "r")))
		diep(LIBRTINFO_MEMORY_FILE);

	// init memory
	memory->ram.used  = 0;
	memory->swap.used = 0;
	
	// total of lines to read
	// this prevent to read more lines as needed
	// and tracks if everything required is found
	missing = 6;

	while(missing && fgets(data, sizeof(data), fp) != NULL) {
		if(strncmp(data, "MemTotal:", 9) == 0) {
			memory->ram.total = atoll(data + 10);
			missing--;

		} else if(strncmp(data, "MemFree:", 8) == 0) {
			_memfree = atoll(data + 9);
			missing--;

		} else if(strncmp(data, "Buffers:", 8) == 0) {
			_buffers = atoll(data + 9);
			missing--;

		} else if(strncmp(data, "Cached:", 7) == 0) {
			_cached = atoll(data + 8);
			missing--;

		} else if(strncmp(data, "SwapTotal:", 10) == 0) {
			memory->swap.total = atoll(data + 11);
			missing--;

		} else if(strncmp(data, "SwapFree:", 9) == 0) {
			_swapfree = atoll(data + 9);
			missing--;
		}
	}

	fclose(fp);
	
	// if we don't have read all required filed, skipping
	if(missing)
		return NULL;

	// computing and fixing unit to bytes
	memory->ram.used  = (memory->ram.total - _memfree - _buffers - _cached) * 1024;
	memory->swap.used = (memory->swap.total - _swapfree) * 1024;
	
	// fixing unit to bytes
	memory->ram.total  *= 1024;
	memory->swap.total *= 1024;
	
	return memory;
}
