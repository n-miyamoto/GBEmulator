#include "Gameboy.h"
#include <iostream>

uint8_t OP_CYCLES[0x100] = {
	//   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	4,12, 8, 8, 4, 4, 8, 4,20, 8, 8, 8, 4, 4, 8, 4,    // 0x00
	4,12, 8, 8, 4, 4, 8, 4, 8, 8, 8, 8, 4, 4, 8, 4,    // 0x10
	8,12, 8, 8, 4, 4, 8, 4, 8, 8, 8, 8, 4, 4, 8, 4,    // 0x20
	8,12, 8, 8,12,12,12, 4, 8, 8, 8, 8, 4, 4, 8, 4,    // 0x30
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,    // 0x40
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,    // 0x50
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,    // 0x60
	8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4,    // 0x70
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,    // 0x80
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,    // 0x90
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,    // 0xA0
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,    // 0xB0
	8,12,12,12,12,16, 8,32, 8, 8,12, 8,12,12, 8,32,    // 0xC0
	8,12,12, 0,12,16, 8,32, 8, 8,12, 0,12, 0, 8,32,    // 0xD0
	12,12, 8, 0, 0,16, 8,32,16, 4,16, 0, 0, 0, 8,32,    // 0xE0
	12,12, 8, 4, 0,16, 8,32,12, 8,16, 4, 0, 0, 8,32     // 0xF0
};


Gameboy::Gameboy(uint8_t* rom, size_t size, uint8_t* boot_rom) {
	isRendered = false;
	rom_ptr = rom;
	rom_size = size;
	uint32_t ram_sizes[] = { 0x00, 0x800, 0x2000, 0x8000, 0x20000 };
	save_size = ram_sizes[rom[ROM_RAM_SIZE]];
	save_ptr = (uint8_t*)malloc(save_size);
	memory_map = (uint8_t*)malloc(sizeof(uint8_t) * 0x10000);
	memset(memory_map, 0, 0x10000);
	memcpy(memory_map, rom, size);
	memcpy(memory_map, boot_rom, 0x100);
	cpu.set_memmap(memory_map);
	gpu.set_memmap(memory_map);
}

Gameboy::~Gameboy() {
	free(memory_map);
}

void Gameboy::stepCPU(void) {
	isRendered = true;
}

void Gameboy::showTitle() {
	char str[ROM_TITLE_END - ROM_TITLE_START + 1];
	str[ROM_TITLE_END - ROM_TITLE_START] = -1;
	memcpy(str, memory_map + ROM_TITLE_START, ROM_TITLE_END - ROM_TITLE_START + 1);
	std::cout << str << std::endl;
}

void Gameboy::showCartInfo() {
	std::cout << "TITLE: ";
	showTitle();
	std::cout << "ROM SIZE: " << rom_size << std::endl;
}

void CPU::shift_operation_CB() {
	uint8_t op = memory_map[PC++];
	uint8_t R  = (op & 0x7);
	uint8_t B  = (op >> 3) & 0x7;
	uint8_t D  = (op >> 3) & 0x1;
	uint8_t val;

	// retrieve byte to manipulate
	switch (R)
	{
		case 0: val = RBC.b8[HI]; break;
		case 1: val = RBC.b8[LO]; break;
		case 2: val = RDE.b8[HI]; break;
		case 3: val = RDE.b8[LO]; break;
		case 4: val = RHL.b8[HI]; break;
		case 5: val = RHL.b8[LO]; break;
		case 6: val = memory_map[RHL.b16]; break;
		case 7: val = RA; break;
	}

	// bit-fiddling OPs
	uint8_t writeback = 1;
	uint8_t N;
	switch (op >> 6)
	{
		case 0x0:
			op = (op >> 4) & 0x3;
			switch (op)
			{
				case 0x0: // RdC R
				case 0x1: // Rd R
					if (D) // RRC R / RR R
					{
						N = val;
						val = (val >> 1);
						val |= op ? (FC << 7) : (N << 7);
						FZ = (val == 0x00);
						FN = 0;
						FH = 0;
						FC = (N & 0x01);
					}
					else    // RLC R / RL R
					{
						N = val;
						val = (val << 1);
						val |= op ? FC : (N >> 7);
						FZ = (val == 0x00);
						FN = 0;
						FH = 0;
						FC = (N >> 7);
					}
					break;
				case 0x2:
					if (D) // SRA R
					{
						FC = val & 0x01;
						val = (val >> 1) | (val & 0x80);
						FZ = (val == 0x00);
						FN = 0;
						FH = 0;
					}
					else    // SLA R
					{
						FC = (val >> 7);
						val = val << 1;
						FZ = (val == 0x00);
						FN = 0;
						FH = 0;
					}
					break;
				case 0x3:
					if (D) // SRL R
					{
						FC = val & 0x01;
						val = val >> 1;
						FZ = (val == 0x00);
						FN = 0;
						FH = 0;
					}
					else    // SWAP R
					{
						N = (val >> 4) & 0x0F;
						N |= (val << 4) & 0xF0;
						val = N;
						FZ = (val == 0);
						FN = 0;
						FH = 0;
						FC = 0;
					}
					break;
			}
			break;
		case 0x1: // BIT B, R
			FZ = !((val >> B) & 0x1);
			FN = 0;
			FH = 1;
			writeback = 0;
			break;
		case 0x2: // RES B, R
			val &= (0xFE << B) | (0xFF >> (8 - B));
			break;
		case 0x3: // SET B, R
			val |= (0x1 << B);
			break;
	}

	// save result
	if (writeback)
	{
		switch (R)
		{
			case 0: RBC.b8[HI] = val; break;
			case 1: RBC.b8[LO] = val; break;
			case 2: RDE.b8[HI] = val; break;
			case 3: RDE.b8[LO] = val; break;
			case 4: RHL.b8[HI] = val; break;
			case 5: RHL.b8[LO] = val; break;
			case 6: memory_map[RHL.b16]= val; break;
			case 7: RA = val; break;
		}
	}
}

CPU::CPU() {
#if 0
	RA = 1;
	RBC.b8[HI] = 0;
	RBC.b8[LO] = 0x13;
	RDE.b8[HI] = 0;
	RDE.b8[LO] = 0xD8;
	RHL.b8[HI] = 0x01;
	RHL.b8[LO] = 0x4D;
	SP= 0xFFFE;
	PC= 0x100;
	FZ = 1;
	FH = 1;
	FN = 0;
	FC = 1;
	IME = 1;
	IF = 0;
#endif
	PC = 0;
	cycle_count = 0;
	lcd_count = 0;
	ready_for_render = false;
}

void CPU::set_memmap(uint8_t* memmap) {
	memory_map = memmap;
}

void CPU::step() 
{

	if (ready_for_render) return;

	//check interrupt
	if (IME && (IF > 0)) {
		IME = 0;
		std::cout << "interrupt ocured \n";
		memory_map[--SP] = PC >> 8;
		memory_map[--SP] = PC & 0xFF;
		if (IF & 1<<static_cast<uint8_t>(INTERRUPTS::V_BLANK)) {
			PC = VBLANK_INTR_ADDR;
			IF ^= 1 << static_cast<uint8_t>(INTERRUPTS::V_BLANK);
		}
	}

	if (PC == 0x100) {
		std::cout << "finish boot seqence\n";
		uint8_t *ptr1 = &memory_map[0x8000];
		uint8_t *ptr2 = &memory_map[0x8800];
		uint8_t *ptr3 = &memory_map[0x9000];
		uint8_t *ptr4 = &memory_map[0x9800];
		uint8_t *ptr5 = &memory_map[0xFE00];

	}
	//fetch
	//dump_reg();
	uint16_t tmp = PC;
	uint8_t op = memory_map[PC++];
	uint16_t NN;
	uint8_t N;
	int8_t SN;
	uint8_t D1, D2;

	switch (op) {
	case 0x00: //nop
		break;
	case 0x01:
		RBC.b8[LO] = memory_map[PC++];
		RBC.b8[HI] = memory_map[PC++];
		break;
	case 0x02:
		memory_map[RBC.b16] = RA;
		break;
	case 0x04:
		RBC.b8[HI]++;
		FZ = (RBC.b8[HI] == 0x00);
		FN = 0;
		FH = ((RBC.b8[HI] & 0x0F) == 0x00);
		break;
	case 0x05:
		RBC.b8[HI]--;
		FZ = (RBC.b8[HI] == 0);
		FN = 1;
		FH = ((RBC.b8[HI] & 0x0F) == 0);
		break;
	case 0x06:
		RBC.b8[HI] = memory_map[PC++]; //B
		break;
	case 0x0C:
		RBC.b8[LO]++;
		FZ = (RBC.b8[LO] == 0x00);
		FN = 0;
		FH = ((RBC.b8[LO] & 0x0F) == 0x00);
		break;
	case 0x0D:
		RBC.b8[LO]--;
		FZ = (RBC.b8[LO] == 0);
		FN = 1;
		FH = ((RBC.b8[LO] & 0x0F) == 0x0F);
		break;
	case 0x0E:
		RBC.b8[LO] = memory_map[PC++];
		break;
	case 0x11:
		RDE.b8[LO] = memory_map[PC++];
		RDE.b8[HI] = memory_map[PC++];
		break;
	case 0x13:
		NN = RDE.b16 + 1;
		RDE.b8[HI] = NN >> 8;
		RDE.b8[LO] = NN & 0xFF;
		break;
	case 0x15:
		RDE.b8[HI]--;
		FZ = (RDE.b8[HI] == 0x00);
		FN = 1;
		FH = ((RDE.b8[HI] & 0x0F) == 0x0F);
		break;
	case 0x16:
		RDE.b8[HI] = memory_map[PC++];
		break;
	case 0x17:
		N = RA;
		RA = RA << 1 | FC;
		FZ = 0;
		FN = 0;
		FH = 0;
		FC = (N >> 7) & 0x01;
		break;
	case 0x18:
		SN = (int8_t)memory_map[PC++];
		PC += SN;
		break;
	case 0x1A:
		RA = memory_map[RDE.b16];
		break;
	case 0x1D:
		RDE.b8[LO]--;
		FZ = (RDE.b8[LO] == 0x00);
		FN = 1;
		FH = ((RDE.b8[LO] & 0x0F) == 0x0F);
		break;
	case 0x1E:
		RDE.b8[LO] = memory_map[PC++];
		break;
	case 0x20:
		SN = (int8_t)memory_map[PC++];
		if (!FZ) PC += SN;
		break;
	case 0x21:
		RHL.b8[LO] = memory_map[PC++];
		RHL.b8[HI] = memory_map[PC++];
		break;
	case 0x22:
		memory_map[RHL.b16] = RA;
		NN = RHL.b16 + 1;
		RHL.b8[HI] = NN >> 8;
		RHL.b8[LO] = NN & 0xFF;
		break;
	case 0x23:
		NN = RHL.b16 + 1;
		RHL.b8[HI] = NN >> 8;
		RHL.b8[LO] = NN & 0xFF;
		break;
	case 0x24:
		RHL.b8[HI]++;
		FZ = (RHL.b8[HI] == 0x00);
		FN = 0;
		FH = ((RHL.b8[HI] & 0x0F) == 0x00);
		break;
	case 0x27:
		D1 = RA >> 4;
		D2 = RA & 0x0F;
		if (FN)
		{
			if (FH)
			{
				D2 -= 6;
			}
			if (FC)
			{
				D1 -= 6;
			}
			if (D2 > 9)
			{
				D2 -= 6;
			}
			if (D1 > 9)
			{
				D1 -= 6;
				FC = 1;
			}
		}
		break;
	case 0x28:
		SN = (int8_t)memory_map[PC++];
		if (FZ) PC += SN;
		break;
	case 0x2E:
		RHL.b8[LO] = memory_map[PC++];
		break;
	case 0x31:
		SP = memory_map[PC++];
		SP |= memory_map[PC++] << 8;
		break;
	case 0x32:
		memory_map[RHL.b16] = RA;
		NN = RHL.b16 - 1;
		RHL.b8[HI] = NN >> 8;
		RHL.b8[LO] = NN & 0xFF;
		break;
	case 0x3C:
		RA++;
		FZ = (RA == 0x00);
		FN = 0;
		FH = ((RA & 0x0F) == 0x00);
		break;
	case 0x3D:
		RA--;
		FZ = (RA == 0x00);
		FN = 1;
		FH = ((RA & 0x0F) == 0x0F);
		break;
	case 0x3E:
		RA = memory_map[PC++];
		break;
	case 0x3F:
		FN = 0;
		FH = 0;
		FC = FC ^ 0x1;
		break;
	case 0x47:
		RBC.b8[HI] = RA;
		break;
	case 0x4F:
		RBC.b8[LO] = RA;
		break;
	case 0x57:
		RDE.b8[HI] = RA;
		break;
	case 0x58:
		RDE.b8[LO] = RBC.b8[HI];
		break;
	case 0x59:
		RDE.b8[LO] = RDE.b8[HI];
		break;
	case 0x66:
		RHL.b8[HI] = memory_map[RHL.b16];
		break;
	case 0x67:
		RHL.b8[HI] = RA;
		break;
	case 0x77:
		memory_map[RHL.b16] = RA;
		break;
	case 0x78:
		RA = RBC.b8[HI];
		break;
	case 0x7B:
		RA = RDE.b8[LO];
		break;
	case 0x7C:
		RA = RHL.b8[HI];
		break;
	case 0x7D:
		RA = RHL.b8[LO];
		break;
	case 0x7E:
		RA = memory_map[RHL.b16];
		break;
	case 0x86:
		N = memory_map[RHL.b16];
		NN = RA + N;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x90:
		NN = RA - RBC.b8[HI];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ RBC.b8[HI] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0xAF:
		RA = 0x00;
		FZ = 1;
		FN = FH = FC = 0;
		break;
	case 0xA7:
		FZ = (RA == 0x00);
		FN = 0;
		FH = 1;
		FC = 0;
		break;
	case 0xB7:
		FZ = (RA == 0x00);
		FN = 0;
		FH = 0;
		FC = 0;
		break;
	case 0xBE:
		N = memory_map[RHL.b16];
		NN = RA - N;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		break;
	case 0xC0:
		if (!FZ)
		{
			NN = memory_map[SP++];
			NN |= memory_map[SP++] << 8;
			PC = NN;
		}
		break;
	case 0xC1:
		RBC.b8[LO] = memory_map[SP++];
		RBC.b8[HI] = memory_map[SP++];
		break;
	case 0xC3:
		NN = memory_map[PC++];
		NN |= memory_map[PC++] << 8;
		PC = NN;
		break;
	case 0xC5:
		memory_map[--SP] = RBC.b8[HI];
		memory_map[--SP] = RBC.b8[LO];
		break;
	case 0xC8:
		if (FZ)
		{
			NN = memory_map[SP++];
			NN |= memory_map[SP++] << 8;
			PC = NN;
		}
		break;
	case 0xC9:
		NN = memory_map[SP++];
		NN |= memory_map[SP++] << 8;
		PC = NN;
		break;
	case 0xCB:
		shift_operation_CB();
		break;
	case 0xCD:
		NN = memory_map[PC++];
		NN |= memory_map[PC++] << 8;
		memory_map[--SP] = PC >> 8;
		memory_map[--SP] = PC & 0xFF;
		PC = NN;
		break;
	case 0xCE:
		N = memory_map[PC++];
		NN = RA + N + FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0xD5:
		memory_map[--SP] = RDE.b8[HI];
		memory_map[--SP] = RDE.b8[LO];
		break;
	case 0xE0:
		memory_map[(0xFF00 | memory_map[PC++])] = RA;
		break;
	case 0xEA:
		memory_map[memory_map[PC++]|memory_map[PC++]<<8] = RA;
		break;
	case 0xF0:
		RA = memory_map[(0xFF00 | memory_map[PC++])];
		break;
	case 0xF3:
		IME = 0;
		break;
	case 0xE2:
		memory_map[0xFF | RBC.b8[LO]] = RA;
		break;
	case 0xE5:
		memory_map[--SP] = RHL.b8[HI];
		memory_map[--SP] = RHL.b8[LO];
		break;
	case 0xE6:
		RA = RA & memory_map[PC++];
		FZ = (RA == 0x00);
		FN = 0;
		FH = 1;
		FC = 0;
		break;
	case 0xF5:
		memory_map[--SP] = RA;
		memory_map[--SP] = FZ<<7|FN<<6|FH<<5|FC<<4;
		break;
	case 0xF9:
		SP = RHL.b16;
		break;
	case 0xFA:
		RA = memory_map[memory_map[PC++] | memory_map[PC++] << 8];
		break;
	case 0xFE:
		N = memory_map[PC++];
		NN = RA - N;
		FZ = ((NN & 0xFF) == 0);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		break;
	default:
		std::cout << "Operation not inplemented : " << std::hex << (int)op << std::endl;//todo for debug
		dump_reg();
		while (1);
		break;
	}

	cycle_count += OP_CYCLES[op];
	lcd_count += OP_CYCLES[op];
	if (lcd_count > LCD_LINE_CYCLES) {
		memory_map[LCDC_Y_CORDINATE] = (memory_map[LCDC_Y_CORDINATE] + 1) % LCD_VERT_LINES;
		lcd_count -= LCD_LINE_CYCLES;


		if (memory_map[LCDC_Y_CORDINATE] == 144) {
			ready_for_render = true;
			//std::cout << "vertical " << std::endl;
		}
	}

}
void CPU::set_interrupt_flag(INTERRUPTS intrpt) {
	IF |= 1 << static_cast<uint8_t>(intrpt);
}

void CPU::dump_reg() {
	int op = memory_map[PC];
	std::cout << std::hex << "PC " << (int)PC << " " << 
		         std::hex << "OP " <<(int)op << " " << 
				 std::hex << "RA " <<(int)RA << " " <<
				 std::hex << "RB " <<(int)RBC.b8[HI] << " " <<
				 std::hex << "RC " <<(int)RBC.b8[LO] << " " <<
				 std::hex << "RD " <<(int)RDE.b8[HI] << " " <<
				 std::hex << "RE " <<(int)RDE.b8[LO] << " " <<
				 std::hex << "RH " <<(int)RHL.b8[HI] << " " <<
				 std::hex << "RL " <<(int)RHL.b8[LO] << " " << std::endl;
}


GPU::GPU() {
	frame_height = 144;
	frame_width = 160;
	frame_buffer = (uint8_t*)malloc(sizeof(uint8_t)*frame_width*frame_height);
	total_frame= (uint8_t*)malloc(sizeof(uint8_t)*256*256);
	memset(frame_buffer, 0, frame_height * frame_width);
	memset(total_frame, 0, 256 * 256);
}
GPU::~GPU() {
	free(frame_buffer);
}

void GPU::set_memmap(uint8_t* memmap) {
	memory_map = memmap;
}

void GPU::rasterize_tile(uint8_t tile_x, uint8_t tile_y, uint8_t tilenum) {

}

void GPU::draw_frame() {
	auto display_enable = (memory_map[LCDC] >> 7) & 0x01;
	if (!display_enable) return;

	auto window_tilemap_select = (memory_map[LCDC] >> 6) & 0x01;
	auto window_display = (memory_map[LCDC] >> 5) & 0x01;
	auto tiledata_select= (memory_map[LCDC] >> 4) & 0x01;
	auto bg_tilemap_select = (memory_map[LCDC] >> 3) & 0x01;
	auto obj_size = (memory_map[LCDC] >> 2) & 0x01;
	auto obj_display_enable = (memory_map[LCDC] >> 1) & 0x01;
	auto bg_display = (memory_map[LCDC] >> 0) & 0x01;



	//draw background
	if (bg_display) {
		auto bg_top_addr = 0x9800 + 0x400 * bg_tilemap_select;
		auto tile_top_addr = 0x9000 - 0x1000 * tiledata_select;
		for (int i = 0; i < 32; i++) {
			for (int j = 0; j < 32; j++) {
				auto tilenum = memory_map[bg_top_addr + i * 32 + j];
				if (tilenum != 0) {
					int kads = 0;
				}
				uint16_t tile_id;
				if (!tiledata_select) tile_id = (int8_t)tilenum;
				else tile_id = tilenum;
				auto tile_addr = tile_top_addr + 16 * tile_id;
				for (int k = 0; k < 8; k++) { //1 tile has 16 byte 8px * 8line 
					uint8_t b1 = memory_map[tile_addr++];
					uint8_t b2 = memory_map[tile_addr++];
					for (int l = 0; l < 8; l++) {
						auto px = ((b2 >> (7 - l)) & 0x01);
						px += ((b1 >> (7 - l)) & 0x01)<<1;
						auto y = 8 * i + k;
						auto x = 8 * j + l;
						total_frame[y * 256 + x] = px;
					}
				}
			}
		}
	}
	else {
		for (int i = 0; i < 256 * 256; i++) frame_buffer[i] = 0;//clear white
	}
	
#if 0
	if (window_display) {
		//draw window
		auto window_top_addr = 0x9800 + 0x400 * window_tilemap_select;
		auto tile_top_addr = 0x9000 - 0x1000 * tiledata_select;
		for (int i = 0; i < 32; i++) {
			for (int j = 0; j < 32; j++) {
				auto tilenum = memory_map[window_top_addr + i * 32 + j];
				uint16_t tile_id;
				if (!tiledata_select) tile_id = (int8_t)tilenum;
				else tile_id = tilenum;
				auto tile_addr = tile_top_addr + 16 * tile_id;
				for (int k = 0; k < 8; k++) { //1 tile has 16 byte 8px * 8line 
					uint8_t b1 = memory_map[tile_addr++];
					uint8_t b2 = memory_map[tile_addr++];
					for (int l = 0; l < 8; l++) {
						auto px = ((b2 >> (7 - l)) & 0x01) << 7;
						px += ((b1 >> (7 - l)) & 0x01) << 6;
						auto x = 8 * i + k;
						auto y = 8 * j + l;
						if(px!=0) total_frame[y * 256 + x] = px;
					}
				}
			}
		}
	}


	if (obj_display_enable) {
		//draw obj
		auto obj_top_addr = 0xFE00;
		auto obj_addr = obj_top_addr;
		auto tile_top_addr = 0x8000;
		for (int i = 0; i < 40; i++) {//40 objects 
			auto top = memory_map[obj_addr++];
			auto left= memory_map[obj_addr++];
			auto tile_id = memory_map[obj_addr++];
			auto flags = memory_map[obj_addr++];
			auto tile_addr = tile_top_addr + 16 * tile_id;
			
			for (int k = 0; k < 8; k++) { //1 tile has 16 byte 8px * 8line 
				uint8_t b1 = memory_map[tile_addr++];
				uint8_t b2 = memory_map[tile_addr++];
				for (int l = 0; l < 8; l++) {
					auto px = ((b2 >> (7 - l)) & 0x01) << 7;
					px += ((b1 >> (7 - l)) & 0x01) << 6;
					auto x = left + k;
					auto y = top + l;
					if (px != 0) total_frame[y * 256 + x] = px;
				}
			}

		}
	}	
#endif	
	//draw framebuffer
	auto scroll_y = memory_map[LCD_SCROLL_Y];
	auto scroll_x = memory_map[LCD_SCROLL_X];

	for (int y = 0; y < frame_height; y++) {
		for (int x = 0; x < frame_width; x++) {
			if (scroll_x + x < frame_width && scroll_y + y < frame_height) {
				frame_buffer[y * frame_width + x] = total_frame[(scroll_y + y) * 256 + (scroll_x + x)];
				//if (total_frame[(scroll_y + y) * 256 + (scroll_x + x)] != 0) std::cout << " ";
			}
			else {
					frame_buffer[y * frame_width + x] = 0;
			}
		}
	}
}

