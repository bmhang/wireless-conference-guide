/* This program parses collected Station Varibles (SV) from a central database and display the result in a user friendly way.
Author
------
Michael Tetemke Mehari
michael.mehari@intec.ugent.be
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "uthash.h"
#include "mysql.h"

#define BOLD	"\033[1m\033[30m"
#define RESET	"\033[0m"

struct RECORD_t
{
	char		bssid_sta[36];		// BSSID-STA pair
	uint32_t	rx_bytes;		// recieved bytes
	uint32_t	rx_packets;		// recieved packets
	uint32_t	tx_bytes;		// transmited bytes
	uint32_t	tx_packets;		// transmited packets
	uint32_t	tx_retries;		// retransmited packets

	UT_hash_handle	hh;			// hash function handler
};

/* Function to explode string to array */
char** explode(char delimiter, char* str)
{
	char *p = strtok (str, &delimiter);
	char ** res  = NULL;
	int n_spaces = 0;

	/* split string and append tokens to 'res' */
	while (p)
	{
		res = realloc (res, sizeof (char*) * ++n_spaces);
		if (res == NULL)
			exit (-1); /* memory allocation failed */

		res[n_spaces-1] = p;
		p = strtok (NULL, &delimiter);
	}

	/* realloc one extra element for the last NULL */
	res = realloc (res, sizeof (char*) * (n_spaces+1));
	res[n_spaces] = 0;

	return res;
}

/* search element in array */
int searchElemInArray(char **array, char *str)
{
	uint8_t count = 0;
	while(array[count] != NULL)
	{
		if(strcmp(array[count], str) == 0)
			return 1;
		count++;
	}
	return 0;
}

// main function
int main(int argc, char *argv[])
{
	char *host = NULL;
	char *user = NULL;
	char *passwd = NULL;
	char *db = NULL;
	char *sniffer_MACs, **MAC_array, *sniffer_MACs_SQL = NULL;
	uint8_t MAC_count = 0;
	char *BSSIDs, **BSSID_array, *BSSIDs_SQL = NULL;
	uint8_t BSSID_count = 0;
	char *station_MAC = NULL;
	uint32_t timespan = 500000;

	struct timeval	tv;
	gettimeofday(&tv, NULL);
	uint64_t end_time = 1e6*tv.tv_sec + tv.tv_usec;

	int option = -1;
	while ((option = getopt (argc, argv, "hn:u:k:d:b:S:M:e:s:")) != -1)
	{
	    switch (option)
	    {
			case 'h':
				fprintf(stderr,"\n");
				fprintf(stderr,"Station Variable (SV) parser\n");
				fprintf(stderr,"-----------------------------\n");
				fprintf(stderr,BOLD "SV_parser" RESET " parses collected Station Varibles (SV) from a central database and ");
				fprintf(stderr,"displays the result in a user friendly way.\n\n");
				fprintf(stderr,"Argument list\n");
				fprintf(stderr,"-------------\n");
				fprintf(stderr,"-h\t\t\thelp menu\n");
				fprintf(stderr,"-n host\t\t\thost name of database server [eg. -n localhost]\n");
				fprintf(stderr,"-u user\t\t\tuser name of database server [eg. -u test]\n");
				fprintf(stderr,"-k passwd\t\tuser password of database server [eg. -k testpass]\n");
				fprintf(stderr,"-d db\t\t\tdatabase name [eg. -d test]\n");
				fprintf(stderr,"-b BSSIDs\t\tcomma separated list of BSSIDs [eg. -b 00:0e:8e:3b:2d:68,45:12:47:35:78:96]\n");
				fprintf(stderr,"-S sniffer_MACs\t\tSniffer MAC addresses during SV calculation [eg. -S 00:18:60:6d:58:53,00:1e:58:b5:66:21]\n");
				fprintf(stderr,"-M station_MAC\t\tstation MAC to get the SV from [eg. -M ec:f4:bb:6f:56:c5]\n");
				fprintf(stderr,"-e end_time\t\tend time (usec) for SV calculation [eg. -e 1467625161127011]\n");
				fprintf(stderr,"-s timespan\t\ttime span (usec) for SV calculation [eg. -s 500000]\n\n");
				fprintf(stderr,"Example\n");
				fprintf(stderr,"-------\n");
				fprintf(stderr,BOLD "./SV_parser -n 10.11.31.5 -u CREW_BM -k CREW_BM -d benchmarking -b 00:0e:8e:3b:2d:68 -s 1000000\n\n" RESET);
				return 1;
			case 'n':
				host = strdup(optarg);
				break;
			case 'u':
				user = strdup(optarg);
				break;
			case 'k':
				passwd = strdup(optarg);
				break;
			case 'd':
				db = strdup(optarg);
				break;
			case 'b':
				BSSIDs = strdup(optarg);

				// Explode the BSSIDs string into an array
				BSSID_array = explode(',', BSSIDs);
				do{
					BSSIDs_SQL = (char *) realloc(BSSIDs_SQL, (BSSID_count+1)*sizeof("'xx:xx:xx:xx:xx:xx',"));
					sprintf(BSSIDs_SQL, "%s'%s',", BSSIDs_SQL, BSSID_array[BSSID_count]);

					BSSID_count++;

				}while (BSSID_array[BSSID_count] != NULL);
				BSSIDs_SQL[strlen(BSSIDs_SQL)-1] = '\0';
				break;
			case 'S':
				sniffer_MACs = strdup(optarg);

				// Explode the sniffer_MACs string into an array
				MAC_array = explode(',', sniffer_MACs);
				do{
					sniffer_MACs_SQL = (char *) realloc(sniffer_MACs_SQL, (MAC_count+1)*sizeof("'xx:xx:xx:xx:xx:xx',"));
					sprintf(sniffer_MACs_SQL, "%s'%s',", sniffer_MACs_SQL, MAC_array[MAC_count]);

					MAC_count++;

				}while (MAC_array[MAC_count] != NULL);
				sniffer_MACs_SQL[strlen(sniffer_MACs_SQL)-1] = '\0';
				break;
			case 'M':
				station_MAC = strdup(optarg);
				break;
			case 'e':
				end_time = atoll(optarg);
				break;
			case 's':
				timespan = atoi(optarg);
				break;
			default:
				fprintf(stderr,"SV_parser: missing operand. Type './SV_parser -h' for more information.\n");
				return 1;
	    }
	}

	if(host == NULL || user == NULL || passwd == NULL || db == NULL || sniffer_MACs_SQL == NULL || BSSIDs_SQL == NULL)
	{
		fprintf(stderr,"Either or all of [Host name/User name/Password/Database/sniffer_MACss/BSSIDs] is not specified. Type './SV_parser -h' for more information\n");
		return 1;
	}

	
	MYSQL	   *conn = NULL;
	MYSQL_RES  *result;
	MYSQL_ROW  row;

	struct RECORD_t *RECORD_ptr, *tmp_ptr, *hash_ptr = NULL;

	uint64_t	start_time;
	char		*query_statement;

	uint8_t		flag;
	uint8_t		bssid_sta_path;		// 0 : station => bssid, 1 : bssid => station
	char		bssid_sta[36];
	uint32_t	frameLength;

	// initialize mysql database
	conn = mysql_init(NULL);
	if (conn == NULL)
	{
		fprintf(stderr, "Description\n");
		return 1;
	}

	// connect to mysql database
	if (mysql_real_connect(conn, host, user, passwd, db, 0, NULL, 0) == NULL)
	{
		fprintf(stderr, "Database connection error!\n");
		return 1;
	}

	start_time = end_time - timespan;
	// If a station was not included in the SQL query, query all stations from the specified BSSIDs
	if(station_MAC == NULL)
	{
		query_statement = (char *) malloc(180 + strlen(sniffer_MACs_SQL) + 2*strlen(BSSIDs_SQL));
		sprintf(query_statement, "SELECT flag,dst_mac,src_mac,frameLength from wifi_sniffer where ts > %lu AND ts < %lu AND sniffer_mac IN (%s) AND (dst_mac IN (%s) OR src_mac IN (%s)) ORDER BY ts ASC", start_time, end_time, sniffer_MACs_SQL, BSSIDs_SQL, BSSIDs_SQL);
	}
	// Else query the selected station from the specified BSSIDs
	else
	{
		query_statement = (char *) malloc(248 + strlen(sniffer_MACs_SQL) + 2*strlen(BSSIDs_SQL));
		sprintf(query_statement, "SELECT flag,dst_mac,src_mac,frameLength from wifi_sniffer where ts > %lu AND ts < %lu AND sniffer_mac IN (%s) AND ((dst_mac IN (%s) AND src_mac='%s') OR (src_mac IN (%s) AND dst_mac='%s')) ORDER BY ts ASC", start_time, end_time, sniffer_MACs_SQL, BSSIDs_SQL, station_MAC, BSSIDs_SQL, station_MAC);
	}

	if(mysql_query(conn, query_statement))
	{
		fprintf(stderr, "DB query error!\n");
		return 1;
	}

	result = mysql_store_result(conn);
	while ((row = mysql_fetch_row(result))) 
	{
		flag = atoi(row[0]);
		// Make sure the packet does not use four address format within DS (AP to AP)
		if((flag & 0x03) == 3)
			continue;

		if(searchElemInArray(BSSID_array, row[1]))
		{
			sprintf(bssid_sta, "%s#%s", row[1], row[2]);
			bssid_sta_path = 0;	// station => bssid
		}
		else if(searchElemInArray(BSSID_array, row[2]))
		{
			// Make sure the destination address is not a broadcast address
			if(strcmp(row[1], "ff:ff:ff:ff:ff:ff") == 0)
				continue;
			sprintf(bssid_sta, "%s#%s", row[2], row[1]);
			bssid_sta_path = 1;	// bssid => station
		}
		frameLength = atoi(row[3]);

		// Hash table implementation for c : https://github.com/troydhanson/uthash
		HASH_FIND_STR(hash_ptr, bssid_sta, RECORD_ptr);
		if(RECORD_ptr == NULL)
		{
			RECORD_ptr = (struct RECORD_t*)malloc(sizeof(struct RECORD_t));
			if(RECORD_ptr == NULL)
			{
				fprintf(stderr, "Unable to create record!\n");
				return 1;
			}

			strcpy(RECORD_ptr->bssid_sta, bssid_sta);
			if(bssid_sta_path == 0)	// station => bssid
			{
				RECORD_ptr->rx_bytes   = frameLength;
				RECORD_ptr->rx_packets = 1;
				RECORD_ptr->tx_bytes   = 0;
				RECORD_ptr->tx_packets = 0;
				RECORD_ptr->tx_retries = 0;
			}
			else if(bssid_sta_path == 1)	// bssid => station
			{
				RECORD_ptr->rx_bytes   = 0;
				RECORD_ptr->rx_packets = 0;
				RECORD_ptr->tx_bytes   = frameLength;
				RECORD_ptr->tx_packets = 1;
				RECORD_ptr->tx_retries = 0;
			}

			// Add the new record to the hash table
			HASH_ADD_STR(hash_ptr, bssid_sta, RECORD_ptr);
		}
		else
		{
			if(bssid_sta_path == 0)	// station => bssid
			{
				RECORD_ptr->rx_bytes   += frameLength;
				RECORD_ptr->rx_packets ++;
			}
			else if(bssid_sta_path == 1)	// bssid => station
			{
				// If packet is not retransmitted
				if(!(flag & 0x08))
				{
					RECORD_ptr->tx_bytes   += frameLength;
					RECORD_ptr->tx_packets ++;
				}
				// Packet is retransmitted
				else
				{
					RECORD_ptr->tx_retries ++;
				}
			}
		}
	}

	// Iterate through the hash table and calculate the aggregate COT metric
	HASH_ITER(hh, hash_ptr, RECORD_ptr, tmp_ptr)
	{
		printf("%s\t%u\t%u\t%u\t%u\t%u\n", RECORD_ptr->bssid_sta, RECORD_ptr->rx_bytes, RECORD_ptr->rx_packets, RECORD_ptr->tx_bytes, RECORD_ptr->tx_packets, RECORD_ptr->tx_retries);
	}
	fflush(stdout);

	// clean mysql database resources
	mysql_free_result(result);
	mysql_close(conn);

	return 0;
}
