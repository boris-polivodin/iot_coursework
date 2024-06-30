/*
 ============================================================================
 Name        : mqtt_client_sub_test.c
 Author      : admin
 Version     :
 Copyright   : Your copyright notice
 Description : test subscribe message on mqtt broker
 ============================================================================
 */

#include "MQTTClient.h"
#include "MQTTClientPersistence.h"
#include "pubsub_opts.h"
#include <unistd.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#if defined(_WIN32)
#define sleep Sleep
#else
#include <sys/time.h>
#endif

#define CLIENTID "MQTTClientSub"
#define TOPIC "/MQTT/TEST"
#define LENGTH 100

volatile int toStop = 0;

int myconnect(MQTTClient client, struct pubsub_opts opts) {
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
	MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
	int rc = 0;

	if (opts.verbose)
		printf("Connecting\n");

	if (opts.MQTTVersion == MQTTVERSION_5)
	{
		MQTTClient_connectOptions conn_opts5 = MQTTClient_connectOptions_initializer5;
		conn_opts = conn_opts5;
	}

	conn_opts.keepAliveInterval = opts.keepalive;
	conn_opts.username = opts.username;
	conn_opts.password = opts.password;
	conn_opts.MQTTVersion = opts.MQTTVersion;
	conn_opts.httpProxy = opts.http_proxy;
	conn_opts.httpsProxy = opts.https_proxy;

	if (opts.will_topic) 	/* will options */
	{
		will_opts.message = opts.will_payload;
		will_opts.topicName = opts.will_topic;
		will_opts.qos = opts.will_qos;
		will_opts.retained = opts.will_retain;
		conn_opts.will = &will_opts;
	}

	if (opts.connection && (strncmp(opts.connection, "ssl://", 6) == 0 ||
			strncmp(opts.connection, "wss://", 6) == 0))
	{
		if (opts.insecure)
			ssl_opts.verify = 0;
		else
			ssl_opts.verify = 1;
		ssl_opts.CApath = opts.capath;
		ssl_opts.keyStore = opts.cert;
		ssl_opts.trustStore = opts.cafile;
		ssl_opts.privateKey = opts.key;
		ssl_opts.privateKeyPassword = opts.keypass;
		ssl_opts.enabledCipherSuites = opts.ciphers;
		conn_opts.ssl = &ssl_opts;
	}

	if (opts.MQTTVersion == MQTTVERSION_5)
	{
		MQTTProperties props = MQTTProperties_initializer;
		MQTTProperties willProps = MQTTProperties_initializer;
		MQTTResponse response = MQTTResponse_initializer;

		conn_opts.cleanstart = 1;
		response = MQTTClient_connect5(client, &conn_opts, &props, &willProps);
		rc = response.reasonCode;
		MQTTResponse_free(response);
	}
	else
	{
		conn_opts.cleansession = 1;
		rc = MQTTClient_connect(client, &conn_opts);
	}

	if (opts.verbose && rc == MQTTCLIENT_SUCCESS)
		fprintf(stderr, "Connected\n");
	else if (rc != MQTTCLIENT_SUCCESS && !opts.quiet)
		fprintf(stderr, "Connect failed return code: %s\n", MQTTClient_strerror(rc));

	return rc;
}

void cfinish(int sig) {
	signal(SIGINT, NULL);
	toStop = 1;
}


void trace_callback(enum MQTTCLIENT_TRACE_LEVELS level, char* message) {
	fprintf(stderr, "Trace : %d, %s\n", level, message);
}

void writeFile(char file_name[], char* value, char* mode) {
	FILE *fp;
	fp = fopen(file_name, mode);
	if (fp == NULL) {
		fp = fopen(file_name, "w");
	}
	const time_t current = time(NULL);
	struct tm *now = localtime(&current);

	fprintf(fp, "\n");
	fprintf(fp, "%hu;%hhu;%hhu;%hhu;%hhu;%s", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, value);

	fclose(fp);
}

int main(int argc, char *argv[]) {
	char file_name[LENGTH] = "values.csv";
	char adress[LENGTH] = "mqtt.eclipseprojects.io";
	char login[LENGTH] = "";
	char password[LENGTH] = "";

	int rez, help_only = 1;
	while ( (rez = getopt(argc,argv,"ha:l:p:f:")) != -1) {
		switch (rez) {
		case 'h':
			printf("This help for receive information from MQTT-broker.\n\
and writing to a data's file\n\
Use parameter -a for setting ip-adress of MQTT-broker\n\
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

	struct pubsub_opts opts = {
		0, 0, 0, 0, "\n", 100,  	/* debug/app options */
		NULL, NULL, 1, 0, 0, /* message options */
		MQTTVERSION_DEFAULT, TOPIC, CLIENTID, 0, 0, login[0] == '\0' ? NULL : login, password[0] == '\0' ? NULL : password, adress, "1883", NULL, 10, /* MQTT options */
		NULL, NULL, 0, 0, /* will options */
		0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* TLS options */
		0, {NULL, NULL}, /* MQTT V5 options */
		NULL, NULL, /* HTTP and HTTPS proxies */
	};

	MQTTClient client;
//	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_createOptions createOpts = MQTTClient_createOptions_initializer;
	int rc = 0;
	char* url;
//	const char* version = NULL;

	#if !defined(_WIN32)
	    struct sigaction sa;
	#endif

//	const char* program_name = "paho_cs_sub";
//	MQTTClient_nameValue* infos = MQTTClient_getVersionInfo();

/*
	if (argc < 2)
		usage(&opts, (pubsub_opts_nameValue*)infos, program_name);

	if (getopts(argc, argv, &opts) != 0)
		usage(&opts, (pubsub_opts_nameValue*)infos, program_name);
*/
	if (strchr(opts.topic, '#') || strchr(opts.topic, '+'))
		opts.verbose = 1;

	if (opts.connection)
		url = opts.connection;
	else {
		url = malloc(100);
		sprintf(url, "%s:%s", opts.host, opts.port);
	}
	if (opts.verbose)
		printf("URL is %s\n", url);

	if (opts.tracelevel > 0) {
		MQTTClient_setTraceCallback(trace_callback);
		MQTTClient_setTraceLevel(opts.tracelevel);
	}

	if (opts.MQTTVersion >= MQTTVERSION_5)
		createOpts.MQTTVersion = MQTTVERSION_5;
	rc = MQTTClient_createWithOptions(&client, url, opts.clientid, MQTTCLIENT_PERSISTENCE_NONE,
			NULL, &createOpts);
	if (rc != MQTTCLIENT_SUCCESS) {
		if (!opts.quiet)
			fprintf(stderr, "Failed to create client, return code: %s\n", MQTTClient_strerror(rc));
		exit(EXIT_FAILURE);
	}


	#if defined(_WIN32)
		signal(SIGINT, cfinish);
		signal(SIGTERM, cfinish);
	#else
	    memset(&sa, 0, sizeof(struct sigaction));
	    sa.sa_handler = cfinish;
	    sa.sa_flags = 0;

	    sigaction(SIGINT, &sa, NULL);
	    sigaction(SIGTERM, &sa, NULL);
	#endif

	if (myconnect(client, opts) != MQTTCLIENT_SUCCESS)
		goto exit;

	if (opts.MQTTVersion >= MQTTVERSION_5) {
		MQTTResponse response = MQTTClient_subscribe5(client, opts.topic, opts.qos, NULL, NULL);
		rc = response.reasonCode;
		MQTTResponse_free(response);
	} else
		rc = MQTTClient_subscribe(client, opts.topic, opts.qos);

	if (rc != MQTTCLIENT_SUCCESS && rc != opts.qos)	{
		if (!opts.quiet)
			fprintf(stderr, "Error %d subscribing to topic %s\n", rc, opts.topic);
		goto exit;
	}

	while (!toStop)	{
		char* topicName = NULL;
		int topicLen;
		MQTTClient_message* message = NULL;

		rc = MQTTClient_receive(client, &topicName, &topicLen, &message, 1000);
		if (rc == MQTTCLIENT_DISCONNECTED)
			myconnect(client, opts);
		else if (message) {
			size_t delimlen = 0;

			if (opts.verbose)
				printf("%s\t", topicName);
			if (opts.delimiter)
				delimlen = strlen(opts.delimiter);
			if (opts.delimiter == NULL || (message->payloadlen > delimlen &&
				strncmp(opts.delimiter, &((char*)message->payload)[message->payloadlen - delimlen], delimlen) == 0))
				printf("%.*s", message->payloadlen, (char*)message->payload);
			else
				printf("%.*s%s", message->payloadlen, (char*)message->payload, opts.delimiter);

			writeFile(file_name, (char*)message->payload, "a");

//			if (message->struct_version == 1 && opts.verbose)
//				logProperties(&message->properties);
			fflush(stdout);
			MQTTClient_freeMessage(&message);
			MQTTClient_free(topicName);
		}
	}

	exit:
		MQTTClient_disconnect(client, 0);

		MQTTClient_destroy(&client);

		return EXIT_SUCCESS;
}
