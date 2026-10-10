#ifndef QUAKEGENERIC_H
#define QUAKEGENERIC_H


int QG_Milliseconds(void);
void QG_GetMouseDiff(int* dx, int* dy);
void QG_CaptureMouse(void);
void QG_ReleaseMouse(void);

void QG_Mkdir(const char* path);

#endif