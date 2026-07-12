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


/* Returns a pinned piece's allowed movement ray (the full line through the king
 * and the piece), or all-ones when the piece is not pinned. A pinned piece may
 * only move along this line.
 */
static inline Bitboard pin_ray(int king_sq, int from, Bitboard pinned) {
    return (pinned & BB_SQ(from)) ? line_bb[king_sq][from] : ~(Bitboard) 0;
} /* pin_ray */


/* Generates pawn pushes, captures, promotions and en passant. Non-en-passant
 * moves are masked by 'check_mask' (block/capture squares when in check) and,
 * for a pinned pawn, its pin ray -- so no per-move king-safety test is needed.
 * En passant is rare and full of edge cases, so it is validated with the exact
 * move_is_legal recompute instead.
 */
static void gen_pawns(Game *game, MoveList *list, int us, int them, int king_sq,
                      uint32_t color_flag, Bitboard check_mask, Bitboard pinned) {
    Bitboard pawns;     // remaining pawns to process
    Bitboard occ;       // full occupancy
    Bitboard caps;      // capture destinations for one pawn
    Bitboard ray;       // this pawn's pin ray (all-ones if unpinned)
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
        ray = pin_ray(king_sq, from, pinned);

        // forward push onto an empty square
        to = from + forward;
        if (!(occ & BB_SQ(to))) {
            if (BB_SQ(to) & check_mask & ray) {
                if (to / 8 == promo_rank) {
                    add_promotions(list, from, to, color_flag);
                }
                else {
                    add_move(list, mk(from, to, color_flag));
                }
            }
            // double push (intermediate square already known empty)
            if (rank == start_rank) {
                to2 = from + 2 * forward;
                if (!(occ & BB_SQ(to2)) && (BB_SQ(to2) & check_mask & ray)) {
                    add_move(list, mk(from, to2, color_flag));
                }
            }
        }

        // diagonal captures onto enemy-occupied squares
        caps = pawn_attacks[us][from] & game->occ[them] & check_mask & ray;
        while (caps) {
            to = bb_pop_lsb(&caps);
            if (to / 8 == promo_rank) {
                add_promotions(list, from, to, color_flag);
            }
            else {
                add_move(list, mk(from, to, color_flag));
            }
        }

        // en passant capture onto the target square (fully validated)
        if (ep_sq <= 63 && (pawn_attacks[us][from] & BB_SQ(ep_sq))) {
            if (move_is_legal(game, from, ep_sq, PAWN, us, them, king_sq, 1)) {
                add_move(list, mk(from, ep_sq, color_flag));
            }
        }
    }
} /* gen_pawns */


/* Emits every destination in 'targets' as a move from 'from'. */
static void emit_targets(MoveList *list, int from, Bitboard targets,
                         uint32_t color_flag) {
    int to;

    while (targets) {
        to = bb_pop_lsb(&targets);
        add_move(list, mk(from, to, color_flag));
    }
} /* emit_targets */


/* Generates knight, bishop, rook and queen moves. Destinations are restricted
 * to 'check_mask' (block/capture the checker when in single check) and, for a
 * pinned piece, its pin ray. With those masks applied no per-move king-safety
 * test is required. A pinned knight can never move and is skipped.
 */
static void gen_pieces(Game *game, MoveList *list, int us, int them, int king_sq,
                       uint32_t color_flag, Bitboard check_mask, Bitboard pinned) {
    Bitboard occ;       // full occupancy for slider lookups
    Bitboard own;       // our pieces (cannot be captured)
    Bitboard bb;        // pieces of one type left to process
    Bitboard targets;   // legal destinations for one piece
    int from;           // source square

    occ = game->occ_all;
    own = game->occ[us];

    // knights (pinned knights excluded: a knight can never stay on the pin line)
    bb = game->pieces[us][KNIGHT] & ~pinned;
    while (bb) {
        from = bb_pop_lsb(&bb);
        targets = knight_attacks[from] & ~own & check_mask;
        emit_targets(list, from, targets, color_flag);
    }

    // bishops
    bb = game->pieces[us][BISHOP];
    while (bb) {
        from = bb_pop_lsb(&bb);
        targets = bishop_attacks(from, occ) & ~own & check_mask
                  & pin_ray(king_sq, from, pinned);
        emit_targets(list, from, targets, color_flag);
    }

    // rooks
    bb = game->pieces[us][ROOK];
    while (bb) {
        from = bb_pop_lsb(&bb);
        targets = rook_attacks(from, occ) & ~own & check_mask
                  & pin_ray(king_sq, from, pinned);
        emit_targets(list, from, targets, color_flag);
    }

    // queens
    bb = game->pieces[us][QUEEN];
    while (bb) {
        from = bb_pop_lsb(&bb);
        targets = queen_attacks(from, occ) & ~own & check_mask
                  & pin_ray(king_sq, from, pinned);
        emit_targets(list, from, targets, color_flag);
    }
} /* gen_pieces */


/* Generates the king's (non-castling) step moves. The king relocates, so each
 * destination is validated with move_is_legal, which recomputes attacks with the
 * king removed from occupancy (so it cannot step along a slider's check ray).
 */
static void gen_king(Game *game, MoveList *list, int us, int them, int king_sq,
                     uint32_t color_flag) {
    Bitboard targets;   // candidate king destinations
    int to;

    targets = king_attacks[king_sq] & ~game->occ[us];
    while (targets) {
        to = bb_pop_lsb(&targets);
        if (move_is_legal(game, king_sq, to, KING, us, them, king_sq, 0)) {
            add_move(list, mk(king_sq, to, color_flag));
        }
    }
} /* gen_king */


/* Generates the two castling moves when legal: correct rights, an empty path,
 * and the king not moving out of, through, or into an attacked square.
 */
static void gen_castles(Game *game, MoveList *list, int us, int them,
                        int king_sq, uint32_t color_flag) {
    Bitboard occ;   // full occupancy

    // caller only invokes this when the king is not in check
    (void) king_sq;
    occ = game->occ_all;

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
    Bitboard occ;           // full occupancy
    Bitboard own;           // our pieces
    Bitboard bishopsQ;      // enemy bishops + queens (diagonal sliders)
    Bitboard rooksQ;        // enemy rooks + queens (orthogonal sliders)
    Bitboard checkers;      // enemy pieces giving check
    Bitboard check_mask;    // squares a non-king piece may move to
    Bitboard pinned;        // our pinned pieces
    Bitboard snipers;       // enemy sliders aligned with the king
    int nc;                 // number of checkers
    int csq;                // the checking square (single check)
    int s;                  // a sniper square
    Bitboard blockers;      // pieces between king and a sniper

    clear_movelist(out);

    us = game->move ? BB_BLACK : BB_WHITE;
    them = us ^ 1;
    color_flag = (us == BB_WHITE) ? MOVE_WHITE_MASK : MOVE_BLACK_MASK;

    // no king means no legal position to generate for (guards bb_lsb on 0)
    if (!game->pieces[us][KING]) {
        return;
    }
    king_sq = bb_lsb(game->pieces[us][KING]);

    occ = game->occ_all;
    own = game->occ[us];
    bishopsQ = game->pieces[them][BISHOP] | game->pieces[them][QUEEN];
    rooksQ   = game->pieces[them][ROOK]   | game->pieces[them][QUEEN];

    // pieces currently giving check to our king
    checkers = (pawn_attacks[us][king_sq] & game->pieces[them][PAWN])
             | (knight_attacks[king_sq] & game->pieces[them][KNIGHT])
             | (bishop_attacks(king_sq, occ) & bishopsQ)
             | (rook_attacks(king_sq, occ) & rooksQ);
    nc = bb_popcount(checkers);

    // squares a non-king piece may move to: everywhere if not in check, else the
    // block-or-capture squares for a single checker, and nothing in double check
    if (nc == 0) {
        check_mask = ~(Bitboard) 0;
    }
    else if (nc == 1) {
        csq = bb_lsb(checkers);
        check_mask = between_bb[king_sq][csq] | checkers;
    }
    else {
        check_mask = 0;
    }

    // pinned pieces: our piece is the sole blocker between the king and an
    // aligned enemy slider
    pinned = 0;
    snipers = (rook_attacks(king_sq, 0) & rooksQ)
            | (bishop_attacks(king_sq, 0) & bishopsQ);
    while (snipers) {
        s = bb_pop_lsb(&snipers);
        blockers = between_bb[king_sq][s] & occ;
        if (blockers && (blockers & (blockers - 1)) == 0 && (blockers & own)) {
            pinned |= blockers;
        }
    }

    // in double check only the king may move; otherwise generate everything
    if (nc < 2) {
        gen_pawns(game, out, us, them, king_sq, color_flag, check_mask, pinned);
        gen_pieces(game, out, us, them, king_sq, color_flag, check_mask, pinned);
    }
    gen_king(game, out, us, them, king_sq, color_flag);

    // castling is only possible when not in check
    if (nc == 0) {
        gen_castles(game, out, us, them, king_sq, color_flag);
    }
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
