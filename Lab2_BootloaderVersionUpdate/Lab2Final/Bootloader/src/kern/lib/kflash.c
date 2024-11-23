#include <kflash.h>
#include <cm4.h>
#include <sys_usart.h>
#include <kstdio.h>

#define FLASH_KEY1 0x45670123
#define FLASH_KEY2 0xCDEF89AB
#define FLASH_BASE_ADDRESS          (0x08000000U)
#define BOOTLOADER_SIZE             (0x00010000U) // 64 KB
#define OS_START_ADDRESS    (FLASH_BASE_ADDRESS + BOOTLOADER_SIZE) // 0X08010000
char version[100]="0.0";


void flash_unlock(void){
 
    if(FLASH->CR & FLASH_CR_LOCK){
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}
 
 
void flash_lock(void){
    FLASH->CR |= FLASH_CR_LOCK;
}

void erase_os_flash(){
 
    /*
    sector 4: 0x0801 0000 - 0x0801 FFFF length= 64KB
    sector 5: 0x0802 0000 - 0x0803 FFFF length= 128KB
    sector 6: 0x0804 0000 - 0x0805 FFFF length= 128KB
    */
 
    flash_unlock();
 
    for(uint8_t sector=0x4; sector<=0x6; sector++){
 
        while(FLASH->SR & FLASH_SR_BSY); // Wait for the flash to be ready
 
        FLASH->CR |= FLASH_CR_SER; // Sector erase enabled
 
        FLASH->CR &= ~(0xF << 3); // Clear the sector number 
        FLASH->CR |= sector << 3;   //select the sector to erase in hex
 
        FLASH->CR |= FLASH_CR_STRT; // start the erase operation
 
        while(FLASH->SR & FLASH_SR_BSY);
 
 
    }
 
    flash_lock();
}

void erase_version_flash(){
 
    /*
    sector 7: 0x0806 0000 - 0x0807 FFFF length= 128KB
    */
 
    flash_unlock();
    uint8_t sector=0x7;
 
 
    while(FLASH->SR & FLASH_SR_BSY); // Wait for the flash to be ready
 
    FLASH->CR |= FLASH_CR_SER; // Sector erase enabled
 
    FLASH->CR &= ~(0xF << 3); // Clear the sector number 
    FLASH->CR |= sector << 3;   //select the sector to erase in hex
 
    FLASH->CR |= FLASH_CR_STRT; // start the erase operation
 
    while(FLASH->SR & FLASH_SR_BSY);
 
    flash_lock();
}

int flash_erased_check(void){
 
    int start_address = OS_START_ADDRESS;
    int end_address = OS_START_ADDRESS + 0x40000U;
 
    for(int i=start_address; i<end_address; i+=4){
        if(*(uint32_t*)i != 0xFFFFFFFF){
            return 0;
        }
    }
    return 1;
}

void flash_write(uint8_t* data, uint32_t length, uint32_t start_address) {
    flash_unlock();

    FLASH->CR |= FLASH_CR_PG;  // Enable programming mode for flash

    for (uint32_t i = 0; i < length; i++) {
        // Write one byte at a time to flash memory
        *(uint8_t *)(start_address + i) = data[i];

        // Wait until the flash is not busy
        while (FLASH->SR & FLASH_SR_BSY);

        // Verify the written data
        if (*(uint8_t *)(start_address + i) != data[i]) {
            kprintf("Verification failed at address 0x%x\n", (start_address + i));
            flash_lock();
            return;
        }

        // Check for any errors
        if (FLASH->SR & (FLASH_SR_WRPERR | FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR)) {
            kprintf("Error writing to flash at address 0x%x\n", (start_address + i));
            FLASH->SR |= (FLASH_SR_WRPERR | FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR);  // Clear error flags
            flash_lock();  // Lock flash after error
            return;
        }
    }

    FLASH->CR &= ~FLASH_CR_PG;  // Disable programming mode after writing
    flash_lock();  // Lock the flash after writing
}

void flash_read(uint32_t length, uint32_t start_address) {
    uint8_t data[length];  // Allocate an array to store the data read from flash
    
    for (uint32_t i = 0; i < length; i++) {
        // Read one byte from flash
        data[i] = *(uint8_t *)(start_address + i);

        // Display the byte in both hexadecimal and character format
        if (data[i] >= 32 && data[i] <= 126) {  // Check if the byte is a printable ASCII character
            kprintf("Data at from flash read  0x%x: 0x%02X ('%c')\n", (start_address + i), data[i], data[i]);
        } else {
            kprintf("Data at from flash read 0x%x: 0x%02X\n", (start_address + i), data[i]);
        }

        // Wait until the flash is not busy
        while (FLASH->SR & FLASH_SR_BSY);
    }
}

char* get_os_version(uint32_t start_address) {
    uint8_t c;
    int j = 0;
    for (uint32_t i = 0; i < 10; i++) {
        // Read one byte from flash
        c = *(uint8_t *)(start_address + i);

        // Display the byte in both hexadecimal and character format
        if ((c >= 48 && c <= 57) || c == 46) {  // Check if the byte is a printable ASCII character
            version[j++] = (char*) c;
        } else {
            // kprintf("Data at from flash read 0x%x: 0x%02X\n", (start_address + i), data[i]);
        }
        // Wait until the flash is not busy
        while (FLASH->SR & FLASH_SR_BSY);
    }
    return version;
}

