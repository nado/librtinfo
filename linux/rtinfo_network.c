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
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/ethtool.h>
#include <unistd.h>
#include "misc.h"
#include "rtinfo.h"

static char * __rtinfo_network_getinterfacename(char *line) {
	int length = 0, i = 0, j;
	
	/* Skipping spaces */
	while(*(line + i) && isspace(*(line + i)))
		i++;
	
	/* Saving start name position */
	j = i;
	
	/* Reading name length */
	while(*(line + i) && *(line + i++) != ':')
		length++;
	
	return strndup(line + j, length);
}

static unsigned int __rtinfo_network_nbiface() {
	unsigned int nbiface = 0;
	char data[256];
	FILE *fp;
	
	if(!(fp = fopen(LIBRTINFO_NET_FILE, "r")))
		diep(LIBRTINFO_NET_FILE);
	
	/* Reading file */
	while(fgets(data, sizeof(data), fp) != NULL) {
		/* Skip header */
		if(!strncmp(data, "Inter-|", 7))
			continue;
		
		if(!strncmp(data, " face |", 7))
			continue;
		
		nbiface++;
	}
	
	fclose(fp);
	
	return nbiface;
}

static rtinfo_network_node_t *__rtinfo_network_getifbyname(rtinfo_network_t *net, char *ifname) {
	unsigned int i;
	
	/* Reading each interfaces which got already a name */
	for(i = 0; i < net->count; i++) {
		if(net->nodes[i].name && !strcmp(net->nodes[i].name, ifname))
			return net->nodes + i;
	}
	
	/* No matching interface, searching the first empty */
	rtinfo_debug("[-] librtinfo: interface <%s> not in memory, searching new slot\n", ifname);
	for(i = 0; i < net->count; i++) {
		if(!net->nodes[i].name)
			return net->nodes + i;
	}
	
	/* All slots are busy, searching the first one disabled */
	for(i = 0; i < net->count; i++) {
		if(!net->nodes[i].enabled)
			return net->nodes + i;
	}
	
	/* No empty slot, no match. Error */
	return NULL;
}

static rtinfo_network_t *__rtinfo_network_reordering(rtinfo_network_t *net) {
	rtinfo_network_node_t *copy, *current;
	unsigned int i;
	size_t u = sizeof(rtinfo_network_node_t) * net->count;
	
	/* Copy data */
	if(!(copy = malloc(u)))
		return NULL;
	
	memcpy(copy, net->nodes, u);
	
	/* Ordering [used][not used] */
	current = net->nodes;
	
	for(i = 0; i < net->count; i++) {
		if(copy[i].enabled)
			*current++ = copy[i];
	}
	
	for(i = 0; i < net->count; i++) {
		if(!copy[i].enabled)
			*current++ = copy[i];
	}
	
	free(copy);
	
	return net;
}

rtinfo_network_t *rtinfo_init_network() {
	rtinfo_network_t *net;
	
	rtinfo_debug("[+] librtinfo: initializing network\n");
	
	if(!(net = (rtinfo_network_t*) malloc(sizeof(rtinfo_network_t))))
		return NULL;
	
	// counting number of interfaces available
	net->count = __rtinfo_network_nbiface();
	
	// allocating, initializing all fields to zero
	if(!(net->nodes = (rtinfo_network_node_t*) calloc(net->count, sizeof(rtinfo_network_node_t))))
		return NULL;
	
	rtinfo_debug("[+] librtinfo: %u interfaces, %lu bytes\n",
	             net->count, net->count * sizeof(rtinfo_network_node_t));
	
	return net;
}

void rtinfo_free_network(rtinfo_network_t *net) {
	unsigned int i;
	
	for(i = 0; i < net->count; i++)
		free(net->nodes[i].name);
	
	free(net->nodes);
	free(net);
}

/* For each interfaces, save old values, write on node */
rtinfo_network_t *rtinfo_get_network(rtinfo_network_t *net) {
	FILE *fp;
	char data[256], *pdata = data;
	unsigned int i = 0;
	unsigned int newnbiface;
	char *ifname;
	rtinfo_network_node_t *intf;
	char changed = 0;

	if(!(fp = fopen(LIBRTINFO_NET_FILE, "r")))
		diep(LIBRTINFO_NET_FILE);

	if((newnbiface = __rtinfo_network_nbiface()) != net->count) {
		rtinfo_debug("[+] librtinfo: interface count changed: %u -> %u\n", net->count, newnbiface);
		
		/* Reset enabled flag, cleaning memory */
		for(i = 0; i < net->count; i++) {
			net->nodes[i].enabled = 0;
			
			free(net->nodes[i].name);
			net->nodes[i].name = NULL;
		}
		
		/* We got more interface, realloc */
		if(newnbiface > net->count) {
			if(!(net->nodes = (rtinfo_network_node_t*) realloc(net->nodes, sizeof(rtinfo_network_node_t) * newnbiface)))
				return NULL;
			
			/* Writing zero to the new interface(s) */
			bzero(net->nodes + net->count, sizeof(rtinfo_network_node_t) * (newnbiface - net->count));
			net->count = newnbiface;
		}
		
		net->count = newnbiface;
		
		/* Interfaces changed */
		changed = 1;
	}

	for(i = 0; fgets(data, sizeof(data), fp); i++) {
		if(i >= net->count) {
			rtinfo_debug("[-] librtinfo: warning, too many disks found\n");
			break;
		}
		
		// skipping header
		if(!strncmp(data, "Inter-|", 7) || !strncmp(data, " face |", 7)) {
			i--;
			continue;
		}
		
		/* Reading name */
		ifname = __rtinfo_network_getinterfacename(data);
		
		if(!(intf = __rtinfo_network_getifbyname(net, ifname))) {
			rtinfo_debug("[-] librtinfo: cannot find interface on array, this should not happen\n");
			continue;
		}
		
		// marking interface as enabled
		intf->enabled = 1;
		
		// saving previous data
		intf->previous = intf->current;
		
		/* Reading interface name (cannot be sur that the interface order has not changed) */
		free(intf->name);
		intf->name = ifname;
		
		/* Reading current values */
		if(!(pdata = skip_until_colon(data)))
			continue;
		
		// updating values
		intf->current.tx  = indexll(pdata, 8);
		intf->current.rx  = indexll(pdata, 0);
	}

	fclose(fp);
	
	/* If interfaces changed, reordering data linear */
	if(changed)
		if(!__rtinfo_network_reordering(net))
			return NULL;
	
	/* Reading ethernet informations */
	rtinfo_get_network_ipv4(net);
	
	return net;
}

rtinfo_network_t *rtinfo_get_network_ipv4(rtinfo_network_t *net) {
	int sockfd;
	struct ifconf ifconf;
	struct ifreq ifr[50];
	struct ethtool_cmd edata;
	int ifs, i;
	unsigned int j;
	char ip[INET_ADDRSTRLEN];
	struct sockaddr_in *s_in;
	
	rtinfo_debug("[+] librtinfo: reading interfaces settings\n");

	if((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0)
		diep("socket");

	ifconf.ifc_buf = (char*) ifr;
	ifconf.ifc_len = sizeof(ifr);

	if(ioctl(sockfd, SIOCGIFCONF, &ifconf) == -1)
		diep("ioctl");

	ifs = ifconf.ifc_len / sizeof(ifr[0]);
	
	/* Reset IP */
	for(j = 0; j < net->count; j++)
		*(net->nodes[j].ip) = '\0';
	
	/* Link Speed */
	bzero(&edata, sizeof(edata));
	edata.cmd = ETHTOOL_GSET;
	
	/* Reading each interfaces */
	for(i = 0; i < ifs; i++) {
		s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;

		if(!inet_ntop(AF_INET, &s_in->sin_addr, ip, sizeof(ip)))
			diep("inet_ntop");

		for(j = 0; j < net->count; j++) {
			if(!strcmp(ifr[i].ifr_name, net->nodes[j].name)) {
				/* Writing IP address */
				rtinfo_debug("[+] librtinfo: reading %d: %s/%s\n", i, ifr[i].ifr_name, ip);
				strcpy(net->nodes[j].ip, ip);
				
				/* Grabbing Link Speed */
				/* Skip loopback */
				if(!strcmp(net->nodes[j].name, "lo"))
					break;
				
				/* Reading... */
				ifr[i].ifr_data = &edata;
				if(ioctl(sockfd, SIOCETHTOOL, &ifr[i]) < 0) {
					net->nodes[j].speed = 0;
					break;
				}
				
				switch(edata.speed) {
					case SPEED_10:
						net->nodes[j].speed = 10;
					break;
					
					case SPEED_100:
						net->nodes[j].speed = 100;
					break;
					
					case SPEED_1000:
						net->nodes[j].speed = 1000;
					break;
					
					case SPEED_2500:
						net->nodes[j].speed = 2500;
					break;
					
					case SPEED_10000:
						net->nodes[j].speed = 10000;
					break;
					
					default:
						net->nodes[j].speed = 0;
				}
				
				break;
			}
		}
	}
	
	close(sockfd);

	return net;
}

rtinfo_network_t *rtinfo_usage_network(rtinfo_network_t *net, int timewait) {
	unsigned int i;
	rtinfo_network_node_t *node;
	
	
	/* Network Usage: (current load - previous load) / timewait (milli sec) */
	for(i = 0; i < net->count; i++) {
		node = &net->nodes[i];
		
		// delta rx
		if(node->current.rx - node->previous.rx > 0)
			node->rate.rx = ((node->current.rx - node->previous.rx) / (timewait / 1000));
			
		else node->rate.rx = 0;
		
		// delta tx
		if(node->current.tx - node->previous.tx > 0)
			node->rate.tx = ((node->current.tx - node->previous.tx) / (timewait / 1000));
			
		else node->rate.tx = 0;
	}

	return net;
}
