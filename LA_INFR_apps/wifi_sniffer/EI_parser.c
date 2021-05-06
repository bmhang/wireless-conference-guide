/* This is an Exposure parser program using Specific Absorption Rate (SAR) metric
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
#include <math.h>
#include <sys/time.h>
#include <ctype.h>

#include "uthash.h"				// hashing function header file
#include "mysql.h"				// install libmysqlclientxx and libmysqlclient-dev before hand

#define BOLD		"\033[1m\033[30m"
#define RESET		"\033[0m"
#define PI		3.14159265358979323846

struct RECORD_t
{
	char		sniffer_mac[18];	// node identification
	double		UL_EI;			// Cumulative Up Link EI value
	double		DL_EI_SUT;		// Cumulative Down Link EI value from SUT
	double		DL_EI_BG;		// Cumulative Down Link EI value from Background
	UT_hash_handle	hh;			// hash function handler
};

// Per packet Recieved Power to Electric Field calculation
double Prx_to_EF(int8_t Prx, uint16_t freq, double Gr, double CablelossRx)
{
	double AF, P_mW, V_uV, V_dbuV, E_dBuV_m, E_V_m;

	AF = 20*log10(freq) - 29.7707 - Gr - 10*log10(1);

	P_mW = pow(10,(Prx/10.0));
	V_uV = sqrt(50*P_mW/1000)*1e6;
	V_dbuV = 20*log10(V_uV);

	E_dBuV_m = V_dbuV + AF + CablelossRx;

	E_V_m = pow(10,(E_dBuV_m/20))/1000000;

	return E_V_m;
}

// Convert a string to lowercase
char* strlwr(char* s)
{
	char* tmp = s;

	for (;*tmp;++tmp)
	{
		*tmp = tolower((unsigned char) *tmp);
	}

	return s;
}

// main function
int main(int argc, char *argv[])
{
	char *host = NULL;
	char *user = NULL;
	char *passwd = NULL;
	char *db = NULL;
	double Gr = 0;
	double CablelossRx = 10;
	double SARref_UL = 0.0070;
	double SARref_DL = 0.0028;
	char *SUT_mac = NULL;
	uint32_t timespan = 500000;

	struct timeval	tv;
	gettimeofday(&tv, NULL);
	uint64_t end_time = 1e6*tv.tv_sec + tv.tv_usec;

	int option = -1;
	while ((option = getopt (argc, argv, "hn:u:k:d:g:l:U:D:m:e:t:")) != -1)
	{
	    switch (option)
	    {
			case 'h':
				fprintf(stderr,"\n");
				fprintf(stderr,"Exposure Index (EI) parser\n");
				fprintf(stderr,"--------------------------\n");
				fprintf(stderr,BOLD "EI_parser" RESET " measures the level of Electro Magnetic exposure (i.e. uplink and downlink) from a Wi-Fi transmission as felt by a client node.\n");
				fprintf(stderr,"This parser program collects data from the database and calculates the EI metric from the specified time span.\n\n");
				fprintf(stderr,"Argument list\n");
				fprintf(stderr,"-------------\n");
				fprintf(stderr,"-h\t\t\thelp menu\n");
				fprintf(stderr,"-n host\t\t\thost name of database server [eg. -n localhost]\n");
				fprintf(stderr,"-u user\t\t\tuser name of database server [eg. -u test]\n");
				fprintf(stderr,"-k passwd\t\tuser password of database server [eg. -k testpass]\n");
				fprintf(stderr,"-d db\t\t\tdatabase name [eg. -d test]\n");
				fprintf(stderr,"-g Gr\t\t\tReciever antenna gain (dBi) [eg. -g 4 ]\n");
				fprintf(stderr,"-l CablelossRx\t\tTx/Rx cable loss (dB) [eg. -l 10 ]\n");
				fprintf(stderr,"-U SARref_UL\t\tUp   link SAR reference value (W/kg for 1W)	[eg. -U 0.0070 ]\n");
				fprintf(stderr,"-D SARref_DL\t\tDown link SAR reference value (W/kg for 1 W/m²) [eg. -D 0.0028 ]\n");
				fprintf(stderr,"-m SUT_mac\t\tComma separated Solution Under Test (SUT) MAC addresses [eg. -m 00:18:60:6d:58:53,00:1e:58:b5:66:21]\n");
				fprintf(stderr,"-e end_time\t\tend time (usec) for EI calculation [eg. -e 1467625161127011]\n");
				fprintf(stderr,"-t timespan\t\ttime span (usec) for EI calculation [eg. -t 500000]\n\n");
				fprintf(stderr,"Example\n");
				fprintf(stderr,"-------\n");
				fprintf(stderr,BOLD "./EI_parser -n localhost -u root -k root -d test -m 00:18:60:6d:58:53 -t 500000\n" RESET);
				fprintf(stderr,"Calculates the EI metric for the past 0.5 sec from SUT 00:18:60:6d:58:53\n\n");
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
			case 'g':
				Gr = atof(optarg);
				break;
			case 'l':
				CablelossRx = atof(optarg);
				break;
			case 'U':
				SARref_UL = atof(optarg);
				break;
			case 'D':
				SARref_DL = atof(optarg);
				break;
			case 'm':
				SUT_mac = strlwr(strdup(optarg));
				break;
			case 'e':
				end_time = atoll(optarg);
				break;
			case 't':
				timespan = atoi(optarg);
				break;
			default:
				fprintf(stderr,"EI_parser: missing operand. Type './EI_parser -h' for more information.\n");
				return 1;
	    }
	}

	if(host == NULL || user == NULL || passwd == NULL || db == NULL)
	{
		fprintf(stderr,"Either or all of [Host name/User name/Password/Database] is not specified. Type './EI_parser -h' for more information\n");
		return 1;
	}

	
	MYSQL	   *conn = NULL;
	MYSQL_RES  *result;
	MYSQL_ROW  row;

	struct RECORD_t *RECORD_ptr, *tmp_ptr, *hash_ptr = NULL;

	uint64_t	start_time;
	char		query_statement[128];
	char 		sniffer_mac[18], src_mac[18];
	int8_t 		Prx,Ptx;
	uint16_t	freq;
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
	sprintf(query_statement, "SELECT sniffer_mac,src_mac,SSI,freq,COT from wifi_sniffer where ts > %lu AND ts < %lu", start_time, end_time);
	if(mysql_query(conn, query_statement))
	{
		fprintf(stderr, "DB query error!\n");
		return 1;
	}

	result = mysql_store_result(conn);
	while ((row = mysql_fetch_row(result))) 
	{
		strcpy(sniffer_mac,row[0]);
		strcpy(src_mac,row[1]);

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
			// It trace is collected from where it has emmerged, calculate Up Link EI
			if(strcmp(sniffer_mac,src_mac) == 0)
			{
				Ptx = atoi(row[2]);
				COT = atof(row[4]);
				RECORD_ptr->UL_EI = COT * SARref_UL * pow(10,((Ptx-CablelossRx)/10.0)) / 1000;
				RECORD_ptr->DL_EI_SUT = 0;
				RECORD_ptr->DL_EI_BG = 0;
			}
			// Else, calculate Down Link EI
			else
			{
				Prx = atoi(row[2]);
				freq = atoi(row[3]);
				COT = atof(row[4]);
				RECORD_ptr->UL_EI = 0;
				// Exposure introduced by SUT
				if(SUT_mac != NULL && strstr(SUT_mac,src_mac))
				{
					RECORD_ptr->DL_EI_SUT = COT * SARref_DL * pow(Prx_to_EF(Prx, freq, Gr, CablelossRx),2) / (120*PI);
					RECORD_ptr->DL_EI_BG  = 0;
				}
				// Exposure introduced by background
				else
				{
					RECORD_ptr->DL_EI_SUT = 0;
					RECORD_ptr->DL_EI_BG  = COT * SARref_DL * pow(Prx_to_EF(Prx, freq, Gr, CablelossRx),2) / (120*PI);
				}
			}

			// Add the new record to the hash table
			HASH_ADD_STR(hash_ptr, sniffer_mac, RECORD_ptr);
		}
		else
		{
			// It trace is collected from where it has emmerged, calculate Up Link EI
			if(strcmp(sniffer_mac,src_mac) == 0)
			{
				Ptx = atoi(row[2]);
				COT = atof(row[4]);
				RECORD_ptr->UL_EI += COT * SARref_UL * pow(10,((Ptx-CablelossRx)/10.0)) / 1000;
			}
			// Else, calculate Down Link EI
			else
			{
				Prx = atoi(row[2]);
				freq = atoi(row[3]);
				COT = atof(row[4]);
				// Exposure introduced by SUT
				if(SUT_mac != NULL && strstr(SUT_mac,src_mac))
					RECORD_ptr->DL_EI_SUT += COT * SARref_DL * pow(Prx_to_EF(Prx, freq, Gr, CablelossRx),2) / (120*PI);
				// Exposure introduced by background
				else
					RECORD_ptr->DL_EI_BG  += COT * SARref_DL * pow(Prx_to_EF(Prx, freq, Gr, CablelossRx),2) / (120*PI);
			}
		}
	}
	
	// Iterate through the hash table and calculate the aggregate COT metric
	HASH_ITER(hh, hash_ptr, RECORD_ptr, tmp_ptr)
	{
		printf("%s\t%f\t%f\t%f\n", RECORD_ptr->sniffer_mac, 1e6*(RECORD_ptr->UL_EI/timespan), 1e6*(RECORD_ptr->DL_EI_SUT/timespan), 1e6*(RECORD_ptr->DL_EI_BG/timespan));
	}
	fflush(stdout);

	// clean mysql database resources
	mysql_free_result(result);
	mysql_close(conn);

	return 0;
}
