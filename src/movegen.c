#include <stdint.h>
#include "movegen.h"
#include "game.h"
#include "bitboard.h"
#include "piece.h"
#include "player.h"
#include "move.h"
#include "movelist.h"

/* Bitboard legal move generator.
 *
 * Strategy: for the side to move, enumerate pseudo-legal destinations for each
 * piece from the precomputed / magic attack tables, then keep only the moves
 * that leave our own king safe. The king-safety test (move_is_legal) recomputes
 * the post-move occupancy and asks whether the king square is attacked, which
 * handles pins, check evasions and the tricky en passant discovered-check case
 * uniformly without a full board copy.
 */

/* Packs a move into the uint32_t encoding from move.h. */
static inline uint32_t mk(int from, int to, uint32_t flags) {
    return ((uint32_t) from) | (((uint32_t) to) << 8) | flags;
} /* mk */


/* Core attack test: is 'sq' attacked by color 'by', given board occupancy 'occ'
 * and 'by_mask' (bits allowed for attacking pieces, used to exclude a piece that
 * is being captured this move). Kept private; callers use the wrappers below.
 */
static int square_attacked_ex(const Game *game, int sq, int by,
                              Bitboard occ, Bitboard by_mask) {
    Bitboard pawns;     // attacking pawns still on the board
    Bitboard knights;   // attacking knights
    Bitboard bishops;   // attacking bishops + queens (diagonal)
    Bitboard rooks;     // attacking rooks + queens (orthogonal)
    Bitboard king;      // attacking king

    pawns   = game->pieces[by][PAWN]   & by_mask;
    knights = game->pieces[by][KNIGHT] & by_mask;
    bishops = (game->pieces[by][BISHOP] | game->pieces[by][QUEEN]) & by_mask;
    rooks   = (game->pieces[by][ROOK]   | game->pieces[by][QUEEN]) & by_mask;
    king    = game->pieces[by][KING];

    // a 'by' pawn attacks sq from the squares an opposite-color pawn on sq hits
    if (pawn_attacks[by ^ 1][sq] & pawns) {
        return 1;
    }
    if (knight_attacks[sq] & knights) {
        return 1;
    }
    if (king_attacks[sq] & king) {
        return 1;
    }
    if (bishop_attacks(sq, occ) & bishops) {
        return 1;
    }
    if (rook_attacks(sq, occ) & rooks) {
        return 1;
    }
    return 0;
} /* square_attacked_ex */


uint8_t square_attacked(Game *game, uint8_t sq, int by) {
    return (uint8_t) square_attacked_ex(game, sq, by, game->occ_all, ~(Bitboard) 0);
} /* square_attacked */


/* Returns non-zero if the given (pseudo-legal) move leaves our king safe.
 * 'piece_type' is the moving piece's type (KING is special-cased since the king
 * itself relocates); 'is_ep' marks an en passant capture whose captured pawn is
 * not on the destination square.
 */
static int move_is_legal(const Game *game, int from, int to, int piece_type,
                         int us, int them, int king_sq, int is_ep) {
    Bitboard occ_after; // board occupancy after the move
    Bitboard by_mask;   // attackers to consider (a captured piece is removed)
    int cap_sq;         // square of the captured piece, or -1
    int ksq;            // our king square after the move

    // slide the mover from 'from' to 'to'
    occ_after = (game->occ_all & ~BB_SQ(from)) | BB_SQ(to);
    by_mask = ~(Bitboard) 0;
    cap_sq = -1;

    if (is_ep) {
        // en passant: the captured pawn sits beside the mover, not on 'to'
        cap_sq = (us == BB_WHITE) ? to - 8 : to + 8;
        occ_after &= ~BB_SQ(cap_sq);
    }
    else if (game->occ[them] & BB_SQ(to)) {
        // ordinary capture on the destination square
        cap_sq = to;
    }

    if (cap_sq >= 0) {
        by_mask = ~BB_SQ(cap_sq);   // a captured piece cannot give check
    }

    ksq = (piece_type == KING) ? to : king_sq;
    return !square_attacked_ex(game, ksq, them, occ_after, by_mask);
} /* move_is_legal */


/* Adds all four promotion moves (knight, bishop, rook, queen) for a pawn
 * reaching the last rank, in the same order as the old generator.
 */
static void add_promotions(MoveList *list, int from, int to, uint32_t color_flag) {
    add_move(list, mk(from, to, color_flag | MOVE_NP_FLAG_MASK));
    add_move(list, mk(from, to, color_flag | MOVE_BP_FLAG_MASK));
    add_move(list, mk(from, to, color_flag | MOVE_RP_FLAG_MASK));
    add_move(list, mk(from, to, color_flag | MOVE_QP_FLAG_MASK));
} /* add_promotions */


/* Generates pawn pushes, captures, promotions and en passant. */
static void gen_pawns(Game *game, MoveList *list, int us, int them,
                      int king_sq, uint32_t color_flag) {
    Bitboard pawns;     // remaining pawns to process
    Bitboard occ;       // full occupancy
    Bitboard caps;      // capture destinations for one pawn
    int forward;        // index delta for a one-square push
    int start_rank;     // rank from which a double push is allowed
    int promo_rank;     // rank on which a pawn promotes
    uint8_t ep_sq;      // en passant target square (0xFF if none)
    int from, to, to2;  // source / destination squares
    int rank;           // source rank

    pawns = game->pieces[us][PAWN];
    occ = game->occ_all;
    forward    = (us == BB_WHITE) ? 8 : -8;
    start_rank = (us == BB_WHITE) ? 1 : 6;
    promo_rank = (us == BB_WHITE) ? 7 : 0;
    ep_sq = (us == BB_WHITE) ? game->w_en_passant_square
                             : game->b_en_passant_square;

    while (pawns) {
        from = bb_pop_lsb(&pawns);
        rank = from / 8;

        // forward push onto an empty square (with promotion / double push)
        to = from + forward;
        if (!(occ & BB_SQ(to))) {
            if (to / 8 == promo_rank) {
                if (move_is_legal(game, from, to, PAWN, us, them, king_sq, 0)) {
                    add_promotions(list, from, to, color_flag);
                }
            }
            else {
                if (move_is_legal(game, from, to, PAWN, us, them, king_sq, 0)) {
                    add_move(list, mk(from, to, color_flag));
                }
                // double push from the starting rank if both squares are empty
                if (rank == start_rank) {
                    to2 = from + 2 * forward;
                    if (!(occ & BB_SQ(to2))
                        && move_is_legal(game, from, to2, PAWN, us, them, king_sq, 0)) {
                        add_move(list, mk(from, to2, color_flag));
                    }
                }
            }
        }

        // diagonal captures onto enemy-occupied squares
        caps = pawn_attacks[us][from] & game->occ[them];
        while (caps) {
            to = bb_pop_lsb(&caps);
            if (to / 8 == promo_rank) {
                if (move_is_legal(game, from, to, PAWN, us, them, king_sq, 0)) {
                    add_promotions(list, from, to, color_flag);
                }
            }
            else if (move_is_legal(game, from, to, PAWN, us, them, king_sq, 0)) {
                add_move(list, mk(from, to, color_flag));
            }
        }

        // en passant capture onto the target square
        if (ep_sq <= 63 && (pawn_attacks[us][from] & BB_SQ(ep_sq))) {
            if (move_is_legal(game, from, ep_sq, PAWN, us, them, king_sq, 1)) {
                add_move(list, mk(from, ep_sq, color_flag));
            }
        }
    }
} /* gen_pawns */


/* Generates moves for a set of leaper/slider pieces sharing an attack function.
 * 'attacks' is the destination set for one piece; this helper is called per
 * piece type with that set already computed.
 */
static void gen_piece_moves(Game *game, MoveList *list, int from, Bitboard attacks,
                            int piece_type, int us, int them, int king_sq,
                            uint32_t color_flag) {
    int to;

    attacks &= ~game->occ[us];      // cannot land on our own pieces
    while (attacks) {
        to = bb_pop_lsb(&attacks);
        if (move_is_legal(game, from, to, piece_type, us, them, king_sq, 0)) {
            add_move(list, mk(from, to, color_flag));
        }
    }
} /* gen_piece_moves */


/* Generates knight, bishop, rook, queen and (non-castling) king moves. */
static void gen_pieces(Game *game, MoveList *list, int us, int them,
                       int king_sq, uint32_t color_flag) {
    Bitboard occ;   // full occupancy for slider lookups
    Bitboard bb;    // pieces of one type left to process
    int from;       // source square

    occ = game->occ_all;

    bb = game->pieces[us][KNIGHT];
    while (bb) {
        from = bb_pop_lsb(&bb);
        gen_piece_moves(game, list, from, knight_attacks[from],
                        KNIGHT, us, them, king_sq, color_flag);
    }

    bb = game->pieces[us][BISHOP];
    while (bb) {
        from = bb_pop_lsb(&bb);
        gen_piece_moves(game, list, from, bishop_attacks(from, occ),
                        BISHOP, us, them, king_sq, color_flag);
    }

    bb = game->pieces[us][ROOK];
    while (bb) {
        from = bb_pop_lsb(&bb);
        gen_piece_moves(game, list, from, rook_attacks(from, occ),
                        ROOK, us, them, king_sq, color_flag);
    }

    bb = game->pieces[us][QUEEN];
    while (bb) {
        from = bb_pop_lsb(&bb);
        gen_piece_moves(game, list, from, queen_attacks(from, occ),
                        QUEEN, us, them, king_sq, color_flag);
    }

    // the (single) king's step moves; castling is handled separately
    gen_piece_moves(game, list, king_sq, king_attacks[king_sq],
                    KING, us, them, king_sq, color_flag);
} /* gen_pieces */


/* Generates the two castling moves when legal: correct rights, an empty path,
 * and the king not moving out of, through, or into an attacked square.
 */
static void gen_castles(Game *game, MoveList *list, int us, int them,
                        int king_sq, uint32_t color_flag) {
    Bitboard occ;   // full occupancy

    occ = game->occ_all;

    // may not castle while in check
    if (square_attacked(game, king_sq, them)) {
        return;
    }

    if (us == BB_WHITE) {
        // kingside: f1,g1 empty and unattacked
        if (game->castle_kingside_w
            && !(occ & (BB_SQ(5) | BB_SQ(6)))
            && !square_attacked(game, 5, them)
            && !square_attacked(game, 6, them)) {
            add_move(list, mk(4, 6, color_flag | MOVE_KSC_FLAG_MASK));
        }
        // queenside: b1,c1,d1 empty and c1,d1 unattacked
        if (game->castle_queenside_w
            && !(occ & (BB_SQ(1) | BB_SQ(2) | BB_SQ(3)))
            && !square_attacked(game, 2, them)
            && !square_attacked(game, 3, them)) {
            add_move(list, mk(4, 2, color_flag | MOVE_QSC_FLAG_MASK));
        }
    }
    else {
        // kingside: f8,g8 empty and unattacked
        if (game->castle_kingside_b
            && !(occ & (BB_SQ(61) | BB_SQ(62)))
            && !square_attacked(game, 61, them)
            && !square_attacked(game, 62, them)) {
            add_move(list, mk(60, 62, color_flag | MOVE_KSC_FLAG_MASK));
        }
        // queenside: b8,c8,d8 empty and c8,d8 unattacked
        if (game->castle_queenside_b
            && !(occ & (BB_SQ(57) | BB_SQ(58) | BB_SQ(59)))
            && !square_attacked(game, 58, them)
            && !square_attacked(game, 59, them)) {
            add_move(list, mk(60, 58, color_flag | MOVE_QSC_FLAG_MASK));
        }
    }
} /* gen_castles */


void generate_moves(Game *game, MoveList *out) {
    int us;                 // side to move color index
    int them;               // opposing color index
    uint32_t color_flag;    // MOVE_WHITE_MASK / MOVE_BLACK_MASK for this side
    int king_sq;            // our king square

    clear_movelist(out);

    us = game->move ? BB_BLACK : BB_WHITE;
    them = us ^ 1;
    color_flag = (us == BB_WHITE) ? MOVE_WHITE_MASK : MOVE_BLACK_MASK;

    // no king means no legal position to generate for (guards bb_lsb on 0)
    if (!game->pieces[us][KING]) {
        return;
    }
    king_sq = bb_lsb(game->pieces[us][KING]);

    gen_pawns(game, out, us, them, king_sq, color_flag);
    gen_pieces(game, out, us, them, king_sq, color_flag);
    gen_castles(game, out, us, them, king_sq, color_flag);
} /* generate_moves */


MoveList *get_possible_moves(Game *game) {
    MoveList *possible_moves = create_movelist();
    generate_moves(game, possible_moves);
    return possible_moves;
} /* get_possible_moves */


uint8_t is_in_check(Game *game, uint8_t player) {
    int us;             // color index of 'player'
    int them;           // opposing color
    Bitboard king;      // that player's king bitboard

    us = (player == PLAYERW) ? BB_WHITE : BB_BLACK;
    them = us ^ 1;
    king = game->pieces[us][KING];
    if (!king) {
        return 0;
    }
    return square_attacked(game, (uint8_t) bb_lsb(king), them);
} /* is_in_check */


uint8_t is_checkmate(Game *game) {
    uint8_t player;         // side to move
    MoveList possible_moves;

    player = !game->move ? PLAYERW : PLAYERB;
    generate_moves(game, &possible_moves);

    // no legal moves while in check is checkmate
    if (possible_moves.length == 0 && is_in_check(game, player)) {
        return CHECKMATE;
    }
    return 0;
} /* is_checkmate */


uint8_t is_draw(Game *game) {
    uint8_t player;         // side to move
    MoveList possible_moves;
    uint8_t pc_w;           // weighted white material count
    uint8_t pc_b;           // weighted black material count
    int i;                  // board scan index
    uint8_t piece;          // piece on the scanned square

    player = !game->move ? PLAYERW : PLAYERB;
    generate_moves(game, &possible_moves);

    // no legal moves while not in check is stalemate
    if (possible_moves.length == 0 && !is_in_check(game, player)) {
        return STALEMATE;
    }

    // draw by insufficient material: a lone king (or king + single minor) per
    // side. weights mirror the old rule -- anything above a minor counts as 2.
    pc_w = 0;
    pc_b = 0;
    for (i = 0; i < BOARD_SIZE; i++) {
        piece = game->board[i];
        if (piece & WHITE) {
            if (is_pawn(piece) || is_rook(piece) || is_queen(piece)) {
                pc_w += 2;
            }
            else if (is_knight(piece) || is_bishop(piece)) {
                pc_w += 1;
            }
        }
        else if (piece & BLACK) {
            if (is_pawn(piece) || is_rook(piece) || is_queen(piece)) {
                pc_b += 2;
            }
            else if (is_knight(piece) || is_bishop(piece)) {
                pc_b += 1;
            }
        }

        if (pc_w > 1 || pc_b > 1) {
            break;      // enough material on one side; not an automatic draw
        }
    }

    if (pc_w < 2 && pc_b < 2) {
        return DRAW_IM;
    }
    return 0;
} /* is_draw */
