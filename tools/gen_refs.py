import chess

# new (non-standard) positions; standard ones use published values in C
positions = [
    ("en passant",      "8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3", 5),
    ("ep pin / checks",  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 b - - 0 1", 5),
    ("promotions",      "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 4),
]

def perft(b, d):
    if d == 0:
        return 1
    n = 0
    for m in b.legal_moves:
        b.push(m)
        n += perft(b, d - 1)
        b.pop()
    return n

for name, fen, maxd in positions:
    board = chess.Board(fen)
    counts = [perft(board, d) for d in range(1, maxd + 1)]
    print(f"{name}: {counts}")
