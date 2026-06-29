import sys
import chess

fen = sys.argv[1] if len(sys.argv) > 1 else chess.STARTING_FEN
depth = int(sys.argv[2]) if len(sys.argv) > 2 else 2

board = chess.Board(fen)

def perft(b, d):
    if d == 0:
        return 1
    n = 0
    for m in b.legal_moves:
        b.push(m)
        n += perft(b, d - 1)
        b.pop()
    return n

total = 0
for m in sorted(board.legal_moves, key=lambda x: x.uci()):
    board.push(m)
    c = perft(board, depth - 1)
    board.pop()
    total += c
    print(f"{m.uci()}: {c}")
print("total:", total)
