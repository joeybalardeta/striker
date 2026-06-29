import sys
import chess

fen = sys.argv[1]
move_uci = sys.argv[2]

board = chess.Board(fen)
board.push(chess.Move.from_uci(move_uci))
print("FEN:", board.fen())
moves = sorted(m.uci() for m in board.legal_moves)
print("count:", len(moves))
print(" ".join(moves))
