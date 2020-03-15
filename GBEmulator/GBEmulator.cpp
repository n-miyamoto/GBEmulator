#include <iostream>
#include <algorithm>
#include <random>
#include <GL/glut.h>
#include "Gameboy.h"
#pragma warning(disable: 4996) 

#define IMAGE_SIZE_IN_BYTE (4 * FRAME_WIDTH * FRAME_HEIGHT)

unsigned char* bitmap;
Gameboy* GB;

//Create bitmap
void create_bitmap(unsigned char* bitmap) {
	uint8_t val;

	for (int i = 0; i < FRAME_HEIGHT; i++) {
		for (int j = 0; j < FRAME_WIDTH; j++) {
			int offset = (FRAME_HEIGHT-i-1) * FRAME_WIDTH + j;
			val = 255 - GB->gpu.frame_buffer[i*FRAME_WIDTH+j]*64;
			bitmap[offset * 4 + 0] = val;
			bitmap[offset * 4 + 1] = val;
			bitmap[offset * 4 + 2] = val;
			bitmap[offset * 4 + 3] = 255;
		}
	}
}

//Idle callback
void idle(void) {
	GB->cpu.step();
}

//keyboard event callback
void key_press(unsigned char key, int x, int y) {
	KEYS k;
	switch (key) {
	case 'a': k = KEYS::BUTTON_A; break;
	case 's': k = KEYS::BUTTON_B; break;
	case 'b': k = KEYS::BUTTON_SELECT; break;
	case 'n': k = KEYS::BUTTON_START; break;
	case 'l': k = KEYS::DIRECTION_R; break;
	case 'h': k = KEYS::DIRECTION_L; break;
	case 'k': k = KEYS::DIRECTION_U; break;
	case 'j': k = KEYS::DIRECTION_D; break;
	default: return;
	}
	GB->press(k);
}

void key_release(unsigned char key , int x , int y) {
	KEYS k;
	switch (key) {
	case 'a': k = KEYS::BUTTON_A; break;
	case 's': k = KEYS::BUTTON_B; break;
	case 'b': k = KEYS::BUTTON_SELECT; break;
	case 'n': k = KEYS::BUTTON_START; break;
	case 'l': k = KEYS::DIRECTION_R; break;
	case 'h': k = KEYS::DIRECTION_L; break;
	case 'k': k = KEYS::DIRECTION_U; break;
	case 'j': k = KEYS::DIRECTION_D; break;
	default: return;
	}
	GB->release(k);
}


//Rasterize callback
static void draw() {
  create_bitmap(bitmap);
  glClear(GL_COLOR_BUFFER_BIT);
  glDrawPixels(FRAME_WIDTH, FRAME_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);
  glFlush();
}

//Timer Callback
static void timer(int value) {
	GB->cpu.set_interrupt_flag(INTERRUPTS::V_BLANK);
	if (GB->cpu.ready_for_render) {
		GB->gpu.draw_frame();
		GB->cpu.ready_for_render = false;
		glutPostRedisplay();
	}
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char *argv[]) 
{
	//load cart
	FILE* fp;
	FILE* br;
	const char *romfile = "rsrc/Tetris.gb";
	const char *boot_rom= "rsrc/DMG_ROM.bin";
	fp = fopen(romfile, "rb");
	if (fp == NULL) {
		std::cout << "no cart" << std::endl;
		return -1;
	}
	br = fopen(boot_rom, "rb");
	if (br == NULL) {
		std::cout << "no rom file" << std::endl;
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	size_t rom_size = ftell(fp);
	rewind(fp);
	uint8_t* rom = (uint8_t*)malloc(rom_size);
	if (rom == NULL) {
		std::cout << "malloc failed" << std::endl;
		return -1;
	}
	fread(rom, sizeof(uint8_t), rom_size, fp);
	fclose(fp);

	fseek(br, 0, SEEK_END);
	size_t boot_rom_size = ftell(br);
	rewind(br);
	uint8_t* bootrom = (uint8_t*)malloc(boot_rom_size);
	if (bootrom == NULL) {
		std::cout << "malloc failed" << std::endl;
		return -1;
	}
	fread(bootrom, sizeof(uint8_t), boot_rom_size, br);
	fclose(br);


	//init GameBoy
	Gameboy gb(rom, rom_size, bootrom);
	gb.showCartInfo();
	GB = &gb;

	bitmap = new unsigned char[IMAGE_SIZE_IN_BYTE];

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);
	glutInitWindowSize(FRAME_WIDTH, FRAME_HEIGHT);
	glutCreateWindow("bitmap");
	glutDisplayFunc(draw);
	glutIgnoreKeyRepeat(GL_TRUE);	
	glutIdleFunc(idle);
	glutTimerFunc(100, timer, 0);
	glutKeyboardFunc(key_press);
	glutKeyboardUpFunc(key_release);

	glutMainLoop();

	free(rom);
	delete[] bitmap;

	return 0;
}

