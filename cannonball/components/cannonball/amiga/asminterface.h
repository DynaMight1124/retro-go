/*
  asminterface.h

  Henryk Richter <henryk.richter@gmx.net>

  Interface macros for ASM subroutines for GCC and SAS/C

  syntax:
   ASM SAVEDS int some_asm_subroutine(   
                         ASMR(d3) unsigned int   some_data    ASMREG(d3),
                         ASMR(a0) unsigned char *some_address ASMREG(a0) 
			             );

  Reason for the double spec of An/Dn: some compilers (SAS) require the register
  on the left hand side, gcc on the right hand side. I wanted to avoid a big macro
  for the whole argument and moved the stuff into two macros per argument.

*/
#ifndef _INC_ASMINTERFACE_H
#define _INC_ASMINTERFACE_H

#ifdef __SASC

#define ASM __asm
#define ASMR(x) register __ ## x 
#define ASMREG(x) 
#define SAVEDS __saveds

#else /* __SASC */

#ifdef __GNUC__

#define ASM 
#define ASMR(x) register
#define ASMREG(x) __asm("" #x "")
#define SAVEDS __saveds

#else /* __GNUC__ */

#error "Compiler not supported yet in asminterface.h"

#endif /* __GNUC__ */
#endif /* __SASC */


#endif /* _INC_ASMINTERFACE_H */
