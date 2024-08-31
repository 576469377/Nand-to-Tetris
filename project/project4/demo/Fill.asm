// Fill.asm
// screen:
// any key is pressed: black
// no key is pressed: white

// // pseudo code:
// for (i = 0;; ++i) {
//   if (key) {
//     RAM[screen] = 1111111111111111
//     screen += 1
//     i += 1
//     if (i >= 8192) {
//       i -= 1
//       screen -= 1
//       continue
//     }
//     continue
//   }
//   if (no_key) {
//     RAM[screen] = 0000000000000000
//     screen -= 1
//     i -= 1
//     if (i < 0) {
//       i += 1
//       screen += 1
//       continue
//     }
//     continue
//   }
// }

@SCREEN
D=A
@screen
M=D // screen = SCREEN

@i
M=0 // i = 0

(LOOP)
  @KBD
  D=M
  @KEY
  D;JNE // if (key != 0) goto KEY

  // NO_KEY
  @screen
  A=M
  M=0 // RAM[screen] = 0000000000000000

  @1
  D=A
  @screen
  M=M-D // screen = screen - 1

  @i
  M=M-1 // i = i - 1
    
  D=M
  @LOOP
  D;JGE // if (i >= 0) goto LOOP

  @i
  M=M+1
  @screen
  M=M+1 // if (i < 0) i = i + 1, screen = screen + 1
  @LOOP
  0;JMP

  (KEY)
    @screen
    A=M
    M=-1 // RAM[screen] = 1111111111111111

    @1
    D=A
    @screen
    M=D+M // screen = screen + 1

    @i
    M=M+1 // i = i + 1
    
    D=M
    @8192
    D=D-A
    @LOOP
    D;JLT // if (i - 8192 < 0) goto LOOP

    @i
    M=M-1
    @screen
    M=M-1 // if (i - 8192 >= 0) i = i - 1, screen = screen - 1
    @LOOP
    0;JMP