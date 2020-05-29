#include <iostream>
#include <fstream>
#include <algorithm>
#include <random>
#include <GL/glut.h>
#include "Gameboy.h"

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
	for (int i = 0; i < FRAME_HEIGHT * FRAME_WIDTH; i++) {
		for (int j=0;j<3;j++) 
			bitmap[i * 3 + j] = 255 - GB->gpu.frame_buffer[i] * 64;
	}
}

//Idle callback
void idle(void) {
	GB->cpu.step();
}

static KEYS char_to_key(const char c) {
	KEYS k;
	switch (c) {
	case 'a': k = KEYS::BUTTON_A; break;
	case 's': k = KEYS::BUTTON_B; break;
	case 'b': k = KEYS::BUTTON_SELECT; break;
	case 'n': k = KEYS::BUTTON_START; break;
	case 'l': k = KEYS::DIRECTION_R; break;
	case 'h': k = KEYS::DIRECTION_L; break;
	case 'k': k = KEYS::DIRECTION_U; break;
	case 'j': k = KEYS::DIRECTION_D; break;
	default : k = KEYS::NOT_KEY;
	}
	return k;
}

//keyboard event callback
void key_press(unsigned char key, int x, int y) {
	KEYS k = char_to_key(key);
	if (k == KEYS::NOT_KEY) return;
	GB->press(k);
}
void key_release(unsigned char key , int x , int y) {
	KEYS k = char_to_key(key);
	if (k == KEYS::NOT_KEY) return;
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

size_t read_file_and_copy(std::unique_ptr<uint8_t[]>& ptr, const char* filepath) {
	//read file
	std::ifstream ifs(filepath, std::ios::in | std::ios::binary);
	if (ifs.fail()) {
		std::cerr << "Failed to open file." << std::endl;
		return -1;
	}
	
	//read file size
	ifs.seekg(0, ifs.end);
	auto sz = ifs.tellg();
	ifs.seekg(0, ifs.beg);

	//alloc buf and load to mem
	ptr = std::make_unique<uint8_t[]>(sz);
	if (ptr == nullptr) {
		std::cout << "malloc failed" << std::endl;
		return -1;
	}
	ifs.read(reinterpret_cast<char*>(ptr.get()), sz);
	
	return sz;
}

int main(int argc, char *argv[]) 
{
	//load cart
	const char* romfile = "rsrc/Tetris.gb";
	std::unique_ptr<uint8_t[]> rom;
	std::size_t rom_size = read_file_and_copy(rom, romfile);

	//load boot rom
	const char* boot_rom_path = "rsrc/DMG_ROM.bin";
	std::unique_ptr<uint8_t[]> boot_rom;
	std::size_t boot_rom_size = read_file_and_copy(boot_rom, boot_rom_path);

	//init GameBoy
	Gameboy gb(rom.get(), rom_size, boot_rom.get());
	gb.show_cart_info();
	GB = &gb;

	bitmap = std::make_unique<uint8_t[]>(IMAGE_SIZE_IN_BYTE);

	//Init opengl
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

	//main loop
	glutMainLoop();

	return 0;
}

