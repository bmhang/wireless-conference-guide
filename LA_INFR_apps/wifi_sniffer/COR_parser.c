/* This is a Channel Occupancy Time parser program
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
	char		sniffer_mac[18];	// Sniffer MAC address
	double		COT_AGGR;		// Channel Occupancy Time
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

// main function
int main(int argc, char *argv[])
{
	char *host = NULL;
	char *user = NULL;
	char *passwd = NULL;
	char *db = NULL;
	char *group_MAC, **MAC_array, *group_MAC_SQL = NULL;
	uint8_t MAC_count = 0;
	uint16_t freq = 2412;
	uint32_t timespan = 1000000;

	struct timeval	tv;
	gettimeofday(&tv, NULL);
	uint64_t end_time = 1e6*tv.tv_sec + tv.tv_usec;

	int option = -1;
	while ((option = getopt (argc, argv, "hn:u:k:d:m:f:e:s:")) != -1)
	{
	    switch (option)
	    {
			case 'h':
				fprintf(stderr,"\n");
				fprintf(stderr,"Channel Occupancy Ratio (COR) parser\n");
				fprintf(stderr,"------------------------------------\n");
				fprintf(stderr,BOLD "COR_parser" RESET " measures the time portion by which external interference occupies the wireless medium.\n");
				fprintf(stderr,"These measurements are taken by interference estimator nodes and the data is stored in a central database.\n");
				fprintf(stderr,"This parser programs reads the data from database and calculate the aggregate COT metric.\n\n");
				fprintf(stderr,"Argument list\n");
				fprintf(stderr,"-------------\n");
				fprintf(stderr,"-h\t\t\thelp menu\n");
				fprintf(stderr,"-n host\t\t\thost name of database server [eg. -n localhost]\n");
				fprintf(stderr,"-u user\t\t\tuser name of database server [eg. -u test]\n");
				fprintf(stderr,"-k passwd\t\tuser password of database server [eg. -k testpass]\n");
				fprintf(stderr,"-d db\t\t\tdatabase name [eg. -d test]\n");
				fprintf(stderr,"-m group_MAC\t\tGroup of MAC addresses where COR is parsed [eg. -m 00:18:60:6d:58:53,00:1e:58:b5:66:21]\n");
				fprintf(stderr,"-f frequency\t\tworking frequency in MHz [eg. -f 2412]\n");
				fprintf(stderr,"-e end_time\t\tend time (usec) for COR calculation [eg. -e 1467625161127011]\n");
				fprintf(stderr,"-s timespan\t\ttime span (usec) for COR calculation [eg. -s 500000]\n\n");
				fprintf(stderr,"Example\n");
				fprintf(stderr,"-------\n");
				fprintf(stderr,BOLD "./COR_parser -n 10.11.31.5 -u CREW_BM -k CREW_BM -d benchmarking  -m 00:18:60:6d:58:53 -s 1000000\n" RESET);
				fprintf(stderr,"Calculates the COR metric from a database server with hostname=10.11.31.5, username=CREW_BM, password=CREW_BM, database_name=benchmarking\n");
				fprintf(stderr,"from one sniffer node for the past 1 seconds\n\n");
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
			case 'm':
				group_MAC = strdup(optarg);

				// Explode the group_MAC string into an array
				MAC_array = explode(',', group_MAC);
				do{
					group_MAC_SQL = (char *) realloc(group_MAC_SQL, (MAC_count+1)*sizeof("'xx:xx:xx:xx:xx:xx',"));
					sprintf(group_MAC_SQL, "%s'%s',", group_MAC_SQL, MAC_array[MAC_count]);

					MAC_count++;

				}while (MAC_array[MAC_count] != NULL);
				group_MAC_SQL[strlen(group_MAC_SQL)-1] = '\0';
				break;
			case 'f':
				freq = atoi(optarg);
				break;
			case 'e':
				end_time = atoll(optarg);
				break;
			case 's':
				timespan = atoi(optarg);
				break;
			default:
				fprintf(stderr,"COR_parser: missing operand. Type './COR_parser -h' for more information.\n");
				return 1;
	    }
	}

	if(host == NULL || user == NULL || passwd == NULL || db == NULL || group_MAC == NULL)
	{
		fprintf(stderr,"Either or all of [Host name/User name/Password/Database/Group of MAC addresses] is not specified. Type './COR_parser -h' for more information\n");
		return 1;
	}

	
	MYSQL	   *conn = NULL;
	MYSQL_RES  *result;
	MYSQL_ROW  row;

	struct RECORD_t *RECORD_ptr, *tmp_ptr, *hash_ptr = NULL;

	uint64_t	start_time;
	char		*query_statement;
	char		sniffer_mac[18];
	double		COT;

	// initialize mysql database
	conn = mysql_init(NULL);
	if (conn == NULL)
	{
		fprintf(stderr, "Description\n");
		return 1;
	}

	// connect to qocon database
	if (mysql_real_connect(conn, host, user, passwd, db, 0, NULL, 0) == NULL)
	{
		fprintf(stderr, "Database connection error!\n");
		return 1;
	}
	

	start_time = end_time - timespan;
	query_statement = (char *) malloc(126 + strlen(group_MAC_SQL));
	sprintf(query_statement, "SELECT sniffer_mac,COT from wifi_sniffer where ts > %lu AND ts < %lu AND sniffer_mac IN (%s) and freq = %u", start_time, end_time, group_MAC_SQL, freq);
	if(mysql_query(conn, query_statement))
	{
		fprintf(stderr, "DB query error!\n");
		return 1;
	}

	result = mysql_store_result(conn);
	while ((row = mysql_fetch_row(result))) 
	{
		strcpy(sniffer_mac,row[0]);
		COT = atof(row[1]);

		// Hash table implementation for c : https://github.com/troydhanson/uthash
		HASH_FIND_STR(hash_ptr, sniffer_mac, RECORD_ptr);
		if(RECORD_ptr == NULL)
		{
			RECORD_ptr = (struct RECORD_t*)malloc(sizeof(struct RECORD_t));
			if(RECORD_ptr == NULL)
			{
				fprintf(stderr, "Unable to create record!\n");
				return 1;
			}
			strcpy(RECORD_ptr->sniffer_mac,sniffer_mac);
			RECORD_ptr->COT_AGGR = COT;

			// Add the new record to the hash table
			HASH_ADD_STR(hash_ptr, sniffer_mac, RECORD_ptr);
		}
		else
		{
			RECORD_ptr->COT_AGGR += COT;
		}
	}

	// Iterate through the hash table and calculate the aggregate COT metric
	HASH_ITER(hh, hash_ptr, RECORD_ptr, tmp_ptr)
	{
		printf("%s\t%f\n", RECORD_ptr->sniffer_mac, (RECORD_ptr->COT_AGGR/(10.0*timespan)));
	}
	fflush(stdout);

	// clean mysql database resources
	mysql_free_result(result);
	mysql_close(conn);

	return 0;
}
