/* This is a parser program for the MOS estimator application
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
#include <math.h>

#include "mysql.h"
#include "uthash.h"	// hashing function header file

#define BOLD	"\033[1m\033[30m"
#define RESET	"\033[0m"

struct RECORD_t
{
	char		sniffer_mac[18];	// Sniffer MAC address
	uint16_t	count;			// Number of scores recieved by this sniffer
	double		PL_sum;			// Aggregate packet loss score
	double		JT_sum;			// Aggregate jitter score
	double		LAT_sum;		// Aggregate latency score
	UT_hash_handle	hh;			// hash function handler
};

// Convert normalized MOS score to R values using modified bairstow's polynomial evaluation method
double normMOS_to_R(double MOS_norm)
{
	// f(R) = R^3 - 160R^2 + 2000/7R + 4*MOS_norm/0.000007
	double fn[4] = {571428.571428571*MOS_norm, 285.714285, -160, 1};
	double epsilon = 1e-10;				// stopping criterion
	double roots[3];				// root holding colomn vector
	double b[4];					// remainder holding variable
	double c[4];					// correction holding variable
	double r=1, s=2, dr, ds;			// coorection parameter for r and s
	int8_t i;

	while(1)
	{
		// iterative procedure to calculate the b coefficients
		b[3] = fn[3];
		b[2] = fn[2] + b[3]*r;
		for(i=1; i>=0; i--)
			b[i]=fn[i] + r*b[i+1] + s*b[i+2];
	   
		if (b[0]<epsilon && b[1]<epsilon)		// if remainder is sufficiently small
		{
			roots[0] = 0.5*(r + sqrt(r*r+4*s));	// root 1
			roots[1] = 0.5*(r - sqrt(r*r+4*s));	// root 2
			roots[2] = -b[1]/b[2];			// root 3
			break;
		}
		else
		{
			// iterative procedure to calculate the c coefficients
			c[3] = b[3];
			c[2] = b[2] + c[3]*r;
			for(i=1; i>=0; i--)
			   c[i] = b[i] + r*c[i+1] + s*c[i+2];

			dr = (b[1]*c[2]-b[0]*c[3])/(c[1]*c[3]-c[2]*c[2]);
			ds = (b[0]*c[2]-b[1]*c[1])/(c[1]*c[3]-c[2]*c[2]);

			r += dr;
			s += ds;
		}
	}

	// Return the maximum R value
	if(roots[0] < 100 && roots[0] > 0)
		return roots[0];
	else if(roots[1] < 100 && roots[1] > 0)
		return roots[1];
	else if(roots[2] < 100 && roots[2] > 0)
		return roots[2];
	else
		return 93.2;
}

/* This function is experimental. It tries to evaluate the normalized MOS score for different encoder bitrates (kbps) as a funciton of encoder type, audio signal bandwidth and encoder bitrate */
double MOS_enc(char * pt, char *audio_BW, double bitrate)
{
	double x = bitrate;

	// OPUS encoding
	if(strcmp(pt, "opus") == 0)
	{
		// Narrow band signal
		if(strcmp(audio_BW, "NARROW-BAND") == 0)
		{
			if(bitrate <= 20.0 && bitrate >= 6.0)
			{
				// 5th degree polynomial approximation of OPUS encoder
				// MOS_norm = ax^5 + bx^4 + cx^3 + dx^2 + ex + f
				double a = 3.47755328189003e-06, b = -2.67166900312929e-04, c = 8.20925600818548e-03, d = -1.27020852209707e-01, e = 1.00001497249875, f = -2.30990939984253;
				return ((((a*x + b)*x + c)*x + d)*x + e)*x + f;
			}
			else if(bitrate > 20.0)
				return 0.9376;
			else
				return 0.5714;
		}
		// Medium band signal
		else if(strcmp(audio_BW, "MEDIUM-BAND") == 0)
		{
			if(bitrate <= 25.0 && bitrate >= 6.0)
			{
				// 5th degree polynomial approximation of OPUS encoder
				// MOS_norm = ax^5 + bx^4 + cx^3 + dx^2 + ex + f
				double a = 4.91874147468462e-07, b = -4.94878327186501e-05, c =	1.98537222975071e-03, d = -4.02337679860307e-02, e = 4.20643199817638e-01, f = -9.36075470817430e-01;
				return ((((a*x + b)*x + c)*x + d)*x + e)*x + f;
			}
			else if(bitrate > 25.0)
				return 0.9276;
			else
				return 0.5079;
		}
		// Wideband signal
		else if(strcmp(audio_BW, "WIDE-BAND") == 0)
		{
			if(bitrate <= 35.0 && bitrate >= 6.0)
			{
				// 5th degree polynomial approximation of OPUS encoder
				// MOS_norm = ax^5 + bx^4 + cx^3 + dx^2 + ex + f
				double a = 2.18805762755069e-07, b = -2.67036238054913e-05, c = 1.27135673182267e-03, d = -2.97438959527979e-02, e = 3.48718494477557e-01, f = -7.83557213137123e-01;
				return ((((a*x + b)*x + c)*x + d)*x + e)*x + f;
			}
			else if(bitrate > 35.0)
				return 0.9147;
			else
				return 0.4797;
		}
		// Supper wideband signal
		else if(strcmp(audio_BW, "SUPPER-WIDE-BAND") == 0)
		{
			if(bitrate <= 41.0 && bitrate >= 8.7)
			{
				// 7th degree polynomial approximation of OPUS encoder
				// MOS_norm = ax^7 + bx^6 + cx^5 + dx^4 + ex^3 + fx^2 + gx + h
				double a = 1.89491748228137e-09, b = -3.64617249621041e-07, c = 2.92819261703315e-05, d = -1.26921705706618e-03, e = 3.20080289687522e-02, f = -4.69600829260597e-01, g = 3.73035932685573, h = -11.7287903519960;
				return ((((((a*x + b)*x + c)*x + d)*x + e)*x + f)*x + g)*x + h;
			}
			else if(bitrate > 41.0)
				return 0.9043;
			else
				return 0.2958;
		}
		// Fullband signal
		else if(strcmp(audio_BW, "FULL-BAND") == 0)
		{
			if(bitrate <= 50.0 && bitrate >= 8.5)
			{
				// 9th degree polynomial approximation of OPUS encoder
				// MOS_norm = ax^9 + bx^8 + cx^7 + dx^6 + ex^5 + fx^4 + gx^3 + hx^2 + ix + j
				double a = 1.09702479278367e-12, b = -3.11159330808731e-10, c = 3.83347910521901e-08, d = -2.68760622413805e-06, e = 1.17966458339847e-04, f = -3.35730316669723e-03, g = 6.19291665885357e-02, h = -7.15228416905110e-01, i = 4.72561014094321, j = -12.9908486208936;
				return ((((((((a*x + b)*x + c)*x + d)*x + e)*x + f)*x + g)*x + h)*x + i)*x + j;
			}
			else if(bitrate > 50.0)
				return 0.9113;
			else
				return 0.3438;
		}
		else
			return -1;
	}
	// SPEEX encoding
	if(strcmp(pt, "speex") == 0)
	{
		// Narrow band signal
		if(strcmp(audio_BW, "NARROW-BAND") == 0)
		{
			return -1;
		}
		// Medium band signal
		else if(strcmp(audio_BW, "MEDIUM-BAND") == 0)
		{
			return -1;
		}
		// Wideband signal
		else if(strcmp(audio_BW, "WIDE-BAND") == 0)
		{
			if(bitrate <= 25.0 && bitrate >= 2.4)
			{
				// 5th degree polynomial approximation of speex encoder
				// MOS_norm = ax^5 + bx^4 + cx^3 + dx^2 + ex + f
				double a = 2.27024851581720e-07, b = -2.31212177346099e-05, c = 9.27816218582455e-04, d = -1.86631060213156e-02, e = 1.96110851677524e-01, f = -7.70503160267407e-02;
				return ((((a*x + b)*x + c)*x + d)*x + e)*x + f;
			}
			else if(bitrate > 25.0)
				return 0.8437;
			else
				return 0.2982;
		}
		else
			return -1;
	}
	else
		return -1;
}

// main function
int main(int argc, char *argv[])
{
	char		*host = NULL;
	char		*user = NULL;
	char		*passwd = NULL;
	char		*db = NULL;
	char		*SUT_mac = NULL;
	uint32_t	timespan = 2400000;
	char 		*pt = NULL;
	char 		*audio_BW = NULL;
	double		bitrate = 16.8;

	struct timeval	systime;
	gettimeofday(&systime, NULL);
	uint64_t end_time = 1e6*systime.tv_sec + systime.tv_usec;

	int option = -1;
	while ((option = getopt (argc, argv, "hn:u:k:d:m:e:t:p:B:b:")) != -1)
	{
	    switch (option)
	    {
			case 'h':
				fprintf(stderr,"Mean Opinion Score (MOS) parser\n");
				fprintf(stderr,"-------------------------------\n");
				fprintf(stderr,BOLD "MOS_parser" RESET " is a program that parses MOS values from database and calculate the aggregate MOS metric for the specified group of nodes.\n\n");
				fprintf(stderr,"Argument list\n");
				fprintf(stderr,"-------------\n");
				fprintf(stderr,"-h\t\t\thelp menu\n");
				fprintf(stderr,"-n host\t\t\thost name of database server [eg. -n localhost]\n");
				fprintf(stderr,"-u user\t\t\tuser name of database server [eg. -u test]\n");
				fprintf(stderr,"-k passwd\t\tuser password of database server [eg. -k testpass]\n");
				fprintf(stderr,"-d db\t\t\tdatabase name [eg. -d test]\n");
				fprintf(stderr,"-m SUT_mac\t\tComma separated Solution Under Test (SUT) MAC addresses [eg. -m 00:18:60:6d:58:53,00:1e:58:b5:66:21]\n");
				fprintf(stderr,"-e end_time\t\tend time (usec) for MOS calculation [eg. -e 1429113498770412]\n");
				fprintf(stderr,"-t timespan\t\ttime span (usec) for MOS calculation [eg. -t 2400000]\n");
				fprintf(stderr,"-p pt\t\tRTP audio payload type [eg. -p opus]\n");
				fprintf(stderr,"-B audio_BW\t\taudio signal band [eg. -B WIDE-BAND]\n");
				fprintf(stderr,"-b bitrate\t\taudio bitrate (kbps) [eg. -b 16.8]\n\n");
				fprintf(stderr,"Example\n");
				fprintf(stderr,"-------\n");
				fprintf(stderr,BOLD "./MOS_parser -n 127.0.0.1 -u root -k root -d benchmarking -m 00:18:60:6d:58:53 -t 3000000\n\n" RESET);
				fprintf(stderr,"Calculates the aggregate MOS score of a node (i.e. 00:18:60:6d:58:53) for the past 3 sec.\n");
				fprintf(stderr,"It reads the data from a database server with hostname=127.0.0.1, username=root, password=root and database_name=benchmarking.\n\n");
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
				SUT_mac = strdup(optarg);
				break;
			case 'e':
				end_time = atoll(optarg);
				break;
			case 't':
				timespan = atoi(optarg);
				break;
			case 'p':
				pt = strdup(optarg);
				break;
			case 'B':
				audio_BW = strdup(optarg);
				break;
			case 'b':
				bitrate = atof(optarg);
				break;

			default:
				fprintf(stderr,"MOS_parser: missing operand. Type './MOS_parser -h' for more information.\n");
				return 1;
	    }
	}

	if(host == NULL || user == NULL || passwd == NULL || db == NULL || SUT_mac == NULL || pt == NULL || audio_BW == NULL)
	{
		fprintf(stderr,"Either or all of [Host name/User name/Password/Database/SUT MAC addresses/pt/audio_BW] is not specified. Type './MOS_parser -h' for more information\n");
		return 1;
	}

	
	MYSQL		*conn = NULL;
	MYSQL_RES	*result;
	MYSQL_ROW	row;

	struct RECORD_t *RECORD_ptr, *tmp_ptr, *hash_ptr = NULL;

	uint64_t	start_time;
	char		query_statement[128];
	char		sniffer_mac[18];

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
	sprintf(query_statement, "SELECT sniffer_mac,packetLoss,jitter,latency FROM PL_JT_LAT_MOS where ts > %lu AND ts < %lu", start_time, end_time);

	if(mysql_query(conn, query_statement))
	{
		fprintf(stderr, "DB query error!\n");
		return 1;
	}

	result = mysql_store_result(conn);
	while ((row = mysql_fetch_row(result))) 
	{
		// Check if current node is part of the specified group of node IPs
		if(strstr(SUT_mac,row[0]) == NULL)
			continue;

		strcpy(sniffer_mac,row[0]);

		// Hash table implementation for c : https://github.com/troydhanson/uthash
		HASH_FIND_STR(hash_ptr, sniffer_mac, RECORD_ptr);
		if(RECORD_ptr == NULL)
		{
			RECORD_ptr= (struct RECORD_t*)malloc(sizeof(struct RECORD_t));
			if(RECORD_ptr == NULL)
			{
				fprintf(stderr, "Unable to create record!\n");
				return 1;
			}
			strcpy(RECORD_ptr->sniffer_mac,sniffer_mac);
			RECORD_ptr->count = 1;
			RECORD_ptr->PL_sum  = atof(row[1]);
			RECORD_ptr->JT_sum  = atof(row[2]);
			RECORD_ptr->LAT_sum = atof(row[3]);

			// Add the new record to the hash table
			HASH_ADD_STR(hash_ptr, sniffer_mac, RECORD_ptr);
		}
		else
		{
			RECORD_ptr->count++;
			RECORD_ptr->PL_sum  += atof(row[1]);
			RECORD_ptr->JT_sum  += atof(row[2]);
			RECORD_ptr->LAT_sum += atof(row[3]);
		}
	}

	// Iterate through hash table and calculate the average MOS score
	HASH_ITER(hh, hash_ptr, RECORD_ptr, tmp_ptr)
	{
		double PL_AVG, JT_AVG, LAT_AVG, maxR, effectiveLatency, R, MOS_AVG;

		PL_AVG  = RECORD_ptr->PL_sum  / RECORD_ptr->count;
		JT_AVG  = RECORD_ptr->JT_sum  / RECORD_ptr->count;
		LAT_AVG = RECORD_ptr->LAT_sum / RECORD_ptr->count;

		// Re-calculate MOS score
		maxR = normMOS_to_R(MOS_enc(pt, audio_BW, bitrate));
		effectiveLatency = LAT_AVG + 2*JT_AVG + 0.01;
		if(effectiveLatency < 0.16)
			R = maxR - 25*effectiveLatency;
		else
			R = maxR - (100*effectiveLatency - 12);
		R = R - 250*PL_AVG;
		MOS_AVG = (R < 0) ? 0 : ((0.01) * R + (1.75e-6)*R*(R-60)*(100-R));

		printf("%s\t%f\t%f\t%f\t%f\n", RECORD_ptr->sniffer_mac, PL_AVG, JT_AVG, LAT_AVG, MOS_AVG);
	}
	fflush(stdout);

	// clean mysql database resources
	mysql_free_result(result);
	mysql_close(conn);

	return 0;
}
