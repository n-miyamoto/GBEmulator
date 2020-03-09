#pragma once
#include <iostream>

#define VBLANK_INTR_ADDR    (0x0040)
#define ROM_TITLE_START		(0x0134)
#define ROM_TITLE_END		(0x0143)
#define ROM_RAM_SIZE		(0x0149)
#define LCDC			    (0xFF40)
#define LCDC_Y_CORDINATE    (0xFF44)
#define LCD_SCROLL_Y        (0xFF42)
#define LCD_SCROLL_X        (0xFF43)
#define MAX_ADDRESS			(0x10000)

#define BOOTROM_SIZE		(0x100)

#define LCD_VERT_LINES		(154)
#define LCD_LINE_CYCLES     (456)

#define HI (1)
#define LO (0)

#define FRAME_WIDTH  (160);
#define FRAME_HEIGHT (144);

typedef union registor {
	uint16_t b16;
	uint8_t b8[2];
}reg;

enum class INTERRUPTS{
	V_BLANK = 0,
	LCD_STAT,
	TIMER
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
	KEY_NUMS
};

class Memory
{
private:
	uint8_t map[MAX_ADDRESS];
	uint8_t boot_rom[BOOTROM_SIZE];
	void dma_operation(uint8_t src);
public:
	uint8_t key = 0xFF;
	Memory(uint8_t* cart, size_t rom_size,  uint8_t* bootrom);
	void write(uint16_t address, uint8_t data);
	uint8_t read(uint16_t address);
	bool is_booting = true;
};

class CPU
{
private:
	void shift_operation_CB();
	Memory* memory;
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
	uint8_t IE = { 0 };
	uint8_t IF = { 0 };
	uint64_t cycle_count = 0;
	uint32_t lcd_count = 0;
public:
	CPU();
	void set_memmap(Memory* mem);
	void step();
	void set_interrupt_flag(INTERRUPTS intrpt);
	void dump_reg(void);
	bool ready_for_render = false;
	
};

class GPU {
private:
	Memory* memory;
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
	std::unique_ptr<Memory> memory;
	uint8_t* memory_map = nullptr;
	uint8_t* rom_ptr = nullptr;
	size_t  rom_size = 0; 
	uint8_t* save_ptr = nullptr;
	size_t save_size = 0;
	bool key_pressed[static_cast<int>(KEYS::KEY_NUMS)] = {0};

public:
	CPU cpu;
	GPU gpu;
	Gameboy(uint8_t* rom, size_t size, uint8_t* boot_rom);
	~Gameboy();
	void showTitle();
	void showCartInfo();
	void stepCPU();
	bool isRendered;
	void press(KEYS key);
	void release(KEYS key);
};

