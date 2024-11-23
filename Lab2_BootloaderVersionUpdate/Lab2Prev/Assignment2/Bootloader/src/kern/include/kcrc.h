#ifndef __CRC_H
#define __CRC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include<sys_bus_matrix.h>

void CRC32_Init(void);
void CRC32_Process(uint32_t data);
uint32_t CRC32_Finalize(uint32_t crc_data);

#ifdef __cplusplus
}
#endif
#endif