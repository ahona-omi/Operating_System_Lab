#ifndef __FLASH_H
#define __FLASH_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void flash_unlock(void);
void flash_lock(void);
void erase_os_flash(void);
void erase_version_flash(void);
int flash_erased_check(void);
void flash_write(uint8_t* data, uint32_t length, uint32_t start_address);
void flash_read(uint32_t length, uint32_t start_address);
char* get_os_version(uint32_t start_address);

#ifdef __cplusplus
}
#endif
#endif