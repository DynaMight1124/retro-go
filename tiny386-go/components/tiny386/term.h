#ifndef TERM_H
#define TERM_H

typedef struct Term Term;
typedef struct VGAState VGAState;
Term *term_init(
	VGAState *vga,
	void (*kbd_cb)(void *, unsigned char scan_code, int is_pressed),
	void *kbd);
void term_step(Term *s);


#endif /* TERM_H */
