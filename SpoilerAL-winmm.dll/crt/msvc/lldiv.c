//***
//lldiv.c - signed long divide routine
//
//       Copyright (c) Microsoft Corporation. All rights reserved.
//
//Purpose:
//       defines the signed long divide routine
//           __alldiv
//
//*******************************************************************************

#define LOWORD(x) [x]
#define HIWORD(x) [(x) + 4]

//***
//lldiv - signed long divide
//
//Purpose:
//       Does a signed long divide of the arguments.  Arguments are
//       not changed.
//
//Entry:
//       Arguments are passed on the stack:
//               1st pushed: divisor (QWORD)
//               2nd pushed: dividend (QWORD)
//
//Exit:
//       EDX:EAX contains the quotient (dividend/divisor)
//       NOTE: this routine removes the parameters from the stack.
//
//Uses:
//       ECX
//
//Exceptions:
//
//*******************************************************************************

#ifdef CFUNCTION
#define __cdecl __fastcall
#define _alldiv _lldiv
#endif

__declspec(naked) __int64 __cdecl _alldiv(__int64 dividend, __int64 divisor)
{
#if 0
	__asm
	{
		push    edi
		push    esi
		push    ebx

		// Set up the local stack and save the index registers.  When this is done
		// the stack frame will look as follows (assuming that the expression a/b will
		// generate a call to lldiv(a, b)):
		//
		//               -----------------
		//               |               |
		//               |---------------|
		//               |               |
		//               |--divisor (b)--|
		//               |               |
		//               |---------------|
		//               |               |
		//               |--dividend (a)-|
		//               |               |
		//               |---------------|
		//               | return addr** |
		//               |---------------|
		//               |      EDI      |
		//               |---------------|
		//               |      ESI      |
		//               |---------------|
		//       ESP---->|      EBX      |
		//               -----------------
		//

		#define DVND (esp + 16)             // stack address of dividend (a)
		#define DVSR (esp + 24)             // stack address of divisor (b)


		// Determine sign of the result (edi = 0 if result is positive, non-zero
		// otherwise) and make operands positive.

		xor     edi, edi                    // result sign assumed positive

		mov     eax, HIWORD(DVND)           // hi word of a
		or      eax, eax                    // test to see if signed
		jge     short L1                    // skip rest if a is already positive
		inc     edi                         // complement result sign flag
		mov     edx, LOWORD(DVND)           // lo word of a
		neg     eax                         // make a positive
		neg     edx
		sbb     eax, 0
		mov     HIWORD(DVND), eax           // save positive value
		mov     LOWORD(DVND), edx
	L1:
		mov     eax, HIWORD(DVSR)           // hi word of b
		or      eax, eax                    // test to see if signed
		jge     short L2                    // skip rest if b is already positive
		inc     edi                         // complement the result sign flag
		mov     edx, LOWORD(DVSR)           // lo word of a
		neg     eax                         // make b positive
		neg     edx
		sbb     eax,0
		mov     HIWORD(DVSR), eax           // save positive value
		mov     LOWORD(DVSR), edx
	L2:

		//
		// Now do the divide.  First look to see if the divisor is less than 4194304K.
		// If so, then we can use a simple algorithm with word divides, otherwise
		// things get a little more complex.
		//
		// NOTE - eax currently contains the high order word of DVSR
		//

		or      eax, eax                    // check to see if divisor < 4194304K
		jnz     short L3                    // nope, gotta do this the hard way
		mov     ecx, LOWORD(DVSR)           // load divisor
		mov     eax, HIWORD(DVND)           // load high word of dividend
		xor     edx, edx
		div     ecx                         // eax <- high order bits of quotient
		mov     ebx, eax                    // save high bits of quotient
		mov     eax, LOWORD(DVND)           // edx:eax <- remainder:lo word of dividend
		div     ecx                         // eax <- low order bits of quotient
		mov     edx, ebx                    // edx:eax <- quotient
		jmp     short L4                    // set sign, restore stack and return

		//
		// Here we do it the hard way.  Remember, eax contains the high word of DVSR
		//

	L3:
		mov     ebx, eax                    // ebx:ecx <- divisor
		mov     ecx, LOWORD(DVSR)
		mov     edx, HIWORD(DVND)           // edx:eax <- dividend
		mov     eax, LOWORD(DVND)
	L5:
		shr     ebx, 1                      // shift divisor right one bit
		rcr     ecx, 1
		shr     edx, 1                      // shift dividend right one bit
		rcr     eax, 1
		or      ebx, ebx
		jnz     short L5                    // loop until divisor < 4194304K
		div     ecx                         // now divide, ignore remainder
		mov     esi, eax                    // save quotient

		//
		// We may be off by one, so to check, we will multiply the quotient
		// by the divisor and check the result against the orignal dividend
		// Note that we must also check for overflow, which can occur if the
		// dividend is close to 2**64 and the quotient is off by 1.
		//

		mul     dword ptr HIWORD(DVSR)      // QUOT * HIWORD(DVSR)
		mov     ecx, eax
		mov     eax, LOWORD(DVSR)
		mul     esi                         // QUOT * LOWORD(DVSR)
		add     edx, ecx                    // EDX:EAX = QUOT * DVSR
		jc      short L6                    // carry means Quotient is off by 1

		//
		// do long compare here between original dividend and the result of the
		// multiply in edx:eax.  If original is larger or equal, we are ok, otherwise
		// subtract one (1) from the quotient.
		//

		cmp     edx, HIWORD(DVND)           // compare hi words of result and original
		ja      short L6                    // if result > original, do subtract
		jb      short L7                    // if result < original, we are ok
		cmp     eax, LOWORD(DVND)           // hi words are equal, compare lo words
		jbe     short L7                    // if less or equal we are ok, else subtract
	L6:
		dec     esi                         // subtract 1 from quotient
	L7:
		xor     edx, edx                    // edx:eax <- quotient
		mov     eax, esi

		//
		// Just the cleanup left to do.  edx:eax contains the quotient.  Set the sign
		// according to the save value, cleanup the stack, and return.
		//

	L4:
		dec     edi                         // check to see if result is negative
		jnz     short L8                    // if EDI == 0, result should be negative
		neg     edx                         // otherwise, negate the result
		neg     eax
		sbb     edx, 0

		//
		// Restore the saved registers and return.
		//

	L8:
		pop     ebx
		pop     esi
		pop     edi

		ret     16

		#undef DVND
		#undef DVSR
	}
#else
	__asm
	{
		push    ebx
		push    ebp
		push    esi
		push    edi

		// Set up the local stack and save the index registers.  When this is done
		// the stack frame will look as follows (assuming that the expression a/b will
		// generate a call to lldiv(a, b)):
		//
		//               -----------------
		//               |               |
		//               |---------------|
		//               |               |
		//               |--divisor (b)--|
		//               |               |
		//               |---------------|
		//               |               |
		//               |--dividend (a)-|
		//               |               |
		//               |---------------|
		//               | return addr** |
		//               |---------------|
		//               |      EBX      |
		//               |---------------|
		//               |      EBP      |
		//               |---------------|
		//               |      ESI      |
		//               |---------------|
		//       ESP---->|      EDI      |
		//               -----------------
		//

		#define DVND (esp + 20)             // stack address of dividend (a)
		#define DVSR (esp + 28)             // stack address of divisor (b)

		// Determine sign of the result (EDI = 0 if result is positive, non-zero
		// otherwise) and make operands positive.

		mov     edi, HIWORD(DVND)           // load dividend
		mov     ecx, HIWORD(DVSR)           // load divisor
		mov     ebx, LOWORD(DVND)
		mov     eax, edi
		sar     edi, 31
		mov     edx, ecx
		sar     ecx, 31
		mov     esi, LOWORD(DVSR)
		xor     eax, edi
		add     ebx, edi
		sbb     eax, edi
		xor     ebx, edi
		xor     edx, ecx
		add     esi, ecx
		sbb     edx, ecx
		xor     esi, ecx
		xor     edi, ecx
		mov     ebp, eax

		//
		// Now do the divide.  First look to see if the divisor is less than 4194304K.
		// If so, then we can use a simple algorithm with word divides, otherwise
		// things get a little more complex.
		//
		// NOTE - EDX currently contains the high order word of DVSR
		//

		test    edx, edx                    // check to see if divisor < 4194304K
		jnz     hard                        // nope, gotta do this the hard way
		div     esi                         // EAX <- high order bits of quotient
		mov     ecx, eax                    // save high bits of quotient
		mov     eax, ebx                    // EDX:EAX <- remainder:lo word of dividend
		div     esi                         // EAX <- low order bits of quotient
		mov     edx, ecx                    // EDX:EAX <- quotient
		jmp     epilog                      // negate result, restore stack and return

		align   16
	hard:
		//
		// Here we do it the hard way.  Remember, EDX contains the high word of DVSR
		//

		lea     ecx, [edx + edx]
		jns     shift
		shr     eax, 31
		xor     edx, edx                    // EDX:EAX = quotient
		jmp     epilog                      // negate result, restore stack and return

		align   16
	shift:
		push    esi                         // save positive value
		push    edx

		#define DVNDLO ebx
		#define DVNDHI ebp
		#define DVSRLO (esp +  4)
		#define DVSRHI (esp     )

		bsr     ecx, ecx
		shrd    esi, edx, cl
		mov     edx, eax                    // EDX:EAX <- dividend
		mov     eax, ebx
		shrd    eax, edx, cl
		shr     edx, cl
		div     esi                         // now divide, ignore remainder

		//
		// We may be off by one, so to check, we will multiply the quotient
		// by the divisor and check the result against the orignal dividend
		// Note that we must also check for overflow, which can occur if the
		// dividend is close to 2**64 and the quotient is off by 1.
		//

		pop     ecx                         // ECX <- DVSRHI
		pop     edx                         // EDX <- DVSRLO
		mov     esi, eax                    // save quotient
		imul    ecx, eax                    // QUOT * HIWORD(DVSR)
		mul     edx                         // QUOT * LOWORD(DVSR)

		//
		// do long compare here between original dividend and the result of the
		// multiply in EDX:EAX.  If original is larger or equal, we are ok, otherwise
		// subtract one (1) from the quotient.
		//

		add     edx, ecx                    // EDX:EAX = QUOT * DVSR
		cmp     ebx, eax
		sbb     ebp, edx                    // if dividend < product, do subtract
		mov     eax, esi
		sbb     eax, 0                      // subtract carry flag from quotient
		xor     edx, edx

		#undef DVNDLO
		#undef DVNDHI
		#undef DVSRLO
		#undef DVSRHI

	epilog:
		//
		// Just the cleanup left to do.  EDX:EAX contains the quotient.  Set the sign
		// according to the save value, cleanup the stack, and return.
		//

		xor     edx, edi                    // if EDI == -1, negate the quotient
		add     eax, edi
		sbb     edx, edi
		xor     eax, edi

		//
		// Restore the saved registers and return.
		//

		pop     edi
		pop     esi
		pop     ebp
		pop     ebx

		ret     16

		#undef DVND
		#undef DVSR
	}
#endif
}
