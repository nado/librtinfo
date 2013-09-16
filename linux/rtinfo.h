#ifndef __SYSINFO_H
	#define __SYSINFO_H
	
	#define LIBRTINFO_DEBUG	        0            // enable lib debug messages
	
	#define LIBRTINFO_HDDTEMP_HOST  "127.0.0.1"  // hdd temp host should always be localhost
	#define LIBRTINFO_HDDTEMP_PORT  7634         // hdd temp port
	
	//
	// system defined path
	//
	#define LIBRTINFO_MEMORY_FILE   "/proc/meminfo"
	#define LIBRTINFO_LOADAVG_FILE  "/proc/loadavg"
	#define LIBRTINFO_CPU_FILE      "/proc/stat"
	#define LIBRTINFO_NET_FILE      "/proc/net/dev"
	#define LIBRTINFO_UPTIME_FILE   "/proc/uptime"
	#define LIBRTINFO_BATTERY_PATH  "/sys/class/power_supply/"
	#define LIBRTINFO_IOSTATS_FILE  "/proc/diskstats"
	
	#include <stdint.h>
	
	//
	// cpu structures
	//
	typedef struct rtinfo_cpu_time_t {
		uint64_t total;         // total cpu time
		uint64_t idle;          // idle cpu time
		
	} rtinfo_cpu_time_t;
	
	// Note: first node (index 0) is the sum of all the CPUs
	//       'nbcpu' will always be (real numbers of cpu) + 1 for the sum
	typedef struct rtinfo_cpu_node_t {
		unsigned char usage;                  // current cpu usage (in percent)
		struct rtinfo_cpu_time_t current;     // instant time values
		struct rtinfo_cpu_time_t previous;    // previous time values
		
	} rtinfo_cpu_node_t;
	
	typedef struct rtinfo_cpu_t {
		unsigned int count;               // number of cpu on system/array
		struct rtinfo_cpu_node_t *nodes;  // cpu nodes array
		
	} rtinfo_cpu_t;
	
	//
	// memory (ram/swap) structure
	//
	typedef struct rtinfo_memory_byte_t {
		uint64_t total;
		uint64_t used;
		
	} rtinfo_memory_byte_t;
	
	typedef struct rtinfo_memory_t {
		struct rtinfo_memory_byte_t ram;    // raw stats in bytes
		struct rtinfo_memory_byte_t swap;   // swap stats in bytes
		
	} rtinfo_memory_t;
	
	//
	// load average structure
	//
	typedef struct rtinfo_loadagv_t {
		float load[3];  // unix-system load average [0-3]
		                // respectivly for 1, 5 and 15 min ago
		
	} rtinfo_loadagv_t;
	
	//
	// battery structure
	//
	typedef enum info_battery_status_t {
		FULL,
		CHARGING,
		DISCHARGING,
		BATTERY_ERROR,
		BATTERY_UNKNOWN
		
	} rtinfo_battery_status_t;
	
	typedef struct rtinfo_battery_t {
		uint32_t charge_full;   // battery full charge value (unit variable)
		uint32_t charge_now;    // battery current charge value (unit variable)
		int8_t load;            // battery current load (in percent)
		
		enum info_battery_status_t status;  // battery current status
		
	} rtinfo_battery_t;
	
	//
	// network structures
	//
	typedef struct rtinfo_network_byte_t {
		uint64_t rx;       // reception bytes 
		uint64_t tx;       // transmission bytes
		
	} rtinfo_network_byte_t;
	
	// per-interface
	typedef struct rtinfo_network_node_t {
		char *name;                            // interface name
		struct rtinfo_network_byte_t current;  // current bytes transfered over the interface
		struct rtinfo_network_byte_t previous; // previous bytes transfered over the interface
		struct rtinfo_network_byte_t rate;     // current rate (in bytes/s)
		char ip[16];                           // last ip address of the interface
		uint16_t speed;                        // link speed in Mbps
		char enabled;                          // enabled/disabled flag
		
	} rtinfo_network_node_t;
	
	typedef struct rtinfo_network_t {
		unsigned int count;                  // number of interfaces
		struct rtinfo_network_node_t *nodes; // network nodes (interface) array
		
	} rtinfo_network_t;
	
	//
	// uptime unified structure
	//
	typedef struct rtinfo_uptime_t {
		uint32_t uptime;
		
	} rtinfo_uptime_t;
	
	//
	// system sensors (temperature) structure
	//
	// cpu sensors
	typedef struct rtinfo_sensors_data_t {
		uint16_t critical;        // manufacturer critical limit
		uint16_t hottest;         // hottest node value
		uint16_t average;         // average nodes values
		
	} rtinfo_sensors_data_t;

	typedef struct rtinfo_sensors_t {
		struct rtinfo_sensors_data_t cpu;  // cpu sensors
		struct rtinfo_sensors_data_t hdd;  // hdd-temp sensors values
		
	} rtinfo_sensors_t;
	
	//
	// io structures
	//
	typedef struct rtinfo_io_byte_t {
		uint64_t read;           // bytes read
		uint64_t write;          // bytes write
		
	} rtinfo_io_byte_t;
	
	typedef struct rtinfo_io_node_t {
		char *name;
		struct rtinfo_io_byte_t previous; // previous value
		struct rtinfo_io_byte_t current;  // current value
		struct rtinfo_io_byte_t rate;     // current rate in bytes/s
		
	} rtinfo_io_node_t;
	
	typedef struct rtinfo_io_t {
		unsigned int count;               // number of devices
		struct rtinfo_io_node_t *nodes;   // nodes array
		
	} rtinfo_io_t;

	//
	// exported functions prototypes
	//
	
	//
	// constructor/destructor for cpu usage
	//
	rtinfo_cpu_t *rtinfo_init_cpu();
	void rtinfo_free_cpu(rtinfo_cpu_t *cpu);
	
	// update cpu information
	rtinfo_cpu_t *rtinfo_get_cpu(rtinfo_cpu_t *cpu);
	
	// compute cpu usage
	rtinfo_cpu_t *rtinfo_usage_cpu(rtinfo_cpu_t *cpu);
	
	//
	// constructor/destructor for network usage
	//
	rtinfo_network_t * rtinfo_init_network();
	void rtinfo_free_network(rtinfo_network_t *net);
	
	// update network information
	rtinfo_network_t *rtinfo_get_network(rtinfo_network_t *net);
	rtinfo_network_t *rtinfo_get_network_ipv4(rtinfo_network_t *net);
	
	// compute network rates
	rtinfo_network_t *rtinfo_usage_network(rtinfo_network_t *net, int timewait);
	
	//
	// constructor/destructor for io usage
	//
	rtinfo_io_t *rtinfo_init_io();
	void rtinfo_free_io(rtinfo_io_t *io);
	
	// get io information
	rtinfo_io_t *rtinfo_get_io(rtinfo_io_t *io);
	
	// compute io usage
	rtinfo_io_t *rtinfo_usage_io(rtinfo_io_t *io, int timewait);
	
	//
	// standard data grabber
	// 
	rtinfo_memory_t  *rtinfo_get_memory(rtinfo_memory_t *memory);
	rtinfo_loadagv_t *rtinfo_get_loadavg(rtinfo_loadagv_t *load);
	rtinfo_battery_t *rtinfo_get_battery(rtinfo_battery_t *battery, char *name);
	rtinfo_sensors_t *rtinfo_get_sensors(rtinfo_sensors_t *sensors);
	
	rtinfo_sensors_data_t *rtinfo_sensors_cpu(rtinfo_sensors_data_t *cpu);
	rtinfo_sensors_data_t *rtinfo_sensors_hdd(rtinfo_sensors_data_t *hdd);
	
	// return a (struct tm) pointer to the current local time
	struct tm *rtinfo_get_time();
	
	// rtinfo uptime grabber
	rtinfo_uptime_t *rtinfo_get_uptime(rtinfo_uptime_t *uptime);
	
	// library version
	float rtinfo_version();
#endif
