#include "hal/udp_server.h"
#include "hal/sampler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#define PORT 12345
#define MAX_RESPONSE_SIZE 1500 // Maximum UDP packet size
#define MAX_COMMAND_SIZE 100

static pthread_t server_thread;
static volatile bool should_stop = false;
static bool is_initialized = false;
static char last_command[MAX_COMMAND_SIZE] = "";
static int sockfd = -1;

static void *server_thread_function();
static void handle_command(const char *command, struct sockaddr_in *client_addr);
static void send_response(const char *response, struct sockaddr_in *client_addr);
static void send_help_message(struct sockaddr_in *client_addr);
static void send_history(struct sockaddr_in *client_addr);

void UdpServer_init(void)
{
    printf("UDP Server - Initializing\n");
    assert(!is_initialized);

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Setup server address structure
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error binding socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Start server thread
    should_stop = false;
    pthread_create(&server_thread, NULL, server_thread_function, NULL);

    is_initialized = true;
    printf("UDP Server listening on port %d\n", PORT);
}

void UdpServer_cleanup(void)
{
    printf("UDP Server - Cleanup\n");
    assert(is_initialized);

    // Stop server thread
    should_stop = true;
    pthread_join(server_thread, NULL);

    // Cleanup socket
    if (sockfd >= 0)
    {
        close(sockfd);
        sockfd = -1;
    }

    is_initialized = false;
}

bool UdpServer_shouldStop(void)
{
    return should_stop;
}

static void *server_thread_function()
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[MAX_COMMAND_SIZE];

    while (!should_stop)
    {
        // Receive command
        ssize_t n = recvfrom(sockfd, buffer, MAX_COMMAND_SIZE - 1, 0,
                             (struct sockaddr *)&client_addr, &client_len);

        if (n < 0)
        {
            continue; // Error receiving, try again
        }

        // Null terminate received data
        buffer[n] = '\0';

        // Remove newline if present
        if (n > 0 && buffer[n - 1] == '\n')
        {
            buffer[n - 1] = '\0';
        }

        // Handle empty command (repeat last command)
        if (strlen(buffer) == 0)
        {
            if (strlen(last_command) == 0)
            {
                send_response("Error: no previous command\n", &client_addr);
            }
            else
            {
                handle_command(last_command, &client_addr);
            }
        }
        else
        {
            // Save command and handle it
            strncpy(last_command, buffer, MAX_COMMAND_SIZE - 1);
            last_command[MAX_COMMAND_SIZE - 1] = '\0';
            handle_command(buffer, &client_addr);
        }
    }
    return NULL;
}

static void handle_command(const char *command, struct sockaddr_in *client_addr)
{
    char response[MAX_RESPONSE_SIZE];

    if (strcmp(command, "help") == 0 || strcmp(command, "?") == 0)
    {
        send_help_message(client_addr);
    }
    else if (strcmp(command, "count") == 0)
    {
        snprintf(response, MAX_RESPONSE_SIZE, "# samples taken total: %lld\n",
                 Sampler_getNumSamplesTaken());
        send_response(response, client_addr);
    }
    else if (strcmp(command, "length") == 0)
    {
        snprintf(response, MAX_RESPONSE_SIZE, "# samples taken last second: %d\n",
                 Sampler_getHistorySize());
        send_response(response, client_addr);
    }
    else if (strcmp(command, "history") == 0)
    {
        send_history(client_addr);
    }
    else if (strcmp(command, "dips") == 0)
    {
        snprintf(response, MAX_RESPONSE_SIZE, "# light dips detected: %d\n",
                 Sampler_getDips());
        send_response(response, client_addr);
    }
    else if (strcmp(command, "stop") == 0)
    {
        send_response("Program terminating.\n", client_addr);
        should_stop = true;
    }
    else
    {
        snprintf(response, MAX_RESPONSE_SIZE,
                 "Unknown command: '%s'\nType 'help' for a list of commands.\n",
                 command);
        send_response(response, client_addr);
    }
}

static void send_response(const char *response, struct sockaddr_in *client_addr)
{
    sendto(sockfd, response, strlen(response), 0,
           (struct sockaddr *)client_addr, sizeof(*client_addr));
}

static void send_help_message(struct sockaddr_in *client_addr)
{
    const char *help_msg =
        "Accepted commands:\n"
        "help    -- show this help message\n"
        "count   -- get the total number of samples taken\n"
        "length  -- get the number of samples taken in the previous second\n"
        "dips    -- get the number of dips in the previous second\n"
        "history -- get all voltage samples (V) from the previous second\n"
        "stop    -- exit the program\n"
        "<enter> -- repeat last command\n";

    send_response(help_msg, client_addr);
}

static void send_history(struct sockaddr_in *client_addr)
{
    int size;
    double *history = Sampler_getHistory(&size);
    if (!history)
    {
        send_response("Error getting history\n", client_addr);
        return;
    }

    // Buffer for formatting response (with room for commas and newlines)
    char response[MAX_RESPONSE_SIZE];
    int response_pos = 0;

    // Format and send history in chunks of 10 numbers per line
    for (int i = 0; i < size; i++)
    {
        // Add number to response with exactly 3 decimal places
        int written = snprintf(response + response_pos,
                               MAX_RESPONSE_SIZE - response_pos,
                               "%.3f", history[i]);
        response_pos += written;

        // Add comma or newline
        if (i < size - 1)
        {
            if ((i + 1) % 10 == 0)
            {
                response[response_pos++] = '\n';
            }
            else
            {
                response[response_pos++] = ',';
                response[response_pos++] = ' ';
            }
        }

        // If buffer is getting full or this is the last number, send it
        if (response_pos > MAX_RESPONSE_SIZE - 50 || i == size - 1)
        {
            response[response_pos] = '\0';
            send_response(response, client_addr);
            response_pos = 0;
        }
    }

    // Add final newline if needed
    if (response_pos > 0 && response[response_pos - 1] != '\n')
    {
        send_response("\n", client_addr);
    }

    free(history);
}