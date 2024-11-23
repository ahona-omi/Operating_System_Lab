#include<kcrc.h>

/** 
  * @brief CRC calculation unit 
  */
#define CRC_BASE	(AHB1PERIPH_BASE + 0x3000UL)


typedef struct
{
  volatile uint32_t DR;         /*!< CRC Data register,             Address offset: 0x00 */
  volatile uint8_t  IDR;        /*!< CRC Independent data register, Address offset: 0x04 */
  uint8_t       RESERVED0;  /*!< Reserved, 0x05                                      */
  uint16_t      RESERVED1;  /*!< Reserved, 0x06                                      */
  volatile uint32_t CR;         /*!< CRC Control register,          Address offset: 0x08 */
}CRC_TypeDef;

#define  CRC                 ((CRC_TypeDef *) CRC_BASE)
/******************************************************************************/
/*                                                                            */
/*                          CRC calculation unit                              */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for CRC_DR register  *********************/
#define CRC_DR_DR_Pos       (0U)                                               
#define CRC_DR_DR_Msk       (0xFFFFFFFFUL << CRC_DR_DR_Pos)                     /*!< 0xFFFFFFFF */
#define CRC_DR_DR           CRC_DR_DR_Msk                                      /*!< Data register bits */


/*******************  Bit definition for CRC_IDR register  ********************/
#define CRC_IDR_IDR_Pos     (0U)                                               
#define CRC_IDR_IDR_Msk     (0xFFUL << CRC_IDR_IDR_Pos)                         /*!< 0x000000FF */
#define CRC_IDR_IDR         CRC_IDR_IDR_Msk                                    /*!< General-purpose 8-bit data register bits */


/********************  Bit definition for CRC_CR register  ********************/
#define CRC_CR_RESET_Pos    (0U)                                               
#define CRC_CR_RESET_Msk    (0x1UL << CRC_CR_RESET_Pos)                         /*!< 0x00000001 */
#define CRC_CR_RESET        CRC_CR_RESET_Msk                                   /*!< RESET bit */

void CRC32_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    CRC->CR |= CRC_CR_RESET;
}

void CRC32_Process(uint32_t data)
{
    CRC->DR = data;
}

uint32_t CRC32_Finalize(uint32_t crc_data)
{
    return CRC->DR ^ crc_data;
}

uint32_t CRC32_get(void)
{
    return CRC->DR;
}