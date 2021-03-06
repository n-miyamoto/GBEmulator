#pragma once
#include <iostream>
#include <memory>
#include "Cartridge.h"
#include <vector>

#define VBLANK_INTR_ADDR    (0x0040)
#define KEYPAD_INTR_ADDR    (0x0060)
#define ROM_TITLE_START		(0x0134)
#define ROM_TITLE_END		(0x0143)
#define MANUFACTURE_CODE_ADDR	(0x013F)
#define CGB_FLAG_ADDR	(0x0143)
#define NEW_LICENSE_CODE_ADDR	(0x0144)
#define SGB_FLAG_ADDR	(0x0146)
#define CARTRIDGE_TYPE_ADDR	(0x0147)
#define ROM_SIZE_ADDR		(0x0148)
#define RAM_SIZE_ADDR		(0x0149)
#define DESTINATION_CODE_ADDR	(0x014A)
#define OLD_LICENSE_CODE_ADDR	(0x014B)
#define MASK_ROM_VER_ADDR	(0x014C)
#define HEADER_CHECKSUM_ADDR	(0x014D)
#define GLOBAL_CHECKSUM_ADDR	(0x014E)
#define CART_MAX_ADDR		(0x8000)
#define DIV_REGISTER		(0xFF04)
#define KEY_INPUT_ADDRES	(0xFF00)
#define INTERRUPT_FLAG		(0xFF0F)
#define LCDC			    (0xFF40)
#define LCDC_Y_CORDINATE    (0xFF44)
#define LCD_SCROLL_Y        (0xFF42)
#define LCD_SCROLL_X        (0xFF43)
#define DMA_OP_ADDRESS		(0xFF46)
#define INTERRUPT_ENABLE	(0xFFFF)
#define MAX_ADDRESS			(0x10000)

#define BOOTROM_SIZE		(0x100)

#define LCD_VERT_LINES		(154)
#define LCD_LINE_CYCLES     (456)

#define HI (1)
#define LO (0)

#define FRAME_WIDTH  (160)
#define FRAME_HEIGHT (144)

#define CLOCK_FREQUENCY (4000000)
#define DIV_COUNTER_INCREMENT_FREQUENCY (16384)

typedef union registor {
	uint16_t b16;
	uint8_t b8[2];
}reg;

enum class INTERRUPTS{
	V_BLANK = 0,
	LCD_STAT,
	TIMER,
	SERIAL,
	KEYPAD
};

enum class KEYS {
	BUTTON_A,
	BUTTON_B,
	BUTTON_SELECT,
	BUTTON_START,
	DIRECTION_R,
	DIRECTION_L,
	DIRECTION_U,
	DIRECTION_D,
	KEY_NUMS,
	NOT_KEY
};

class Memory
{
private:
	//uint8_t map[MAX_ADDRESS];
	std::vector<uint8_t> map;
	//uint8_t boot_rom[BOOTROM_SIZE];
	std::vector<uint8_t> boot_rom;

	void dma_operation(uint8_t src);
	uint8_t memory_bank = 0;
	std::vector<std::unique_ptr<uint8_t[]> > rom_banks = {};
	const uint8_t memory_bank_size = 0;
public:
	Memory(Cartridge &cart, uint8_t* rom, size_t rom_size,  uint8_t* bootrom);
	uint8_t key = 0xFF;
	void write(uint16_t address, uint8_t data);
	uint8_t read(uint16_t address);
	bool is_booting = true;
};

class CPU
{
private:
	void shift_operation_CB();
	Memory* memory = nullptr;
	//general registors
	uint8_t RA = 0;
	reg RBC = { 0 };
	reg RDE = { 0 };
	reg RHL = { 0 };
	uint16_t SP = { 0 };
	uint16_t PC = { 0 };
	//flags
	uint8_t FZ = { 0 };
	uint8_t FH = { 0 };
	uint8_t FN = { 0 };
	uint8_t FC = { 0 };
	//interrupts
	uint8_t IME = { 0 };
	uint8_t HALT = { 0 };
	uint64_t cycle_count = 0;
	uint32_t lcd_count = 0;
	uint32_t div_count = 0;
public:
	void set_memmap(Memory* mem);
	void step();
	void set_interrupt_flag(INTERRUPTS intrpt);
	void dump_reg(void);
	bool ready_for_render = false;
	
};

class GPU {
private:
	Memory* memory=nullptr;
	uint8_t frame_width = FRAME_WIDTH;
	uint8_t frame_height = FRAME_HEIGHT;
	std::unique_ptr<uint8_t[]> total_frame = nullptr;
public:
	GPU();
	void set_memmap(Memory* memory);
	void draw_frame();
	std::unique_ptr<uint8_t[]> frame_buffer = nullptr;
};

class Gameboy
{
private:
	Cartridge cartridge;
	std::unique_ptr<Memory> memory;
	uint8_t* rom_ptr = nullptr;
	size_t rom_size = 0; 
	bool key_pressed[static_cast<int>(KEYS::KEY_NUMS)] = {0};

public:
	Gameboy(uint8_t* rom, size_t size, uint8_t* boot_rom);
	CPU cpu;
	GPU gpu;
	void show_title();
	void show_cart_info();
	bool isRendered = false;
	void press(KEYS key);
	void release(KEYS key);
};

