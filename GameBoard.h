#pragma once
#include "Header.h"
#include "Move.h"
#include<array>
#include<vector>

struct GameBoard
{
    std::array<pieceType, 64> whitePieceArray{ EMPTY };
    std::array<pieceType, 64> blackPieceArray{ EMPTY };
    Bitboard whites[6], blacks[6];
    Bitboard occupied, blackPieces, whitePieces;
    bool turn; 
    bool wOO, wOOO, bOO, bOOO; 
    bool inDoubleCheck;
    Bitboard checksFrom;
    Bitboard enPassantTarget;
    int plyCount, ply50MoveRule;
    unsigned long wKingPos, bKingPos;
    Bitboard whiteKingRookMoves, whiteKingBishopMoves, blackKingRookMoves, blackKingBishopMoves;
    std::vector<uint64_t> positionHistory;
    std::vector<Move> moveHistory;
    GameBoard();
    bool setFromFen(const std::string& fen);

    Bitboard wpPushes();
    Bitboard wpDoublePushes();
    Bitboard wpRightCaptures();
    Bitboard wpLeftCaptures();
    Bitboard wpPromotions();
    Bitboard wpRightPromotions();
    Bitboard wpLeftPromotions();

    Bitboard bpPushes();
    Bitboard bpDoublePushes();
    Bitboard bpRightCaptures();
    Bitboard bpLeftCaptures();
    Bitboard bpPromotions();
    Bitboard bpRightPromotions();
    Bitboard bpLeftPromotions();

    Bitboard singleWhitePawnAttacks(unsigned long index);
    Bitboard singleBlackPawnAttacks(unsigned long index);

    Bitboard whiteAttacks();
    Bitboard blackAttacks();

    // determines whether white/black king is in discovered check after moving a piece from index

    /*unsigned long wDiscovedFrom(unsigned long moved) {
        if(!((bit << moved) & (whiteKingBishopMoves | whiteKingRookMoves))) return 64;
        unsigned char result;
        unsigned long result_index;
        Bitboard currentChecks = (RookMagicBitboards[wKingPos][(occupied & RawRookAttacks[wKingPos]) * RookMagics[wKingPos] >> RookShifts[wKingPos]]) & (blacks[ROOK] | blacks[QUEEN]);
        while (currentChecks) {
            result = _BitScanForward64(&result_index, currentChecks);
            if (result && result_index != checkedFrom) return result_index;
            currentChecks &= currentChecks - 1;
        }
        currentChecks = (BishopMagicBitboards[wKingPos][(occupied & RawBishopAttacks[wKingPos]) * BishopMagics[wKingPos] >> BishopShifts[wKingPos]]) & (blacks[BISHOP] | blacks[QUEEN]);
        while (currentChecks) {
            result = _BitScanForward64(&result_index, currentChecks);
            if (result && result_index != checkedFrom) return result_index;
            currentChecks &= currentChecks - 1;
        }
        return 64;
    }
    unsigned long bDiscovedFrom(unsigned long moved) {
        if(!((bit << moved) & (blackKingBishopMoves | blackKingRookMoves))) return 64;
        unsigned char result;
        unsigned long result_index;
        Bitboard currentChecks = (RookMagicBitboards[bKingPos][(occupied & RawRookAttacks[bKingPos]) * RookMagics[bKingPos] >> RookShifts[bKingPos]]) & (whites[ROOK] | whites[QUEEN]);
        while (currentChecks) {
            result = _BitScanForward64(&result_index, currentChecks);
            if (result && result_index != checkedFrom) return result_index;
            currentChecks &= currentChecks - 1;
        }
        currentChecks = (BishopMagicBitboards[bKingPos][(occupied & RawBishopAttacks[bKingPos]) * BishopMagics[bKingPos] >> BishopShifts[bKingPos]]) & (whites[BISHOP] | whites[QUEEN]);
        while (currentChecks) {
            result = _BitScanForward64(&result_index, currentChecks);
            if (result && result_index != checkedFrom) return result_index;
            currentChecks &= currentChecks - 1;
        }
        return 64;
    }
*/
    Bitboard wDiscoveredFrom(unsigned long moved);
    Bitboard bDiscoveredFrom(unsigned long moved);

    // mask is a block check bath, or all squares if not in check
    void generateWhitePromotions(std::vector<Move>& moves, Bitboard mask);
    void generateBlackPromotions(std::vector<Move>& moves, Bitboard mask);

    void generateWhiteMoves(std::vector<Move>& moves, Bitboard mask);
    void generateBlackMoves(std::vector<Move>& moves, Bitboard mask);

    void generateWhiteKingMoves(std::vector<Move>& moves, bool capturesOnly);
    void generateBlackKingMoves(std::vector<Move>& moves, bool capturesOnly);

    void generateWhiteCastles(std::vector<Move>& moves);
    void generateBlackCastles(std::vector<Move>& moves);

    std::vector<Move> allMoves();
    std::vector<Move> noisyMoves();

    void doMove(Move& m);
    void undoMove(Move& m);

    bool isRepetition() const;

    long perft(int depth);

    std::string notation_from_index(unsigned long index) const;
    void printBoard() const;
    uint64_t computeZobristKey() const;
    uint64_t reserveKey() const;

    std::string rankToFEN(int rank) const;
    std::string computeFEN() const;
};

