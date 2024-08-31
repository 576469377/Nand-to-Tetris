// Mult.asm
// input: R0, R1 (R0 >= 0, R1 >= 0, R0 * R1 < 32768)
// output: R2 = R0 * R1

// //pseudo code
// for (i = 1; i <= R0; ++i) {
//     R2 = R2 + R1
// }

@i
M=1 // i = 1
@R2
M=0 // R2 = 0

(LOOP)
  @i
  D=M
  @R0
  D=D-M
  @END
  D;JGT // if i - R0 > 0
  @R1
  D=M
  @R2
  M=D+M // R2 = R2 + R1
  @i
  M=M+1 // i = i + 1
  @LOOP
  0;JMP

(END)
  @END
  0;JMP
