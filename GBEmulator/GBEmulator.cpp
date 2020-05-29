#include <iostream>
#include <fstream>
#include <algorithm>
#include <random>
#include <GL/glut.h>
#include "Gameboy.h"
#pragma warning(disable: 4996) 

#define IMAGE_SIZE_IN_BYTE (3 * FRAME_WIDTH * FRAME_HEIGHT)

#ifdef EMSCRIPTEN
    #include <emscripten/emscripten.h>
    #define GL_GLEXT_PROTOTYPES
    #define EGL_EGLEXT_PROTOTYPES
#endif

std::unique_ptr<uint8_t[]> bitmap;
Gameboy* GB;

const int modifier = 10;
 
// Window size
int display_width = FRAME_WIDTH * modifier;
int display_height = FRAME_HEIGHT * modifier;

//Create bitmap
void create_bitmap(unsigned char* bitmap) {
	uint8_t val;

	for (int i = 0; i < FRAME_HEIGHT; i++) {
		for (int j = 0; j < FRAME_WIDTH; j++) {
			int offset = i * FRAME_WIDTH + j;
			val = 255 - GB->gpu.frame_buffer[static_cast<size_t>(i)*FRAME_WIDTH+j]*64;
			bitmap[offset * 3 + 0] = val;
			bitmap[offset * 3 + 1] = val;
			bitmap[offset * 3 + 2] = val;
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
	create_bitmap(bitmap.get());
	glClear(GL_COLOR_BUFFER_BIT);
	glTexSubImage2D(GL_TEXTURE_2D, 0 ,0, 0, FRAME_WIDTH, FRAME_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)bitmap.get());	
    glBegin( GL_QUADS );
        glTexCoord2d(0.0, 0.0);		glVertex2d(0.0,			  0.0);
        glTexCoord2d(1.0, 0.0); 	glVertex2d(display_width, 0.0);
        glTexCoord2d(1.0, 1.0); 	glVertex2d(display_width, display_height);
        glTexCoord2d(0.0, 1.0); 	glVertex2d(0.0,			  display_height);
    glEnd();

	glutSwapBuffers();  
}

void reshape_window(GLsizei w, GLsizei h)
{
    glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);        
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);
 
    display_width = w;
    display_height = h;
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
	const char* boot_rom = "rsrc/DMG_ROM.bin";
	const char* romfile = "rsrc/Tetris.gb";

	// load cart romfile
	std::ifstream rom_file(romfile, std::ios_base::in | std::ios_base::binary);
	if (rom_file.fail()) {
		std::cerr << "Failed to open file." << std::endl;
		return -1;
	}
	// copy rom size
	rom_file.seekg(0, std::ios::end);
	size_t rom_size = rom_file.tellg();
	rom_file.clear();
	auto rom = std::make_unique<uint8_t[]>(rom_size);
	if (rom == nullptr) {
		std::cout << "malloc failed" << std::endl;
		return -1;
	}
	rom_file.read((char*)rom.get(), rom_size);

	//load cart
	//FILE* fp;
	FILE* br;
	//const char *romfile = "rsrc/Tetris.gb";
	//const char *boot_rom= "rsrc/DMG_ROM.bin";
	br = fopen(boot_rom, "rb");
	/*fp = fopen(romfile, "rb");
	if (fp == NULL) {
		std::cout << "no cart" << std::endl;
		return -1;
	}
	if (br == NULL) {
		std::cout << "no rom file" << std::endl;
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	size_t rom_size = ftell(fp);
	rewind(fp);
	auto rom = std::make_unique<uint8_t[]>(rom_size);
	if (rom == nullptr) {
		std::cout << "malloc failed" << std::endl;
		return -1;
	}
	fread(rom.get(), sizeof(uint8_t), rom_size, fp);
	fclose(fp);
*/
	fseek(br, 0, SEEK_END);
	size_t boot_rom_size = ftell(br);
	rewind(br);
	auto bootrom = std::make_unique<uint8_t[]>(boot_rom_size);
	if (bootrom == nullptr) {
		std::cout << "malloc failed" << std::endl;
		return -1;
	}
	fread(bootrom.get(), sizeof(uint8_t), boot_rom_size, br);
	fclose(br);

	//init GameBoy
	Gameboy gb(rom.get(), rom_size, bootrom.get());
	gb.showCartInfo();
	GB = &gb;

	bitmap = std::make_unique<uint8_t[]>(IMAGE_SIZE_IN_BYTE);

	glutInit(&argc, argv);
	glutInitWindowSize(FRAME_WIDTH, FRAME_HEIGHT);
	glutInitWindowPosition(320, 320);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA );
	glutCreateWindow("bitmap");
	glutDisplayFunc(draw);
	glutIgnoreKeyRepeat(GL_TRUE);	
	glutIdleFunc(idle);
	glutReshapeFunc(reshape_window);
	glutTimerFunc(100, timer, 0);
	glutKeyboardFunc(key_press);
	glutKeyboardUpFunc(key_release);

	glTexImage2D(
		GL_TEXTURE_2D, 0, 3, FRAME_WIDTH, FRAME_HEIGHT,
		0, GL_RGB, GL_UNSIGNED_BYTE, bitmap.get()
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); 
	glEnable(GL_TEXTURE_2D);
	glutMainLoop();

	return 0;
}

