#pragma once // include guard

// ============================================================
// КОНФІГУРАЦІЯ WI-FI
// ============================================================

#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"


// ============================================================
// AWS IoT Core
// ============================================================

#define AWS_IOT_ENDPOINT "YOUR_AWS_IOT_ENDPOINT"


// ============================================================
// AMAZON ROOT CA
// ============================================================

static const char AWS_CERT_CA[] = R"EOF(
PASTE_AMAZON_ROOT_CA_HERE
)EOF";


// ============================================================
// DEVICE CERTIFICATE
// ============================================================

static const char AWS_CERT_CRT[] = R"EOF(
PASTE_DEVICE_CERTIFICATE_HERE
)EOF";


// ============================================================
// DEVICE PRIVATE KEY
// ============================================================

static const char AWS_CERT_PRIVATE[] = R"EOF(
PASTE_DEVICE_PRIVATE_KEY_HERE
)EOF";