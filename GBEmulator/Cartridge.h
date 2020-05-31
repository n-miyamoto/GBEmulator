#pragma once
#include <cstdint>

class Cartridge
{
public:
	Cartridge(uint8_t* rom);
	char title[16];
	char manufacturer_code[4];
	uint8_t cgb_flag;
	uint16_t new_licensee_code;
	uint8_t sgb_flag;
	uint8_t cartridge_type;
	uint8_t rom_size_id;
	uint8_t rom_size_bytes;
	uint16_t rom_size_banknum;
	uint8_t ram_size_id;
	uint8_t ram_size_bytes;
	uint16_t ram_size_banknum;
	uint8_t old_license_code;
	uint8_t destination_code;
	uint8_t mask_rom_version;
	uint8_t header_checksum;
	uint16_t global_checksum;
};

