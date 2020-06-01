#include "Gameboy.h"
#include <memory>
#include <iostream>
#include <cstring>
#include <algorithm>

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

Gameboy::Gameboy(uint8_t* rom, size_t size, uint8_t* boot_rom) 
	: cartridge(rom),
	memory(std::make_unique<Memory>(cartridge, rom,size,boot_rom)),
	rom_ptr(rom), 
	rom_size(size)
{
	cpu.set_memmap(memory.get());
	gpu.set_memmap(memory.get());
}

void Gameboy::show_title() {
	char str[ROM_TITLE_END - ROM_TITLE_START + 1];
	str[ROM_TITLE_END - ROM_TITLE_START] = -1;
	for (int a = ROM_TITLE_START; a < ROM_TITLE_END; a++) {
		str[a - ROM_TITLE_START] = memory->read(a);
	}
	std::cout << str << std::endl;
}

void Gameboy::show_cart_info() {
	std::cout << "TITLE: ";
	show_title();
	std::cout << "ROM SIZE: " << rom_size << std::endl;
}

void Gameboy::press(KEYS key) {
	std::cout << "key pressed" << std::endl;
	key_pressed[static_cast<int>(key)] = true;
	memory->key &= ~(1 << static_cast<int>(key));
}
void Gameboy::release(KEYS key){
	std::cout << "key released" << std::endl;
	key_pressed[static_cast<int>(key)] = false;
	memory->key |= 1 << static_cast<int>(key);
}

void CPU::shift_operation_CB() {
	uint8_t op = memory->read(PC++);
	uint8_t R  = (op & 0x7);
	uint8_t B  = (op >> 3) & 0x7;
	uint8_t D  = (op >> 3) & 0x1;
	uint8_t val = 0;

	// retrieve byte to manipulate
	switch (R)
	{
		case 0: val = RBC.b8[HI]; break;
		case 1: val = RBC.b8[LO]; break;
		case 2: val = RDE.b8[HI]; break;
		case 3: val = RDE.b8[LO]; break;
		case 4: val = RHL.b8[HI]; break;
		case 5: val = RHL.b8[LO]; break;
		case 6: val = memory->read(RHL.b16); break;
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
						FN = FH = 0;
						FC = (N & 0x01);
					}
					else    // RLC R / RL R
					{
						N = val;
						val = (val << 1);
						val |= op ? FC : (N >> 7);
						FZ = (val == 0x00);
						FN = FH = 0;
						FC = (N >> 7);
					}
					break;
				case 0x2:
					if (D) // SRA R
					{
						FC = val & 0x01;
						val = (val >> 1) | (val & 0x80);
						FZ = (val == 0x00);
						FN = FH = 0;
					}
					else    // SLA R
					{
						FC = (val >> 7);
						val = val << 1;
						FZ = (val == 0x00);
						FN = FH = 0;
					}
					break;
				case 0x3:
					if (D) // SRL R
					{
						FC = val & 0x01;
						val = val >> 1;
						FZ = (val == 0x00);
						FN = FH = 0;
					}
					else    // SWAP R
					{
						N = (val >> 4) & 0x0F;
						N |= (val << 4) & 0xF0;
						val = N;
						FZ = (val == 0);
						FN = FH = FC = 0;
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
			case 6: memory->write(RHL.b16, val); break;
			case 7: RA = val; break;
		}
	}
}

void CPU::set_memmap(Memory *mem) {
	memory = mem;
}

void CPU::step() 
{
	if (ready_for_render) return;
	if (!IME) HALT = 0;

	// V-blank interrupt
	if (IME&&
		!!(memory->read(INTERRUPT_FLAG) & 1 << static_cast<uint8_t>(INTERRUPTS::V_BLANK)) &&
		!!(memory->read(INTERRUPT_ENABLE) & 1 << static_cast<uint8_t>(INTERRUPTS::V_BLANK))) {
		HALT = 0;
		dump_reg();
		std::cout << "V-blank interrupt\n";
		IME = 0;
		memory->write(--SP, PC >> 8);
		memory->write(--SP, PC & 0xFF);
		PC = VBLANK_INTR_ADDR;
		uint8_t IF = memory->read(INTERRUPT_FLAG) ^ 1 << static_cast<uint8_t>(INTERRUPTS::V_BLANK);
		memory->write(INTERRUPT_FLAG, IF);
	}

	// keypad interrupt
	if (IME&&
		!!(memory->read(INTERRUPT_FLAG) & 1 << static_cast<uint8_t>(INTERRUPTS::KEYPAD)) &&
		!!(memory->read(INTERRUPT_ENABLE) & 1 << static_cast<uint8_t>(INTERRUPTS::KEYPAD))) {
		HALT = 0;
		dump_reg();
		std::cout << "keypad interrupt\n";
		IME = 0;
		memory->write(--SP, PC >> 8);
		memory->write(--SP, PC & 0xFF);
		PC = KEYPAD_INTR_ADDR;
		uint8_t IF = memory->read(INTERRUPT_FLAG) ^ 1 << static_cast<uint8_t>(INTERRUPTS::KEYPAD);
		memory->write(INTERRUPT_FLAG, IF);
	}

	if (PC == BOOTROM_SIZE && memory->is_booting) {
		std::cout << "finish boot seqence\n";
		memory->is_booting = false;
	}

	uint16_t tmp = PC;
	uint8_t op = memory->read(PC++);
	uint16_t NN;
	uint32_t NNNN;
	int8_t SN;
	uint8_t N, D1, D2;

	switch (op) {
	case 0x00: //nop
		break;
	case 0x01:
		RBC.b8[LO] = memory->read(PC++);
		RBC.b8[HI] = memory->read(PC++);
		break;
	case 0x02:
		memory->write(RBC.b16, RA);
		break;
	case 0x03:
		NN = RBC.b16 + 1;
		RBC.b8[HI] = NN >> 8;
		RBC.b8[LO] = NN & 0xFF;
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
		FH = ((RBC.b8[HI] & 0x0F) == 0x0F);
		break;
	case 0x06:
		RBC.b8[HI] = memory->read(PC++); //B
		break;
	case 0x07: // RLCA
		RA = (RA << 1) | (RA >> 7);
		FZ = FN = FH = 0;
		FC = (RA & 0x01);
		break;
	case 0x08:
		RBC.b8[LO] = memory->read(PC++);
		break;
	case 0x09:
		NNNN = RHL.b16 + RBC.b16;
		FN = 0;
		FH = (NNNN ^ RHL.b16 ^ RBC.b16) & 0x1000 ? 1 : 0;
		FC = (NNNN & 0xFFFF0000) ? 1 : 0;
		RHL.b8[HI] = (NNNN & 0x0000FF00) >> 8;
		RHL.b8[LO] = (NNNN & 0x000000FF);
		break;
	case 0x0A:
		RA = memory->read(RBC.b16);
		break;
	case 0x0B:
		NN = RBC.b16 - 1;
		RBC.b8[HI] = NN >> 8;
		RBC.b8[LO] = NN & 0xFF;
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
		RBC.b8[LO] = memory->read(PC++); //B
		break;
	case 0x0F: // RRCA
		FC = RA & 0x01;
		RA = (RA >> 1) | (RA << 7);
		FZ = 0;
		FN = 0;
		FH = 0;
		break;
	case 0x10:
		std::cout << "HALT";
		HALT = 1;
		break;
	case 0x11:
		RDE.b8[LO] = memory->read(PC++); //B
		RDE.b8[HI] = memory->read(PC++); //B
		break;
	case 0x12:
		memory->write(RDE.b16, RA);
		break;
	case 0x13:
		NN = RDE.b16 + 1;
		RDE.b8[HI] = NN >> 8;
		RDE.b8[LO] = NN & 0xFF;
		break;
	case 0x14:
		RDE.b8[HI]++;
		FZ = (RDE.b8[HI] == 0x00);
		FN = 0;
		FH = ((RDE.b8[HI] & 0x0F) == 0x00);
		break;
		break;
	case 0x15:
		RDE.b8[HI]--;
		FZ = (RDE.b8[HI] == 0x00);
		FN = 1;
		FH = ((RDE.b8[HI] & 0x0F) == 0x0F);
		break;
	case 0x16:
		RDE.b8[HI] = memory->read(PC++);
		break;
	case 0x17:
		N = RA;
		RA = RA << 1 | FC;
		FZ = FN = FH = 0;
		FC = (N >> 7) & 0x01;
		break;
	case 0x18:
		SN = (int8_t)memory->read(PC++);
		PC += SN;
		break;
	case 0x19:
		NNNN = RHL.b16 + RDE.b16;
		FN = 0;
		FH = (NNNN ^ RHL.b16 ^ RDE.b16) & 0x1000 ? 1 : 0;
		FC = (NNNN & 0xFFFF0000) ? 1 : 0;
		RHL.b8[HI] = (NNNN & 0x0000FF00) >> 8;
		RHL.b8[LO] = (NNNN & 0x000000FF);
		break;
	case 0x1A:
		RA = memory->read(RDE.b16);
		break;
	case 0x1B:
		NN = RDE.b16 - 1;
		RDE.b8[HI] = NN >> 8;
		RDE.b8[LO] = NN & 0xFF;
		break;
	case 0x1C:
		RDE.b8[LO]++;
		FZ = (RDE.b8[LO] == 0x00);
		FN = 0;
		FH = ((RDE.b8[LO] & 0x0F) == 0x00);
		break;
	case 0x1D:
		RDE.b8[LO]--;
		FZ = (RDE.b8[LO] == 0x00);
		FN = 1;
		FH = ((RDE.b8[LO] & 0x0F) == 0x0F);
		break;
	case 0x1E:
		RDE.b8[LO] = memory->read(PC++);
		break;
	case 0x1F:
		N = RA;
		RA = RA >> 1 | (FC << 7);
		FZ = FN = FH = 0;
		FC = N & 0x1;
		break;
	case 0x20:
		SN = (int8_t)memory->read(PC++);
		if (!FZ) PC += SN;
		break;
	case 0x21:
		RHL.b8[LO] = memory->read(PC++);
		RHL.b8[HI] = memory->read(PC++);
		break;
	case 0x22:
		memory->write(RHL.b16 ,RA);
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
	case 0x25:
		RHL.b8[HI]--;
		FZ = (RHL.b8[HI] == 0x00);
		FN = 1;
		FH = ((RHL.b8[HI] & 0x0F) == 0x0F);
		break;
	case 0x26:
		RHL.b8[HI] = memory->read(PC++);
		break;
	case 0x27:
		D1 = RA >> 4;
		D2 = RA & 0x0F;
		if (FN)
		{
			if (FH) D2 -= 6;
			if (FC) D1 -= 6;
			if (D2 > 9) D2 -= 6;
			if (D1 > 9)
			{
				D1 -= 6;
				FC = 1;
			}
		}
		break;
	case 0x28:
		SN = (int8_t)memory->read(PC++);
		if (FZ) PC += SN;
		break;
	case 0x29:
		NNNN = RHL.b16 + RHL.b16;
		FN = 0;
		FH = (NNNN & 0x1000) ? 1 : 0;
		FC = (NNNN & 0xFFFF0000) ? 1 : 0;
		RHL.b8[HI] = (NNNN & 0x0000FF00) >> 8;
		RHL.b8[LO] = (NNNN & 0x000000FF);
		break;
	case 0x2A:
		RA = memory->read(RHL.b16);
		NN = RHL.b16 + 1;
		RHL.b8[HI] = NN >> 8;
		RHL.b8[LO] = NN & 0xFF;
		break;
	case 0x2B:
		NN = RHL.b16 - 1;
		RHL.b8[HI] = NN >> 8;
		RHL.b8[LO] = NN & 0xFF;
		break;
	case 0x2C:
		RHL.b8[LO]++;
		FZ = (RHL.b8[LO] == 0x00);
		FN = 0;
		FH = ((RHL.b8[LO] & 0x0F) == 0x00);
		break;
	case 0x2D:
		RHL.b8[LO]--;
		FZ = (RHL.b8[LO] == 0x00);
		FN = 1;
		FH = ((RHL.b8[LO] & 0x0F) == 0x0F);
		break;
	case 0x2E:
		RHL.b8[LO] = memory->read(PC++);
		break;
	case 0x2F:
		RA = RA ^ 0xFF;
		FN = FH = 1;
		break;
	case 0x30: // JP NC, imm
		SN = (int8_t)memory->read(PC++);
		if (!FC) PC += SN;
		break;
	case 0x31:
		SP = memory->read(PC++);
		SP |= memory->read(PC++) << 8;
		break;
	case 0x32:
		memory->write(RHL.b16,  RA);
		NN = RHL.b16 - 1;
		RHL.b8[HI] = NN >> 8;
		RHL.b8[LO] = NN & 0xFF;
		break;
	case 0x34:
		N = memory->read(RHL.b16) + 1;
		FZ = (N == 0x00);
		FN = 0;
		FH = ((N & 0x0F) == 0x00);
		memory->write(RHL.b16, N);
		break;
	case 0x35:
		N = memory->read(RHL.b16) - 1;
		FZ = (N == 0x00);
		FN = 1;
		FH = ((N & 0x0F) == 0x0F);
		memory->write(RHL.b16, N);
		break;
	case 0x36:
		memory->write(RHL.b16, memory->read(PC++));
		break;
	case 0x37:
		FN = FH = 0;
		FC = 1;
		break;
	case 0x38:
		SN = (int8_t)memory->read(PC++);
		if (FC)
		{
			PC += SN;
		}
		break;
	case 0x39: // ADD HL, SP
		NNNN = RHL.b16 + SP;
		FN = 0;
		FH = (NNNN ^ RHL.b16 ^ SP) & 0x1000 ? 1 : 0;
		FC = (NNNN & 0xFFFF0000) ? 1 : 0;
		RHL.b8[HI] = (NNNN & 0x0000FF00) >> 8;
		RHL.b8[LO] = (NNNN & 0x000000FF);
		break;
	case 0x3A:
		RA = memory->read(RHL.b16);
		NN = RHL.b16 - 1;
		RHL.b8[HI] = NN >> 8;
		RHL.b8[LO] = NN & 0xFF;
		break;
	case 0x3B: // DEC SP
		SP--;
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
		RA = memory->read(PC++);
		break;
	case 0x3F:
		FN = FH = 0;
		FC = FC ^ 0x1;
		break;
	case 0x40://do nothing
		break;
	case 0x41: // LD B, C
		RBC.b8[HI] = RBC.b8[LO];
		break;
	case 0x42:
		RBC.b8[HI] = RDE.b8[HI];
		break;
	case 0x43: // LD B, E
		RBC.b8[HI] = RDE.b8[LO];
		break;
	case 0x44:
		RBC.b8[HI] = RHL.b8[HI];
		break;
	case 0x45: // LD B, L
		RBC.b8[HI] = RHL.b8[LO];
		break;
	case 0x46:	
		RBC.b8[HI] = memory->read(RHL.b16);
		break;
	case 0x47:
		RBC.b8[HI] = RA;
		break;
	case 0x48: // LD C, B
		RBC.b8[LO] = RBC.b8[HI];
		break;
	case 0x49: // LD C, C
		break;
	case 0x4A: // LD C, D
		RBC.b8[LO] = RDE.b8[HI];
	case 0x4B: // LD C, E
		RBC.b8[LO] = RDE.b8[LO];
		break;
	case 0x4C: // LD C, H
		RBC.b8[LO] = RHL.b8[HI];
		break;
	case 0x4D: // LD C, L
		RBC.b8[LO] = RHL.b8[LO];
		break;
	case 0x4E:	
		RBC.b8[LO] = memory->read(RHL.b16);
		break;
	case 0x4F:
		RBC.b8[LO] = RA;
		break;
	case 0x50:
		RDE.b8[HI] = RBC.b8[HI];
		break;
	case 0x51:
		RDE.b8[HI] = RBC.b8[LO];
		break;
	case 0x52:// do nothing
		break;
	case 0x53:
		RDE.b8[HI] = RDE.b8[LO];
		break;
	case 0x54:
		RDE.b8[HI] = RHL.b8[HI];
		break;
	case 0x55:
		RDE.b8[HI] = RHL.b8[LO];
		break;
	case 0x56:
		RDE.b8[HI] = memory->read(RHL.b16);
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
	case 0x5B://do nothing
		break;
	case 0x5C:
		RDE.b8[LO] = RHL.b8[HI];
		break;
	case 0x5D:
		RDE.b8[LO] = RHL.b8[LO];
		break;
	case 0x5E:
		RDE.b8[LO] = memory->read(RHL.b16);
		break;
	case 0x5F:
		RDE.b8[LO] = RA;
		break;
	case 0x60:
		RHL.b8[HI] = RBC.b8[HI];
		break;
	case 0x61:
		RHL.b8[HI] = RBC.b8[LO];
		break;
	case 0x62:
		RHL.b8[HI] = RDE.b8[HI];
		break;
	case 0x66:
		RHL.b8[HI] = memory->read(RHL.b16);
		break;
	case 0x67:
		RHL.b8[HI] = RA;
		break;
	case 0x68: // LD L, B
		RHL.b8[LO] = RBC.b8[HI];
		break;
	case 0x69:
		RHL.b8[LO] = RBC.b8[LO];
		break;
	case 0x6A: // LD L, D
		RHL.b8[LO] = RDE.b8[HI];
		break;
	case 0x6B:
		RHL.b8[LO] = RDE.b8[LO];
		break;
	case 0x6C: // LD L, H
		RHL.b8[LO] = RHL.b8[HI];
		break;
	case 0x6D: // LD L, L
		break;
	case 0x6E: // LD L, (HL)
		RHL.b8[LO] = memory->read(RHL.b16);
		break;
	case 0x6F:
		RHL.b8[LO] = RA;
		break;	
	case 0x70:
		memory->write(RHL.b16, RBC.b8[HI]);
		break;
	case 0x71:
		memory->write(RHL.b16, RBC.b8[LO]);
		break;
	case 0x72:
		memory->write(RHL.b16, RDE.b8[HI]);
		break;
	case 0x73:
		memory->write(RHL.b16, RDE.b8[LO]);
		break;
	case 0x74:
		memory->write(RHL.b16, RHL.b8[HI]);
		break;
	case 0x75:
		memory->write(RHL.b16, RHL.b8[LO]);
		break;
	case 0x76:
		//TODO
		break;
	case 0x77:
		memory->write(RHL.b16,  RA);
		break;
	case 0x78:
		RA = RBC.b8[HI];
		break;
	case 0x79:
		RA = RBC.b8[LO];
		break;
	case 0x7A:
		RA = RDE.b8[HI];
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
		RA = memory->read(RHL.b16);
		break;
	case 0x7F:
		break;
	case 0x80:
		NN = RA + RBC.b8[HI];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ RBC.b8[HI] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x81: // ADD A, C
		NN = RA + RBC.b8[LO];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ RBC.b8[LO] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
	case 0x82:
		NN = RA + RDE.b8[HI];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ RDE.b8[HI] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x83: // ADD A, E
		NN = RA + RDE.b8[LO];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ RDE.b8[LO] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x85:
		NN = RA + RHL.b8[LO];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ RHL.b8[LO] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x86:
		N = memory->read(RHL.b16);
		NN = RA + N;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x87:
		NN = RA + RA;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = NN & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x88: // ADC A, B
		N = RBC.b8[HI];
		NN = RA + N + FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x89:
		N = RBC.b8[LO];
		NN = RA + N + FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x8A:
		N = RDE.b8[HI];
		NN = RA + N + FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x8C:
		N = RHL.b8[HI];
		NN = RA + N + FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x8D:
		N = RHL.b8[LO];
		NN = RA + N + FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x8E: // ADC A, (HL)
		N = memory->read(RHL.b16);
		NN = RA + N + FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x8F: // ADC A, A
		N = RA;
		NN = RA + N + FC;
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
	case 0x91: // SUB C
		NN = RA - RBC.b8[LO];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ RBC.b8[LO] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x92: // SUB D
		NN = RA - RDE.b8[HI];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ RDE.b8[HI] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x93: // SUB E
		NN = RA - RDE.b8[LO];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ RDE.b8[LO] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x94: // SUB H
		NN = RA - RHL.b8[HI];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ RHL.b8[HI] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x95: // SUB L
		NN = RA - RHL.b8[LO];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ RHL.b8[LO] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x96:
		N = memory->read(RHL.b16);
		NN = RA - N;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x97:
		RA = 0x00;
		FZ = FN = 1;
		FH = FC = 0;
		break;
	case 0x98:
		N = RBC.b8[HI];
		NN = RA - N - FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x99:
		N = RBC.b8[LO];
		NN = RA - N - FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x9A:
		N = RDE.b8[HI];
		NN = RA - N - FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x9B:
		N = RDE.b8[LO];
		NN = RA - N - FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x9C:
		N = RHL.b8[HI];
		NN = RA - N - FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x9D:
		N = RHL.b8[LO];
		NN = RA - N - FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x9E:
		N = memory->read(RHL.b16);
		NN = RA - N - FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0x9F:
		RA = FC ? 0xFF : 0x00;
		FZ = FC ? 0x00 : 0x01;
		FN = 1;
		FH = FC;
		break;
	case 0xA0:
		RA = RA & RBC.b8[HI];
		FZ = (RA == 0x00);
		FN = 0;
		FH = 1;
		FC = 0;
		break;
	case 0xA1:
		RA = RA & RBC.b8[LO];
		FZ = (RA == 0x00);
		FN = 0;
		FH = 1;
		FC = 0;
		break;
	case 0xA2:
		RA = RA & RDE.b8[HI];
		FZ = (RA == 0x00);
		FN = 0;
		FH = 1;
		FC = 0;
		break;
	case 0xA3:
		RA = RA & RDE.b8[LO];
		FZ = (RA == 0x00);
		FN = 0;
		FH = 1;
		FC = 0;
		break;
	case 0xA4:
		RA = RA & RHL.b8[HI];
		FZ = (RA == 0x00);
		FN = 0;
		FH = 1;
		FC = 0;
		break;
	case 0xA5:
		RA = RA & RHL.b8[LO];
		FZ = (RA == 0x00);
		FN = 0;
		FH = 1;
		FC = 0;
		break;
	case 0xA6:
		RA = RA & memory->read(RHL.b16);
		FZ = (RA == 0x00);
		FN = 0;
		FH = 1;
		FC = 0;
		break;
	case 0xA7:
		FZ = (RA == 0x00);
		FN = 0;
		FH = 1;
		FC = 0;
		break;
	case 0xA8:
		RA = RA ^ RBC.b8[HI];
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xA9:
		RA = RA ^ RBC.b8[LO];
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xAA: // XOR D
		RA = RA ^ RDE.b8[HI];
		FZ = (RA == 0x00);
		FN = 0;
		FH = 0;
		FC = 0;
		break;
	case 0xAB: // XOR E
		RA = RA ^ RDE.b8[LO];
		FZ = (RA == 0x00);
		FN = 0;
		FH = 0;
		FC = 0;
		break;
	case 0xAC: // XOR H
		RA = RA ^ RHL.b8[HI];
		FZ = (RA == 0x00);
		FN = 0;
		FH = 0;
		FC = 0;
		break;
	case 0xAD: // XOR L
		RA = RA ^ RHL.b8[LO];
		FZ = (RA == 0x00);
		FN = 0;
		FH = 0;
		FC = 0;
		break;
	case 0xAE:
		RA = RA ^ memory->read(RHL.b16);
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xAF:
		RA = 0x00;
		FZ = 1;
		FN = FH = FC = 0;
		break;
	case 0xB0:
		RA = RA | RBC.b8[HI];
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xB1:
		RA = RA | RBC.b8[LO];
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xB2:
		RA = RA | RDE.b8[HI];
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xB3:
		RA = RA | RDE.b8[LO];
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xB4:
		RA = RA | RHL.b8[HI];
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xB5:
		RA = RA | RHL.b8[LO];
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xB6:
		RA = RA | memory->read(RHL.b16);
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xB7:
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xB8:
		NN = RA - RBC.b8[HI];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ RBC.b8[HI] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		break;
	case 0xB9:
		NN = RA - RBC.b8[LO];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ RBC.b8[LO] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		break;
	case 0xBA:
		NN = RA - RDE.b8[HI];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ RDE.b8[HI] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		break;
	case 0xBB:
		NN = RA - RDE.b8[LO];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ RDE.b8[LO] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		break;
	case 0xBC:
		NN = RA - RHL.b8[HI];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ RHL.b8[HI] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		break;
	case 0xBD:
		NN = RA - RHL.b8[LO];
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ RHL.b8[LO] ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		break;
	case 0xBE:
		N = memory->read(RHL.b16);
		NN = RA - N;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		break;
	case 0xBF:
		FZ = FN = 1;
		FH = FC = 0;
		break;
	case 0xC0:
		if (!FZ)
		{
			NN = memory->read(SP++);
			NN |= memory->read(SP++) << 8;
			PC = NN;
		}
		break;
	case 0xC1:
		RBC.b8[LO] = memory->read(SP++);
		RBC.b8[HI] = memory->read(SP++);
		break;
	case 0xC2:
		NN = memory->read(PC++);
		NN |= memory->read(PC++) << 8;
		if (!FZ) PC = NN;
		break;
	case 0xC3:
		NN = memory->read(PC++);
		NN |= memory->read(PC++) << 8;
		PC = NN;
		break;
	case 0xC5:
		memory->write(--SP,RBC.b8[HI]);
		memory->write(--SP,RBC.b8[LO]);
		break;
	case 0xC6: // ADD A, imm
		N = memory->read(PC++);
		NN = RA + N;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0xC7:
		memory->write(--SP, PC >> 8);
		memory->write(--SP, PC & 0xFF);
		PC = 0x0000;
		break;
	case 0xC8:
		if (FZ)
		{
			NN = memory->read(SP++);
			NN |= memory->read(SP++) << 8;
			PC = NN;
		}
		break;
	case 0xC9:
		NN = memory->read(SP++);
		NN |= memory->read(SP++) << 8;
		PC = NN;
		break;
	case 0xCA:
		NN =  memory->read(PC++);
		NN |= memory->read(PC++) << 8;
		if (FZ) PC = NN;
		break;
	case 0xCB:
		shift_operation_CB();
		break;
	case 0xCC:	// CALL Z, imm
		NN = memory->read(PC++);
		NN |= memory->read(PC++) << 8;
		if (FZ)
		{
			memory->write(--SP, PC >> 8);
			memory->write(--SP, PC & 0xFF);
			PC = NN;
		}
		break;
	case 0xCD:
		NN = memory->read(PC++);
		NN |= memory->read(PC++) << 8;
		memory->write(--SP , PC >> 8);
		memory->write(--SP , PC & 0xFF);
		PC = NN;
		break;
	case 0xCE:
		N = memory->read(PC++);
		NN = RA + N + FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 0;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0xCF: 
		memory->write(--SP, PC >> 8);
		memory->write(--SP, PC & 0xFF);
		PC = 0x0008;
		break;
	case 0xD0: // RET NC
		if (!FC)
		{
			NN =  memory->read(SP++);
			NN |= memory->read(SP++) << 8;
			PC = NN;
		}
		break;
	case 0xD1:
		RDE.b8[LO] = memory->read(SP++);
		RDE.b8[HI] = memory->read(SP++);
		break;
	case 0xD2:
		NN =  memory->read(PC++);
		NN |= memory->read(PC++) << 8;
		if (!FC) PC = NN;
		break;
	case 0xD5:
		memory->write(--SP,RDE.b8[HI]);
		memory->write(--SP,RDE.b8[LO]);
		break;
	case 0xD6:
		N = memory->read(PC++);
		NN = RA - N;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0xD7:
		memory->write(--SP, PC >> 8);
		memory->write(--SP, PC & 0xFF);
		PC = 0x0010;
		break;
	case 0xD8:
		if (FC)
		{
			NN = memory->read(SP++);
			NN |= memory->read(SP++) << 8;
			PC = NN;
		}
		break;
	case 0xD9:
		NN = memory->read(SP++);
		NN |= memory->read(SP++) << 8;
		PC = NN;
		IME = 1;
		break;
	case 0xDA: // JP C, imm
		NN = memory->read(PC++);
		NN |= memory->read(PC++) << 8;
		if (FC)
		{
			PC = NN;
		}
		break;
	case 0xDB: // illegal
		break;
	case 0xDC: // CALL C, imm
		NN = memory->read(PC++);
		NN |= memory->read(PC++) << 8;
		if (FC)
		{
			memory->write(--SP, PC >> 8);
			memory->write(--SP, PC & 0xFF);
			PC = NN;
		}
		break;
	case 0xDD: // illegal
		break;
	case 0xDE:
		N = memory->read(PC++);
		NN = RA - N - FC;
		FZ = ((NN & 0xFF) == 0x00);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		RA = NN & 0xFF;
		break;
	case 0xE0:
		memory->write((0xFF00 | memory->read(PC++)),  RA);
		break;
	case 0xE1:
		RHL.b8[LO] = memory->read(SP++);
		RHL.b8[HI] = memory->read(SP++);
		break;
	case 0xE2:
		memory->write(0xFF00 | RBC.b8[LO], RA);
		break;
	case 0xE5:
		memory->write(--SP,RHL.b8[HI]);
		memory->write(--SP,RHL.b8[LO]);
		break;
	case 0xE6:
		RA = RA & memory->read(PC++);
		FZ = (RA == 0x00);
		FN = FC = 0;
		FH = 1;
		break;
	case 0xE9:
		PC = RHL.b16;
		break;
	case 0xEA:
		memory->write(memory->read(PC++) |memory->read(PC++)<<8,  RA);
		break;
	case 0xEB: // illegal
		break;
	case 0xEC: // illegal
		break;
	case 0xED: // illegal
		break;
	case 0xEE:
		RA = RA ^ memory->read(PC++);
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xEF:
		memory->write(--SP, PC >> 8);
		memory->write(--SP, PC & 0xFF);
		PC = 0x0028;
		break;
	case 0xF0:
		RA = memory->read((0xFF00 | memory->read(PC++)));
		break;
	case 0xF1:
		N = memory->read(SP++);
		FZ = (N >> 7) & 1;
		FN = (N >> 6) & 1;
		FH = (N >> 5) & 1;
		FC = (N >> 4) & 1;
		RA = memory->read(SP++);
		break;
	case 0xF3:
		std::cout << "disable IME\n";
		IME = 0;
		break;
	case 0xF5:
		memory->write(--SP , RA);
		memory->write(--SP , FZ<<7|FN<<6|FH<<5|FC<<4);
		break;
	case 0xF6:
		RA = RA | memory->read(PC++);
		FZ = (RA == 0x00);
		FN = FH = FC = 0;
		break;
	case 0xF7:
		memory->write(--SP, PC >> 8);
		memory->write(--SP, PC & 0xFF);
		PC = 0x0030;
		break;
	case 0xF8: // LD HL, SP+/-imm
		SN = (int8_t)memory->read(PC++);
		NN = SP + SN;
		if (SN >= 0)
		{
			FZ = 0;
			FN = 0;
			FH = ((SP ^ SN ^ NN) & 0x1000) ? 1 : 0;
			FC = (SP > NN);
		}
		else
		{
			FZ = 0;
			FN = 0;
			FH = ((SP ^ SN ^ NN) & 0x1000) ? 1 : 0;
			FC = (SP < NN);
		}
		RHL.b8[HI] = (NN & 0xFF00) >> 8;
		RHL.b8[LO] = (NN & 0x00FF);
		break;
	case 0xF9:
		SP = RHL.b16;
		break;
	case 0xFA:
		RA = memory->read( memory->read(PC++) | memory->read(PC++) << 8);
		break;
	case 0xFB:
		IME = 1;
		break;
	case 0xFE:
		N = memory->read(PC++);
		NN = RA - N;
		FZ = ((NN & 0xFF) == 0);
		FN = 1;
		FH = (RA ^ N ^ NN) & 0x10 ? 1 : 0;
		FC = (NN & 0xFF00) ? 1 : 0;
		break;
	case 0xFF:
		memory->write(--SP, PC >> 8);
		memory->write(--SP, PC & 0xFF);
		PC = 0x0038;
		break;
	default:
		std::cout << "Operation not inplemented : " << std::hex << (int)op << std::endl;//todo for debug
		dump_reg();
		while (1);
	}

	cycle_count += OP_CYCLES[op];
	lcd_count += OP_CYCLES[op];
	div_count += OP_CYCLES[op];

	//increment div counter
	if (div_count > CLOCK_FREQUENCY / DIV_COUNTER_INCREMENT_FREQUENCY) {
		div_count = 0;
		memory->write(DIV_REGISTER, memory->read(DIV_REGISTER)+1);
	}

	if (lcd_count > LCD_LINE_CYCLES) {
		memory->write( LCDC_Y_CORDINATE , (memory->read(LCDC_Y_CORDINATE) + 1) % LCD_VERT_LINES);
		lcd_count -= LCD_LINE_CYCLES;
		if (memory->read(LCDC_Y_CORDINATE) == FRAME_HEIGHT) {
			ready_for_render = true;
		}
	}
}
void CPU::set_interrupt_flag(INTERRUPTS intrpt) {
	memory->write( INTERRUPT_FLAG , memory->read(INTERRUPT_FLAG) | 1 << static_cast<uint8_t>(intrpt) );
}

void CPU::dump_reg() {
	int op = memory->read(PC);
	std::cout << std::hex << "PC " << (int)PC << " " <<
		         std::hex << "OP " << (int)op << " " << 
				 std::hex << "RA " << (int)RA << " " <<
				 std::hex << "RB " << (int)RBC.b8[HI] << " " <<
				 std::hex << "RC " << (int)RBC.b8[LO] << " " <<
				 std::hex << "RD " << (int)RDE.b8[HI] << " " <<
				 std::hex << "RE " << (int)RDE.b8[LO] << " " <<
				 std::hex << "RH " << (int)RHL.b8[HI] << " " <<
				 std::hex << "RL " << (int)RHL.b8[LO] << " " << std::endl;
}

GPU::GPU() {
	frame_buffer= std::make_unique<uint8_t[]>(static_cast<size_t>(frame_height)*frame_width );
	total_frame = std::make_unique<uint8_t[]>(256 * 256);
}

void GPU::set_memmap(Memory* mem) {
	memory = mem;
}

void GPU::draw_frame() {
	auto display_enable = (memory->read(LCDC) >> 7) & 0x01;
	if (!display_enable) return;

	auto window_tilemap_select = (memory->read(LCDC) >> 6) & 0x01;
	auto window_display = (memory->read(LCDC) >> 5) & 0x01;
	auto tiledata_select= (memory->read(LCDC) >> 4) & 0x01;
	auto bg_tilemap_select = (memory->read(LCDC) >> 3) & 0x01;
	auto obj_size = (memory->read(LCDC) >> 2) & 0x01;
	auto obj_display_enable = (memory->read(LCDC) >> 1) & 0x01;
	auto bg_display = (memory->read(LCDC) >> 0) & 0x01;

	//draw background
	if (bg_display) {
		auto bg_top_addr = 0x9800 + 0x400 * bg_tilemap_select;
		auto tile_top_addr = 0x9000 - 0x1000 * tiledata_select;
		for (int i = 0; i < 32; i++) {
			for (int j = 0; j < 32; j++) {
				auto tilenum = memory->read(bg_top_addr + i * 32 + j);
				//if (tilenum != 0) {
				//	int kads = 0;
				//}
				uint16_t tile_id;
				if (!tiledata_select) tile_id = (int8_t)tilenum;
				else tile_id = tilenum;
				auto tile_addr = tile_top_addr + 16 * tile_id;
				for (int k = 0; k < 8; k++) { //1 tile has 16 byte 8px * 8line 
					uint8_t b1 = memory->read(tile_addr++);
					uint8_t b2 = memory->read(tile_addr++);
					for (int l = 0; l < 8; l++) {
						auto px = ((b2 >> (7 - l)) & 0x01);
						px += ((b1 >> (7 - l)) & 0x01)<<1;
						auto y = 8 * i + k;
						auto x = 8 * j + l;
						total_frame[static_cast<size_t>(y) * 256 + x] = px;
					}
				}
			}
		}
	}
	else {
		std::memset(total_frame.get(), 0, 256*256);
	}
	
	if (window_display) {
		//draw window
		auto window_top_addr = 0x9800 + 0x400 * window_tilemap_select;
		auto tile_top_addr = 0x9000 - 0x1000 * tiledata_select;
		for (int i = 0; i < 32; i++) {
			for (int j = 0; j < 32; j++) {
				auto tilenum = memory->read(window_top_addr + i * 32 + j);
				uint16_t tile_id;
				if (!tiledata_select) tile_id = (int8_t)tilenum;
				else tile_id = tilenum;
				auto tile_addr = tile_top_addr + 16 * tile_id;
				for (int k = 0; k < 8; k++) { //1 tile has 16 byte 8px * 8line 
					uint8_t b1 = memory->read(tile_addr++);
					uint8_t b2 = memory->read(tile_addr++);
					for (int l = 0; l < 8; l++) {
						auto px = ((b2 >> (7 - l)) & 0x01);
						px += ((b1 >> (7 - l)) & 0x01) << 1;
						auto y = 8 * i + k;
						auto x = 8 * j + l;
						if(px!=0) total_frame[static_cast<size_t>(y) * 256 + x] = px;
					}
				}
			}
		}
	}

	auto scroll_y = memory->read(LCD_SCROLL_Y);
	auto scroll_x = memory->read(LCD_SCROLL_X);

	for (int y = 0; y < frame_height; y++) {
		for (int x = 0; x < frame_width; x++) {
			if (scroll_x + x < frame_width && scroll_y + y < frame_height) {
				frame_buffer[static_cast<size_t>(y) * frame_width + x] = total_frame[(static_cast<size_t>(scroll_y) + y) * 256 + (static_cast<size_t>(scroll_x) + x)];
			}
			else {
				frame_buffer[static_cast<size_t>(y) * frame_width + x] = 0;
			}
		}
	}
	
	if (obj_display_enable) {
		//draw obj
		auto obj_top_addr = 0xFE00;
		auto obj_addr = obj_top_addr;
		auto tile_top_addr = 0x8000;
		for (int i = 0; i < 40; i++) {//40 objects 
			auto top = memory->read(obj_addr++) - 16;
			auto left= memory->read(obj_addr++) - 8;
			auto tile_id = memory->read(obj_addr++);
			auto flags = memory->read(obj_addr++);
			auto tile_addr = tile_top_addr + 16 * tile_id;
			
			for (int k = 0; k < 8; k++) { //1 tile has 16 byte 8px * 8line 
				uint8_t b1 = memory->read(tile_addr++);
				uint8_t b2 = memory->read(tile_addr++);
				for (int l = 0; l < 8; l++) {
					auto px = ((b2 >> (7 - l)) & 0x01);
					px += ((b1 >> (7 - l)) & 0x01) << 1;
					auto x = left + l;
					auto y = top + k;
					if (px != 0) frame_buffer[static_cast<size_t>(y) * frame_width + x] = px;
				}
			}
		}
	}	
}

Memory::Memory(Cartridge& cart, uint8_t* rom, size_t rom_size, uint8_t* bootrom) {
	std::memcpy(boot_rom, bootrom, BOOTROM_SIZE);
	std::memset(map, 0, MAX_ADDRESS);
	std::memcpy(map, rom, CART_MAX_ADDR);

	rom_banks.resize(cart.rom_size_banknum);
	memory_bank_size = cart.rom_size_banknum;
	// allocate and copy rom banks
	for (int i = 0;i < cart.rom_size_banknum;i++) {
		rom_banks[i] = std::make_unique<uint8_t[]>(0x4000);
		std::memcpy(rom_banks[i].get(), rom + i*0x4000, 0x4000);
	}
	
}

void Memory::dma_operation(uint8_t src) {
	uint16_t src_addr = src << 8;	//copy from 0x**00 ~ 0x**9F
	uint16_t dst_addr = 0xFE00;		//copy to   0xFE00 ~ 0xFE9F
	std::memcpy(map + dst_addr, map + src_addr, 0x9F);
}

void Memory::write(uint16_t address, uint8_t data) {
	if (memory_bank_size && address >= 0x2000 && address < 0x4000) {
		std::cout << "switch bank : "<<  address << " " << static_cast<int>(data) << std::endl;
		if(memory_bank_size > data)
			memory_bank = data;
		return;
	}

	if (address >= 0 && address < 0x8000) {
		std::cout << "unable to write ROM\n";
		return;
	}

	if (address >= 0x2000 && address < 0x4000) {
		std::cout << "switch bank\n";
		return;
	}

	if (address >= 0xA000 && address < 0xC000) {
		std::cout << ">>>External rom<<<\n";
		return;
	}

	//handle key input
	if (address == KEY_INPUT_ADDRES) {	
		if (data & 0x10) data = (key & 0x0F);
		else if (data & 0x20) data = 0x0F & (key>>4);
	}

	//DMA operation
	if (address == DMA_OP_ADDRESS) {
		dma_operation(data);
	}

	//Default operation
	map[address] = data;
}

uint8_t Memory::read(uint16_t address) {

	// access to rom cartridge
	if (memory_bank_size && address >= 0x4000 && address < 0x8000) {
		return rom_banks[memory_bank][address - 0x4000];
	}


	if (address >= 0xA000 && address < 0xC000) {
		std::cout << ">>>External ram<<<\n";
	}

	if (is_booting && address < BOOTROM_SIZE ) {
		//booting. read from boot rom
		return boot_rom[address];
	}
	//Default read
	return map[address];
}

