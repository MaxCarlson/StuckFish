#ifndef COMMON_H
#define COMMON_H

#include <cstdint>

typedef uint64_t U64;

static const int kSquares = 64;

//paths to magics file locations
static const char kRookMagics[] = "Magics/rook_magics.magic";
static const char kRookMasks[] = "Magics/rook_masks.magic";
static const char kRookShifts[] = "Magics/rook_shifts.magic";
static const char kRookOffsets[] = "Magics/rook_offsets.magic";
static const char kRookAttackTable[] = "Magics/rook_attack_table.magic";

static const char kBishopMagics[] = "Magics/bishop_magics.magic";
static const char kBishopMasks[] = "Magics/bishop_masks.magic";
static const char kBishopShifts[] = "Magics/bishop_shifts.magic";
static const char kBishopOffsets[] = "Magics/bishop_offsets.magic";
static const char kBishopAttackTable[] = "Magics/bishop_attack_table.magic";


/* //for standalone on my local system
static const char kRookMagics[] = "C:/Users/C-60/Desktop/QtPrograms/Advanced-Qt-Chess/Magics/rook_magics.magic";
static const char kRookMasks[] = "C:/Users/C-60/Desktop/QtPrograms/Advanced-Qt-Chess/Magics/rook_masks.magic";
static const char kRookShifts[] = "C:/Users/C-60/Desktop/QtPrograms/Advanced-Qt-Chess/Magics/rook_shifts.magic";
static const char kRookOffsets[] = "C:/Users/C-60/Desktop/QtPrograms/Advanced-Qt-Chess/Magics/rook_offsets.magic";
static const char kRookAttackTable[] = "C:/Users/C-60/Desktop/QtPrograms/Advanced-Qt-Chess/Magics/rook_attack_table.magic";

static const char kBishopMagics[] = "C:/Users/C-60/Desktop/QtPrograms/Advanced-Qt-Chess/Magics/bishop_magics.magic";
static const char kBishopMasks[] = "C:/Users/C-60/Desktop/QtPrograms/Advanced-Qt-Chess/Magics/bishop_masks.magic";
static const char kBishopShifts[] = "C:/Users/C-60/Desktop/QtPrograms/Advanced-Qt-Chess/Magics/bishop_shifts.magic";
static const char kBishopOffsets[] = "C:/Users/C-60/Desktop/QtPrograms/Advanced-Qt-Chess/Magics/bishop_offsets.magic";
static const char kBishopAttackTable[] = "C:/Users/C-60/Desktop/QtPrograms/Advanced-Qt-Chess/Magics/bishop_attack_table.magic";
*/
#endif
