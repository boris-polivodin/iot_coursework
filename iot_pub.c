/*
 ============================================================================
 Разработать собственную систему сбора информации для «умного дома» (температура, влажность,
 загазованность, потребление энергии и т. д – по выбору обучающегося) ,
 построенную на основе протокола обмена MQTT

 Датчик записывает в отдельный файл массив данных, состоящий из даты
 измерения и величину температуры.
 Издатель считывает текстовый файл и отправляет его в среду разработки Node-Red
 по протоколу обмена MQTT через фиксированный временной интервал.
 Подписчик подписан на среду Node-Red и считывает значения датчика. Подписчик
 записывает полученные данные в файл с указанием времени и даты записи.
 Программы издателя и подписчика работают одновременно.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <MQTTClient.h>

#define DELAY 60
#define MIN_VALUE 15
#define MAX_VALUE 25
#define CLIENTID "MQTTClientPub"
#define TOPIC "/MQTT/TEST"
#define QOS 1
#define TIMEOUT 10000L
#define LENGTH 100

typedef struct Sensor {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	int8_t value;
} Sensor;

const size_t COLUMNS = 6;
size_t rows;

int get_rand_range_int(const int min, const int max) {
    return (rand() % (max - min + 1)) + min;
}

struct tm* wait(double dly){

	clock_t begin = clock();
	while ((double)(clock() - begin) / CLOCKS_PER_SEC < DELAY) {}

	time_t current;
	time(&current);
    struct tm *now = localtime(&current);

    return now;
}

void getData(Sensor *s) {
	struct tm *now = wait(DELAY);
	s->year = now->tm_year + 1900;
	s->month = now->tm_mon + 1;
	s->day = now->tm_mday;
	s->hour = now->tm_hour;
	s->minute = now->tm_min;
	s->value = get_rand_range_int(MIN_VALUE, MAX_VALUE);
}

size_t getLineCount(char file_name[]) {
	FILE* in = fopen(file_name, "r");
	if (in == NULL) {
		perror( "Ошибка при открытии исходного файла\n" );
		exit(1);
	}

	size_t counter = 0; // предполагаем, что строк ноль в файле - т.е. пустой
	int ch, pre = EOF; // текущий символ, последний считаный символ

	// в цикле считаем сколько переводов строки в файле и запоминаем это в переменную counter
	// если файл пустой, то тело цикла не выполнится ни разу, так как первый считанный символ будет как раз EOF
	// и в pre останется EOF
	while ((ch = fgetc(in)) != EOF) { // в цикле проходим весь файл посимвольно
		pre = ch; // запоминаем последний считаный символ
		if ( ch == '\n' ) // если нашли перевод строки,
			++counter; // то увеличиваем счетчик строк
	}

	// весь смысл переменной pre в том, чтобы запомнить какой символ мы считали перед тем как наткнулись на EOF в файле
	// или в pre будет EOF - если тело цикла ни разу не выполнилось, это будет при пустом файле
	// последняя строка файла может заканчиваться не \n, а строку посчитать надо - вот для этого и нужна переменная pre

	if (pre == EOF) {// если файл пустой
//		puts( "Файл пустой!" ); // выводим информацию об этом
		counter = 0;
	} else if (pre != '\n') { // если pre содержит в себе не перевод строки, и файл не пустой
		++counter; // то увеличиваем счетчик строк
	}
	fclose(in);

	putchar('\n');
		// выводим на экран информацию о количестве строк
//	printf("%s %s = %lu\n", "Строк в файле", FNAME, counter);
	return counter;
}

size_t readFile(struct Sensor s[], char file_name[]) {

	FILE *in;
	in = fopen(file_name, "r");
	if (in == NULL) {
		printf("Error occured while opening input file!");
		exit(1);
	}

	int read = 0;
	size_t index = 0;
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	int8_t value;

	while (!feof(in)) {
		read = fscanf(in
				, "%hu;%hhu;%hhu;%hhu;%hhu;%hhd"
				, &year
				, &month
				, &day
				, &hour
				, &minute
				, &value);

		if (read == COLUMNS) {
			s[index].year = year;
			s[index].month = month;
			s[index].day = day;
			s[index].hour = hour;
			s[index].minute = minute;
			s[index].value = value;

			index++;
		} else {
			char str[32];
			read = fscanf(in, "%[^\n]", str);
			printf("ERROR item=%s in line=%u\n", str, index + 1);
		}

		if (ferror(in)) {
			printf("Error reading input file!");
			exit(1);
		}

	}

	fclose(in);

	return index;

}

void writeFile(char file_name[], Sensor *s, uint8_t size, char* mode) {
	FILE *fp;
	fp = fopen(file_name, mode);
	if (fp == NULL) {
		perror("Error occured while opening output file!");
		exit(1);
	}

	for (int i = 0; i < size; i++)	{
		if (rows)
			fprintf(fp, "\n");
		fprintf(fp, "%hu;%hhu;%hhu;%hhu;%hhu;%hhd", s[i].year, s[i].month, s[i].day, s[i].hour, s[i].minute, s[i].value);
		rows++;
	}
	fclose(fp);
}

int sendMessage(char message[], char adress[], char login[], char password[]) {

	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	int rc;

	MQTTClient_create(&client, adress, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.username = (login[0] == '\0' ? NULL : login);
	conn_opts.password = (password[0] == '\0' ? NULL : password);

	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
		printf("Failed to connect, return code %d\n", rc);
		exit(-1);
	}

	pubmsg.payload = message;
	pubmsg.payloadlen = strlen(message);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;

	MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
	printf("Waiting for up to %d seconds for publication of %s\n"
			"on topic %s for client with ClientID: %s\n",
			(int) (TIMEOUT / 1000), message, TOPIC, CLIENTID);
	rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
	printf("Message with delivery token %d delivered\n", token);
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);

	return rc;
}

int main(int argc, char *argv[]) {

	char file_name[LENGTH] = "values.csv";
	char adress[LENGTH] = "mqtt.eclipseprojects.io";
	char port[7] = ":1883";
	char login[LENGTH] = "";
	char password[LENGTH] = "";

	int rez, help_only = 1;
	while ( (rez = getopt(argc,argv,"ha:l:p:f:")) != -1) {
		switch (rez) {
		case 'h':
			printf("This help for creating of delivery information to MQTT-broker.\n\
and writing to a data's file\n\
Use parameter -a for setting ip-adress of MQTT-broker with port\n\
Use parameter -l and -p for setting login/password of MQTT-broker\n\
Use parameter -f for setting file's name. Default name: values.csv\n\
For example: prog -a 192.168.10.147 -l user -p 123 -f temperature.csv\n");
			break;
		case 'f':
			strcpy(file_name, optarg);
			help_only = 0;
			break;
		case 'a':
			strcpy(adress, optarg);
			help_only = 0;
			break;
		case 'l':
			strcpy(login, optarg);
			help_only = 0;
			break;
		case 'p':
			strcpy(password, optarg);
			help_only = 0;
			break;
		case '?':
			printf("Error parameter!\n");
			break;
		};
	}

	if (help_only) {
		return 0;
	}

	strcat(adress, port);

	/*
	 * в начале создаем/перезаписываем файл, потом получаем новые данные,
	 * записываем их в файл
	 * и отправить на брокер
	 */

	Sensor* sensor = (Sensor*) malloc(sizeof(Sensor));
	writeFile(file_name, sensor, 0, "w");
	rows = getLineCount(file_name);

	const time_t start_time = time(NULL);
	srand((unsigned) start_time / 2);

	char str[4];
	int res;

	while (1) {
		printf("\n");
		getData(sensor);
		writeFile(file_name, sensor, 1, "a");

		snprintf(str, sizeof str, "%d", sensor->value);

		res = sendMessage(str, adress, login, password);
	}

	free(sensor);

	return res;
}
