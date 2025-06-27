// filepath: /server/aesdsocket.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <getopt.h>  // Per getopt

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"

int server_socket = -1;
int client_socket = -1;

void handle_signal(int signum) {
    (void)signum;  // Sopprime il warning unused parameter
    syslog(LOG_INFO, "Caught signal, exiting");

    if (client_socket != -1) {
        close(client_socket);
    }
    if (server_socket != -1) {
        close(server_socket);
    }
    remove(DATA_FILE);
    closelog();
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int daemon_mode = 0;
    int opt;

    // Parsing argomenti da linea di comando
    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
            case 'd':
                daemon_mode = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[4096];
    ssize_t received;
    FILE *fp;

    // Apri syslog
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    // Registra handler per SIGINT e SIGTERM
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Crea socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Permetti riuso indirizzo
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // Configura indirizzo server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        syslog(LOG_ERR, "Bind failed: %s", strerror(errno));
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Se modalità demone, esegui fork e setup daemon
    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "Fork failed: %s", strerror(errno));
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            // Processo padre esce subito
            close(server_socket);
            exit(EXIT_SUCCESS);
        }

        // Processo figlio diventa session leader
        if (setsid() < 0) {
            syslog(LOG_ERR, "setsid failed: %s", strerror(errno));
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // Reindirizza stdin, stdout, stderr su /dev/null
        int fd = open("/dev/null", O_RDWR);
        if (fd < 0) {
            syslog(LOG_ERR, "Failed to open /dev/null: %s", strerror(errno));
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }

    // Ascolta
    if (listen(server_socket, 10) == -1) {
        syslog(LOG_ERR, "Listen failed: %s", strerror(errno));
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Ciclo infinito di accettazione connessioni
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        syslog(LOG_INFO, "Accepted connection from %s", ip_str);

        // Apri file per aggiungere dati ricevuti
        fp = fopen(DATA_FILE, "a+");
        if (!fp) {
            syslog(LOG_ERR, "Could not open file: %s", strerror(errno));
            close(client_socket);
            continue;
        }

        size_t total_received = 0;
        do {
            received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (received <= 0) {
                break;
            }
            buffer[received] = '\0';
            total_received += received;
            fputs(buffer, fp);
            if (strchr(buffer, '\n')) {
                break; // Pacchetto completo
            }
        } while (1);

        fflush(fp);
        fclose(fp);

        // Manda indietro il contenuto del file
        fp = fopen(DATA_FILE, "r");
        if (!fp) {
            syslog(LOG_ERR, "Could not open file for reading: %s", strerror(errno));
            close(client_socket);
            continue;
        }

        while ((received = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            send(client_socket, buffer, received, 0);
        }
        fclose(fp);

        syslog(LOG_INFO, "Connection closed from %s", ip_str);
        close(client_socket);
        client_socket = -1;
    }

    close(server_socket);
    remove(DATA_FILE);
    closelog();
    return 0;
}

