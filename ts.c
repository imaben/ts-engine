#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "ts.h"
#include "server.h"

#define fatal(fmt, ...) do { \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    exit(1); \
} while (0)

static struct option options[] = {
    {"host",        required_argument,   0,   'h'},
    {"port",        required_argument,   0,   'p'},
    {"daemon",      no_argument,         0,   'd'},
    {"help",        no_argument,         0,   '?'},
    {0, 0, 0, 0}
};

ts_setting_t __setting, *ts_setting = &__setting;

static void ts_usage() {
    char *usage =
        "Usage: ts_engine [options] [-h] <host> [-p] <port> [--] [args...]\n\n"
        " -h --host <host>\t\tThe host to bind\n"
        " -p --port <port>\t\tThe port to listen\n"
        " -d --daemon\t\t\tUsing daemonize mode\n"
        " --help\t\t\t\tDiskplay the usage\n";
    fprintf(stdout, usage);
    exit(0);
}

static void ts_parse_options(int argc, char **argv) {
    int c;
    while ((c = getopt_long(argc, argv, "h:p:d", options, NULL)) != -1) {
        switch (c) {
            case 'h':
                ts_setting->host = strdup(optarg);
                break;
            case 'p':
                ts_setting->port = atoi(optarg);
                break;
            case 'd':
                ts_setting->daemon = 1;
                break;
            case '?':
                ts_usage();
                break;
            default:
                ts_usage();
        }
    }
}

static void ts_sighandler(int signal) {
    if (signal == SIGUSR1) {
        // todo restart
        return;
    }
    ts_server_shutdown();
}

static void daemonize() {
    int fd;

    if (fork() != 0) {
        exit(0);
    }

    setsid();

    if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);

        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }
}

static void ts_signal_init() {
    sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

    struct sigaction siginfo = {
        .sa_handler = ts_sighandler,
        .sa_mask = signal_mask,
        .sa_flags = SA_RESTART,
    };
    sigaction(SIGINT, &siginfo, NULL);
    sigaction(SIGTERM, &siginfo, NULL);
    sigaction(SIGHUP, &siginfo, NULL);
    sigaction(SIGQUIT, &siginfo, NULL);
    sigaction(SIGKILL, &siginfo, NULL);
    sigaction(SIGUSR1, &siginfo, NULL);
}

int main(int argc, char **argv) {
    bzero(ts_setting, sizeof(*ts_setting));
    ts_parse_options(argc, argv);
    if (ts_setting->host == NULL || ts_setting->port == 0) {
        ts_usage();
    }
    if (ts_setting->daemon) {
        daemonize();
    }

    ts_signal_init();
    if (ts_server_init(ts_setting) < 0) {
        fatal("failed to init server\n");
    }
    if (ts_server_start() < 0) {
        fatal("failed to start server: %s\n", ts_server_err);
    }
    return 0;
}

