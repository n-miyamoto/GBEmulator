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


#define LCD_VERT_LINES		(154)
#define LCD_LINE_CYCLES     (456)

#define HI (1)
#define LO (0)

typedef union registor {
	uint16_t b16;
	uint8_t b8[2];
}reg;

enum class INTERRUPTS{
	V_BLANK = 0,
	LCD_STAT,
	TIMER
};

class CPU
{
private:
	//general registors
	uint8_t RA;
	reg RBC;
	reg RDE;
	reg RHL;
	uint16_t SP;
	uint16_t PC;
	uint8_t* memory_map;
	//flags
	uint8_t FZ;
	uint8_t FH;
	uint8_t FN;
	uint8_t FC;
	//interrupts
	uint8_t IME;
	uint8_t IE;
	uint8_t IF;
	
	void shift_operation_CB();

	uint64_t cycle_count;
	uint32_t lcd_count;
public:
	CPU();
	void set_memmap(uint8_t* memmap);
	void step();
	void set_interrupt_flag(INTERRUPTS intrpt);
	void dump_reg(void);
	bool ready_for_render;
};

class GPU {
private:
	uint8_t* memory_map;
	uint8_t frame_width;
	uint8_t frame_height;
	void rasterize_tile(uint8_t tile_x, uint8_t tile_y, uint8_t tilenum);
public:
	GPU();
	~GPU();
	void set_memmap(uint8_t* memmap);
	uint8_t* total_frame;
	uint8_t* frame_buffer;
	void draw_frame();
};

class Gameboy
{
private:
	uint8_t* memory_map;
	uint8_t* rom_ptr;
	size_t  rom_size;
	uint8_t* save_ptr;
	size_t save_size;

public:
	CPU cpu;
	GPU gpu;
	Gameboy(uint8_t* rom, size_t size, uint8_t* boot_rom);
	~Gameboy();
	void showTitle();
	void showCartInfo();
	void stepCPU();
	bool isRendered;
};

