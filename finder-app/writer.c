#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file_path> <text_string>\n", argv[0]);
        return 1;
    }

    const char *filepath = argv[1];
    const char *text = argv[2];

    // Inizializza syslog con LOG_USER
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);

    syslog(LOG_DEBUG, "Writing '%s' to '%s'", text, filepath);

    FILE *f = fopen(filepath, "w");
    if (!f) {
        syslog(LOG_ERR, "Failed to open file '%s' for writing", filepath);
        perror("Error opening file");
        closelog();
        return 1;
    }

    if (fprintf(f, "%s", text) < 0) {
        syslog(LOG_ERR, "Failed to write to file '%s'", filepath);
        perror("Error writing to file");
        fclose(f);
        closelog();
        return 1;
    }

    if (fclose(f) != 0) {
        syslog(LOG_ERR, "Failed to close file '%s'", filepath);
        perror("Error closing file");
        closelog();
        return 1;
    }

    closelog();
    return 0;
}

