#ifndef MOVEH
#define MOVEH

// move bit map
// 32-bit
// bits 7:0     - from square (0-63)
// bits 15:8    - to square (0-63)
// bits 24:16   - flag bits
//  bit 16      - king side castle
//  bit 17      - queen side castle
//  bit 18      - en passant
//  bit 19      - knight pawn promotion
//  bit 20      - bishop pawn promotion
//  bit 21      - rook pawn promotion
//  bit 22      - queen pawn promotion
//  bit 23      - white move
//  bit 24      - black move
// bits 31:25   - reserved


#define MOVE_FROM_MASK(value) (value & 0xFF)
#define MOVE_TO_MASK(value) ((value >> 8) & 0xFF)
#define MOVE_FLAG_MASK(value) ((value >> 16) & 0xFFFF)

#define MOVE_KSC_FLAG_MASK  0x00010000  // bit 16
#define MOVE_QSC_FLAG_MASK  0x00020000  // bit 17
#define MOVE_EP_FLAG_MASK   0x00040000  // bit 18
#define MOVE_KPP_FLAG_MASK  0x00080000  // bit 19
#define MOVE_BPP_FLAG_MASK  0x000F0000  // bit 20
#define MOVE_RPP_FLAG_MASK  0x00100000  // bit 21
#define MOVE_QPP_FLAG_MASK  0x00200000  // bit 22
#define MOVE_WHITE_MASK     0x00400000  // bit 23
#define MOVE_BLACK_MASK     0x00800000  // bit 24

#endif
