#ifndef Q_ARENA_H
#define Q_ARENA_H

// Single contiguous arena holding all permanent engine memory (model hunk,
// zone / Z, images, surface cache, framebuffers). Allocated from PSRAM at
// startup, isolated from the transient heap used by file loading etc.
//
// Author: Alejandro Villegas Alonso
//         https://www.linkedin.com/in/alejandro-villegas-alonso-825041b1

int		Q_Arena_Init (int size);	// reserve from PSRAM, returns actual reserved (0 = fail)
int		Q_Arena_Reserved (void);	// bytes actually reserved
void	*Q_Arena_Carve (int size);			// reserve a fixed sub-region at the front (hunk pool)
void	*Q_Arena_Alloc (int size);
void	Q_Arena_Free (void *p);
int		Q_Arena_Used (void);				// bytes resident in used blocks (incl headers)
int		Q_Arena_FreeSize (void);			// bytes still allocatable
int		Q_Arena_Largest (void);				// largest free block, bytes
int		Q_Arena_Count (void);				// number of blocks (free + used)
int		Q_Arena_Contains (void *p);			// 1 if p is inside the allocator region

#endif