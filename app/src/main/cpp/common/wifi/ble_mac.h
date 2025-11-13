#ifndef __BLE_MAC_H__
#define __BLE_MAC_H__

int ble_save_mac_address(const char *filename, const unsigned char *macHex);
int ble_read_mac_address(const char *filename, unsigned char *macHex, unsigned char *macStr);

#endif