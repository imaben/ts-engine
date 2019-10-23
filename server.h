#pragma once

#include "ts.h"

#define MAX_PACKET_SIZE 102400

int ts_server_init(ts_setting_t *setting);
int ts_server_start();
void ts_server_shutdown();

extern char ts_server_err[256];

