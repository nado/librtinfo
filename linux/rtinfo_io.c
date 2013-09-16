/*
 * network interfaces statistics support for librtinfo
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

//
// read the number of line of the diskstats file
//
static unsigned int __rtinfo_io_count() {
	unsigned int io = 0;
	char data[256];
	FILE *fp;
	
	if(!(fp = fopen(LIBRTINFO_IOSTATS_FILE, "r")))
		diep(LIBRTINFO_IOSTATS_FILE);
	
	// reading whole file to count lines
	while(fgets(data, sizeof(data), fp) != NULL)
		io++;
	
	fclose(fp);
	
	return io;
}

//
// well, the problem of this shit:
//
//   /sys/block/___/queue/hw_sector_size
// contains the block size of a disk (not every time 512 bytes)
//
// but /sys/block does not contains partitions
//
//   /sys/class/block
// contains partitions, but the file hw_sector_size does not exists for it
// we need to find the root-device of the partition to get the hw_sector_size
//
// in my computer:
//   /sys/class/block/sda1 -> ../../devices/pci0000:00/0000:00:1f.2/ata1/host0/target0:0:0/0:0:0:0/block/sda/sda1
//
// we can find that the path contains the root device: sda
// we read the symlink destination, then grab the last path-node (../sda) to get it
//

//
// check is the given block device name is a disk or not (ie: partition)
//
static int __rtinfo_io_isdisk(char *device) {
	FILE *fp;
	char path[128], data[64];
	int isdisk = 0;
	
	// at first, reading device type
	snprintf(path, sizeof(path), "/sys/class/block/%s/uevent", device);
	
	if(!(fp = fopen(path, "r"))) {
		rtinfo_perror(path);
		return 0;
	}
	
	while(fgets(data, sizeof(data), fp)) {
		if(!strncmp(data, "DEVTYPE=disk", 12)) {
			isdisk = 1;
			break;
		}
	}
	
	fclose(fp);
	
	return isdisk;
}

//
// get the root-node of a path (in this case for the root-disk)
//
static char *__rtinfo_io_getroot(char *path) {
	char *match = NULL;
	size_t len = strlen(path);
	
	// reading path from the end
	while(--len) {
		// if the last slash was already found, return this node
		if(match)
			return strndup(path + 1, match - path - 1);
			
		// reading until find a slash
		if(path[len] == '/')
			match = path + len;
	}
	
	return NULL;
}

//
// get the disk-name of a given partition
//
static char *__rtinfo_io_getdisk(char *device) {
	ssize_t size;
	char path[256], buffer[192];
	
	// building long path of the device
	snprintf(path, sizeof(path), "/sys/class/block/%s", device);
	
	// reading the symlink destination
	size = readlink(path, buffer, sizeof(buffer));
	buffer[size] = '\0';
	
	// freeing previous device name
	free(device);
	
	return __rtinfo_io_getroot(buffer);
}

static int __rtinfo_io_sector_size(char *device) {
	char path[512], data[512], *block;
	
	block = strdup(device);	
	
	// fix slashed name (ie: etherd/e12.0 -> etherd!e12.0)
	fixslashes(block);
	
	// if not a disk device, finding root disk device
	// the hw_sector_size doesn't exists for a partition
	// check the note abose
	if(!__rtinfo_io_isdisk(block))
		block = __rtinfo_io_getdisk(block);
	
	// setting path of hw_sector_size
	snprintf(path, sizeof(path), "/sys/block/%s/queue/hw_sector_size", block);
	free(block);
	
	if(!file_get(path, data, sizeof(data)))
		return 0;
	
	return atoi(data);
}

static char *__rtinfo_io_diskname(char *line) {
	char *match;
	
	while(isspace(*line) || isdigit(*line))
		line++;
	
	if(!(match = strchr(line, ' ')))
		return NULL;
	
	return strndup(line, match - line);
}

static rtinfo_io_node_t *__rtinfo_io_getdiskbyname(rtinfo_io_t *io, char *name) {
	unsigned int i;
	
	// reading each disks which got already a name
	for(i = 0; i < io->count; i++) {
		if(io->nodes[i].name && !strcmp(io->nodes[i].name, name))
			return io->nodes + i;
	}
	
	// no matching disk, searching the first empty
	rtinfo_debug("[-] librtinfo: disk <%s> not in memory, searching new slot\n", name);
	for(i = 0; i < io->count; i++) {
		if(!io->nodes[i].name)
			return io->nodes + i;
	}
	
	// no empty slot
	return NULL;
}

rtinfo_io_t *rtinfo_get_io(rtinfo_io_t *io) {
	rtinfo_io_node_t *node;
	unsigned int i;
	char data[256], *name;
	char changed = 0;
	int sectorsize;
	FILE *fp;

	if(!(fp = fopen(LIBRTINFO_IOSTATS_FILE, "r")))
		diep(LIBRTINFO_IOSTATS_FILE);

	for(i = 0; fgets(data, sizeof(data), fp); i++) {
		if(i >= io->count) {
			rtinfo_debug("[-] librtinfo: warning, too many disks found\n");
			break;
		}
		
		// reading disk name
		name = __rtinfo_io_diskname(data);
		
		if(!(node = __rtinfo_io_getdiskbyname(io, name))) {
			rtinfo_debug("[-] librtinfo: cannot find disk on array, this should not happen\n");
			continue;
		}
		
		// saving previous data
		node->previous = node->current;
		
		// updating disk name (cannot be sur that the disks order has not changed)
		free(node->name);
		node->name = name;
		
		// reading new values
		sectorsize = __rtinfo_io_sector_size(node->name);
		rtinfo_debug("[+] librtinfo: %s: sector size: %d bytes\n", node->name, sectorsize);
		
		node->current.read  = indexll(data, 6) * sectorsize;
		node->current.write = indexll(data, 10) * sectorsize;
	}

	fclose(fp);	

	return io;
}

rtinfo_io_t *rtinfo_init_io() {
	rtinfo_io_t *io;
	
	rtinfo_debug("[+] librtinfo: initializing io\n");
	
	if(!(io = (rtinfo_io_t *) malloc(sizeof(rtinfo_io_t))))
		return NULL;
	
	// counting number of devices found
	io->count = __rtinfo_io_count();
	
	// allocating, initializing all fields to zero
	if(!(io->nodes = (rtinfo_io_node_t *) calloc(io->count, sizeof(rtinfo_io_node_t))))
		return NULL;
	
	rtinfo_debug("[+] librtinfo: %u io devices\n", io->count);
	
	return io;
}

void rtinfo_free_io(rtinfo_io_t *io) {
	unsigned int i;
	
	for(i = 0; i < io->count; i++)
		free(io->nodes[i].name);
	
	free(io->nodes);
	free(io);
}

rtinfo_io_t *rtinfo_usage_io(rtinfo_io_t *io, int timewait) {
	unsigned int i;
	rtinfo_io_node_t *node;
	
	// io usage: (current bytes - previous bytes) / timewait (milli sec)
	for(i = 0; i < io->count; i++) {
		node = &io->nodes[i];
		
		if(node->current.read - node->previous.read > 0)
			node->rate.read = (node->current.read - node->previous.read) / (timewait / 1000);
			
		else node->rate.read = 0;
		
		if(node->current.write - node->previous.write > 0)
			node->rate.write = (node->current.write - node->previous.write) / (timewait / 1000);
			
		else node->rate.write = 0;
	}

	return io;
}
