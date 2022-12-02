#pragma once

// TODO: Put in config
// This is terrible to have the keys just exposed here. lol.
#define MOSS_DEFAULT_WIFI_SSID "Apt 3"
#define MOSS_DEFAULT_WIFI_PASS "augslambo"


int moss_wifi_init();
int moss_wifi_connect_default();