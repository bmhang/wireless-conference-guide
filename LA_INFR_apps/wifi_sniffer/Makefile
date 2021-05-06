
CFLAGS = -g -Wall
LIBS = -ltrace
MYSQL_INCLUDE = `mysql_config --cflags --libs`

all: wifi_sniffer COR_parser EI_parser SV_parser

wifi_sniffer: wifi_sniffer.c
	gcc $(CFLAGS) -o wifi_sniffer wifi_sniffer.c $(LIBS) $(MYSQL_INCLUDE)

COR_parser: COR_parser.c
	gcc $(CFLAGS) -o COR_parser COR_parser.c $(MYSQL_INCLUDE)

EI_parser: EI_parser.c
	gcc $(CFLAGS) -o EI_parser EI_parser.c $(MYSQL_INCLUDE)

SV_parser: SV_parser.c
	gcc $(CFLAGS) -o SV_parser SV_parser.c $(MYSQL_INCLUDE)

clean:
	rm -f wifi_sniffer COR_parser EI_parser SV_parser
