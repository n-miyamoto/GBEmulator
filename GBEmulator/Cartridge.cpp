#include "Cartridge.h"
#include "Gameboy.h"

Cartridge::Cartridge(uint8_t* rom)
{
	for (int i = 0;i < 16; i++) title[i] = rom[ROM_TITLE_START + i];
	for (int i = 0;i < 4; i++) manufacturer_code[i] = rom[MANUFACTURE_CODE_ADDR + i];
	cgb_flag = rom[CGB_FLAG_ADDR];
	new_licensee_code = (uint16_t)*(rom+NEW_LICENSE_CODE_ADDR);
	sgb_flag = rom[SGB_FLAG_ADDR]; 
	cartridge_type = rom[CARTRIDGE_TYPE_ADDR];

	// calc ROM bank size (each 16KB)
	rom_size_id = rom[ROM_SIZE_ADDR];
	if (rom_size_id == 0x00) {
		rom_size_banknum = 0;
	}
	else if (rom_size_id < 0x08) {
		rom_size_banknum = 1 << (rom_size_id + 1);
	}
	else if (rom_size_id == 0x52) rom_size_banknum = 72;
	else if (rom_size_id == 0x53) rom_size_banknum = 80;
	else if (rom_size_id == 0x54) rom_size_banknum = 96;

	// calc ROM bank size (each 8KB)
	ram_size_id = rom[RAM_SIZE_ADDR];
	if (ram_size_id < 0x03) ram_size_banknum = 0;
	else if (ram_size_id == 0x03) ram_size_banknum = 0;
	else if (ram_size_id == 0x04) ram_size_banknum = 16;
	else if (ram_size_id == 0x05) ram_size_banknum = 8;

	old_license_code = rom[OLD_LICENSE_CODE_ADDR];
	destination_code = rom[DESTINATION_CODE_ADDR]; 
	mask_rom_version = rom[MASK_ROM_VER_ADDR];
	header_checksum = rom[HEADER_CHECKSUM_ADDR];
	global_checksum = rom[GLOBAL_CHECKSUM_ADDR];
}
