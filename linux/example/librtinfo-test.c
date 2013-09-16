#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <rtinfo.h>
#include <inttypes.h>

#define UPDATE_INTERVAL		1000000

int main(void) {
	rtinfo_network_t *net;
	rtinfo_io_t *io;
	rtinfo_cpu_t *cpu;
	rtinfo_memory_t memory;
	rtinfo_loadagv_t loadavg;
	rtinfo_uptime_t uptime;
	rtinfo_sensors_t sensors;
	rtinfo_battery_t battery;
	struct tm *timeinfo;
	unsigned int i;
	
	// initializing variables which require pre-allocation
	printf("[+] initializing (librtinfo %.2f)...\n", rtinfo_version());
	cpu = rtinfo_init_cpu();
	net = rtinfo_init_network();
	io  = rtinfo_init_io();

	// working
	
	/* Pre-reading data */
	printf("[+] reading data for a first time: cpu, network, io\n");
	rtinfo_get_cpu(cpu);
	rtinfo_get_network(net);
	rtinfo_get_io(io);
	
	while(1) {

	// use a timewait
	printf("[+] waiting next snapshot...\n");
	usleep(UPDATE_INTERVAL);
	
	// second read
	printf("[+] read one more time...\n");
	rtinfo_get_cpu(cpu);
	rtinfo_usage_cpu(cpu);
	// rtinfo_get_io(io);
	
	printf("[+] data dump:\n");
	for(i = 0; i < cpu->count; i++)
		printf("[ ] CPU %d: %d%%\n", i, cpu->nodes[i].usage);
	
	/* Reading Network */
	rtinfo_get_network(net);
	rtinfo_usage_network(net, UPDATE_INTERVAL / 1000);
	
	printf("[I] ---\n");
	for(i = 0; i < net->count; i++)
		printf("[ ] [%d]: %-8s | %-15s | %10" PRIu64 " b/s | %10" PRIu64 " b/s | Speed: %d Mbps\n", i, 
		       net->nodes[i].name, net->nodes[i].ip,
		       net->nodes[i].rate.tx, net->nodes[i].rate.rx,
		       net->nodes[i].speed);
	
	/* Reading Memory */
	printf("[I] ---\n");
	if(!rtinfo_get_memory(&memory))
		return 1;
	
	printf("[ ] RAM : Total: %" PRIu64 " bytes - Used: %" PRIu64 " bytes\n", memory.ram.total, memory.ram.used);
	printf("[ ] SWAP: Total: %" PRIu64 " bytes - Used: %" PRIu64 " bytes\n", memory.swap.total, memory.swap.used);
	
	/* Reading Load Average */
	printf("[I] ---\n");
	if(!rtinfo_get_loadavg(&loadavg))
		return 1;
	
	printf("[ ] Load Average: %.2f %.2f %.2f\n", loadavg.load[0], loadavg.load[1], loadavg.load[2]);
	
	/* Reading Battery State */
	printf("[I] ---\n");
	
	/* Use 'NULL' to search dynamically battery present */
	if(!rtinfo_get_battery(&battery, NULL))
		return 1;
	
	printf("[ ] Battery: Load: %d (-1 means an error)\n", battery.load);
	printf("[ ] Battery: Full now: %u / charge Now: %u / status: %d\n",
	       battery.charge_now, battery.charge_full, battery.status);
	
	printf("[I] ---\n");
	if(!rtinfo_get_uptime(&uptime))
		return 1;
	
	printf("[ ] Uptime: %u seconds\n", uptime.uptime);
	
	/* Reading Time Info */
	timeinfo = rtinfo_get_time();
	printf("[ ] Time  : %02d:%02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	
	printf("[I] ---\n");
	
	if(!rtinfo_get_sensors(&sensors))
		return 1;
	
	printf("[ ] cpu average  : %d C\n", sensors.cpu.average);
	printf("[ ] cpu critical : %d C\n", sensors.cpu.critical);
	printf("[ ] cpu hottest  : %d C\n", sensors.cpu.hottest);
	printf("[ ] hdd average  : %u C\n", sensors.hdd.average);
	printf("[ ] hdd critical : %u C\n", sensors.hdd.critical);
	printf("[ ] hdd hottest  : %u C\n", sensors.hdd.hottest);
	
	if(!rtinfo_get_io(io))
		return 1;
	
	rtinfo_usage_io(io, UPDATE_INTERVAL / 1000);
	
	for(i = 0; i < io->count; i++)
		printf("[ ] %-10s: read: %10" PRIu64 " b/s, write: %10" PRIu64 " b/s\n",
		       io->nodes[i].name, io->nodes[i].rate.read, io->nodes[i].rate.write);
	
	}

	rtinfo_free_cpu(cpu);
	rtinfo_free_network(net);
	rtinfo_free_io(io);

	
	return 0;
}
