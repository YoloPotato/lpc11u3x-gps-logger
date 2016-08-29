

#ifndef _EEPROM_H
#define _EEPROM_H

unsigned int EEPROM_Write(uint8_t AddressToWrite, void* DataArray, uint8_t BytesToWrite);
unsigned int EEPROM_Read(uint8_t AddressToRead, void* DataArray, uint8_t BytesToRead);
uint8_t EEPROM_Test(void);

#endif /* _EEPROM_H */