/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 *
 * Example by ddomnik and ChatGPT
 *
 */


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_wifi.h"

#include <stdio.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"

#define SSID "your_wifi_ssid"
#define PASSWORD "your_wifi_password"

#define SERVER_PORT 3333


// Event handler for Wi-Fi events
static esp_err_t event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data) {

	printf("Event Handler ID: %ld\n", event_id);

	switch (event_id) {
        case WIFI_EVENT_AP_START:
            printf("Soft-AP started\n");
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            printf( "Station connected to Soft-AP\n");
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            printf("Station disconnected from Soft-AP\n");
            break;
        default:
            break;
    }
    return ESP_OK;
}

// Initialize Wi-Fi
void wifi_init_ap() {

	esp_netif_init(); // REQUIRED - Initialize the underlying TCP/IP stack.

#if 0 //Not actually needed for most basic setup
	esp_event_loop_create_default();
	esp_netif_create_default_wifi_ap();

    esp_event_loop_create_default();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = SSID,
            .password = PASSWORD,
            .max_connection = 4,  // Maximum number of stations that can be connected to the AP
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
    esp_wifi_start();

#endif
}

void server_task(void *pvParameters) {
    int socket_server, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create a socket
    if ((socket_server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error creating socket.\n");
        vTaskDelete(NULL);
    } else {
    	printf("Server socket created. (id: %d)\n", socket_server);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind the socket
    if (bind(socket_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Error binding socket.\n");
        close(socket_server);
        vTaskDelete(NULL);
    } else {
        char str[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &(server_addr.sin_addr.s_addr), str, INET_ADDRSTRLEN);

        printf("Server IP: %s\n", str);
        printf("Server PORT: %d\n", ntohs(server_addr.sin_port));
    }

    // Listen for incoming connections
    if (listen(socket_server, 1) < 0) {
        printf("Error listening on socket.\n");
        close(socket_server);
        vTaskDelete(NULL);
    }

    // Accept a connection
    if ((new_sock = accept(socket_server, (struct sockaddr *) &client_addr, &addr_len)) < 0) {
        printf("Error accepting connection.\n");
        close(socket_server);
        vTaskDelete(NULL);
    } else {
        printf("TCP connection accepted!\n");

        char str[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &(client_addr.sin_addr.s_addr), str, INET_ADDRSTRLEN);

        printf("TCP connection Incoming ip: %s \n", str);
        printf("TCP connection Incoming port: %d \n", ntohs(client_addr.sin_port));
        printf("TCP connection server socket: %d \n", socket_server);
        printf("TCP connection client socket: %d \n", new_sock);
    }

    printf("Server: Connection accepted from %s\n", inet_ntoa(client_addr.sin_addr));

    char rx_buffer[128];
    int len;

    while (1) {
        // Receive data
        len = recv(new_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

        if (len < 0) {
            printf("Server: Error receiving data.\n");
            break;
        } else if (len == 0) {
            printf("Server: Connection closed by client.\n");
            break;
        } else {
            rx_buffer[len] = '\0';
            printf("(at %lld) Server: Received message from client: %s\n", esp_timer_get_time()/1000, rx_buffer);

            // Send a reply
            if (send(new_sock, rx_buffer, len, 0) < 0) {
                printf("Server: Error sending reply.\n");
                break;
            }
        }
    }

    // Close sockets
    close(new_sock);
    close(socket_server);

    vTaskDelete(NULL);
}

void client_task(void *pvParameters) {
    struct sockaddr_in server_addr;
    int client_sock;

    // Create a socket
    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error creating socket.\n");
        vTaskDelete(NULL);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_aton("127.0.0.1", &server_addr.sin_addr);

    // Connect to the server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Error connecting to server.\n");
        close(client_sock);
        vTaskDelete(NULL);
    }

    char tx_buffer[] = "ping";
    char rx_buffer[128];
    int len;

    while (1) {
        // Send data
        if (send(client_sock, tx_buffer, sizeof(tx_buffer), 0) < 0) {
            printf("Client: Error sending data.\n");
            break;
        } else {
        	printf("(at %lld) Client: Message sent: %s\n", esp_timer_get_time()/1000, tx_buffer);
        }

        // Receive reply
        len = recv(client_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

        if (len < 0) {
            printf("Client: Error receiving reply.\n");
            break;
        } else if (len == 0) {
            printf("Client: Server closed connection.\n");
            break;
        } else {
            rx_buffer[len] = '\0';
            printf("(at %lld) Client: Received reply from server: %s\n", esp_timer_get_time()/1000, rx_buffer);
        }

        vTaskDelay(1000); // Delay for 1 second
    }

    // Close socket
    close(client_sock);

    vTaskDelete(NULL);
}

void app_main() {

	wifi_init_ap();

	printf("START SERVER TASK\n");
    xTaskCreate(server_task, "server_task", 4096, NULL, 5, NULL);

    printf("START CLIENT TASK\n");
    xTaskCreate(client_task, "client_task", 4096, NULL, 5, NULL);
}




#if 0
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"

#define CORE_0 0
#define CORE_1 1

#define TASK_SIZE 4096

static StackType_t StackA[TASK_SIZE] = {0};
static StackType_t StackB[TASK_SIZE] = {0};
static StackType_t StackC[TASK_SIZE] = {0};

static StaticTask_t TaskA;
static StaticTask_t TaskB;
static StaticTask_t TaskC;

TaskHandle_t taskHandle_A;
TaskHandle_t taskHandle_B;
TaskHandle_t taskHandle_C;

void taskARunnabe(void* args);
void taskBRunnabe(void* args);
void taskCRunnabe(void* args);

static int createdTaskB = 0;

void app_main(void)
{
    printf("Hello world!\n");

    printf("Starting Task C!\n");
    taskHandle_C = xTaskCreateStaticPinnedToCore(taskCRunnabe, "TaskC", TASK_SIZE, NULL, 2, StackC, &TaskC, CORE_0);
	if(!taskHandle_C)
	{
		printf("Starting Task C Failed!\n");
	}

    printf("Starting Task A!\n");
    taskHandle_A = xTaskCreateStaticPinnedToCore(taskARunnabe, "TaskA", TASK_SIZE, NULL, 2, StackA, &TaskA, CORE_1);
	if(!taskHandle_A)
	{
		printf("Starting Task A Failed!\n");
	}
}


void taskCRunnabe(void* args)
{
	while(1){
		//printf("TaskC\n");
		ESP_LOGE("C","TaskC\n");
		vTaskDelay(10); // Avoid Task watchdog for Task C
	};

	printf("End Task C!\n");
}

void taskARunnabe(void* args)
{
	//while(1){}; //Causes Task watchdog as expected

	while(1){
		if(createdTaskB == 0)
		{
			createdTaskB = 1;
			printf("Starting Task B!\n");
			taskHandle_B = xTaskCreateStaticPinnedToCore(taskBRunnabe, "TaskB", TASK_SIZE, NULL, 1, StackB, &TaskB, CORE_1);

			if(!taskHandle_B)
			{
				printf("Starting Task B Failed!\n");
			}
		}

		vTaskDelay(10); // Avoid Task watchdog for Task A

		//printf("TaskA\n");
		ESP_LOGE("A","TaskA\n");
	}

	printf("End Task A!\n");
}


void taskBRunnabe(void* args)
{
	while(1){
		ESP_LOGE("B","TaskB\n");
	};

	printf("End Task B!\n");
}

#endif
