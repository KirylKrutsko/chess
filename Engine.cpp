#include "Engine.h"
#include "Tables.h"
#include "TTEntry.h"
#include "GameBoard.h"
#include<iostream>
#include<vector>
#include<algorithm>
#include <chrono>
#include <string>

Engine::Engine(GameBoard& b) : board(b) {
    maxDepth = 30;
}

    int Engine::search(int depth, int alpha, int beta, long& nodesOnDepth, long& movesOnDepth, int& positionOnDepth) {
        // repetition and 50-move rule draw detection
        if (board.isDrawByRepetition() || board.ply50MoveRule >= 100) return 0;

        uint64_t key = board.computeZobristKey();
        TTEntry entry;
        if (TT.retrieve(key, entry)) {
            if ((entry.type == EXACT || (entry.type == LOWER && entry.eval >= beta) || (entry.type == UPPER && entry.eval <= alpha)) && (entry.depth >= depth)) {
                return entry.eval;
            }
        }

        nodesOnDepth++;
        std::vector<Move> moves = board.allMoves();
        if (moves.empty()) {
            if (board.checksFrom) { // mate
                return board.turn ? (-checkmateScore + board.plyCount) : (checkmateScore - board.plyCount);
            }
            return 0; // stalemate
        }

        movesOnDepth += moves.size();
        positionOnDepth++;

        if (depth == 0) {
            int eval = quienscenceSearch(alpha, beta, nodesOnDepth);
            return eval;
        }
        orderMoves(moves, key);
        Move bestMove = moves[0];
        int bestEval;
        EntryType nodeType = EXACT;

        if (board.turn) {
            bestEval = std::numeric_limits<int>::min();
            for (Move& move : moves) {
                board.doMove(move);
                int eval = search(depth - 1, alpha, beta, nodesOnDepth, movesOnDepth, positionOnDepth);
                board.undoMove(move);

                bestEval = std::max(bestEval, eval);
                alpha = std::max(alpha, eval);
                if (beta <= alpha) {
                    nodeType = UPPER;
                    return beta;  // Beta cut-off
                }
            }
            TT.insert(key, depth, bestEval, nodeType, bestMove);
            return bestEval;
        }
        else {
            bestEval = std::numeric_limits<int>::max();
            for (Move& move : moves) {
                board.doMove(move);
                int eval = search(depth - 1, alpha, beta, nodesOnDepth, movesOnDepth, positionOnDepth);
                board.undoMove(move);

                bestEval = std::min(bestEval, eval);
                beta = std::min(beta, eval);
                if (beta <= alpha) {
                    nodeType = LOWER;
                    return alpha;  // Alpha cut-off
                }
            }
            TT.insert(key, depth, bestEval, nodeType, bestMove);
            return bestEval;
        }
    }

    int Engine::quienscenceSearch(int alpha, int beta, long& nodesOnDepth) {
        // repetition and 50-move rule draw detection
        if (board.isDrawByRepetition() || board.ply50MoveRule >= 100) return 0;

        uint64_t key = board.computeZobristKey();
        TTEntry entry;
        if (TT.retrieve(key, entry)) {
             return entry.eval;
        }

        nodesOnDepth++;
        int standPad = evaluate();
        std::vector<Move> moves = board.noisyMoves();
        if (moves.empty()) {
            TT.insert(key, 0, standPad, EXACT);
            return standPad;
        }

        orderMoves(moves, key);
        Move bestMove;
        int bestEval;
        EntryType nodeType = EXACT;

        if (board.turn) {
            if (standPad >= beta) return beta;
            alpha = std::max(alpha, standPad);
            bestEval = alpha;
            for (Move& move : moves) {
                board.doMove(move);
                int eval = quienscenceSearch(alpha, beta, nodesOnDepth);
                board.undoMove(move);

                bestEval = std::max(bestEval, eval);
                alpha = std::max(alpha, eval);
                if (beta <= alpha) {
                    nodeType = UPPER;
                    return beta;  // Beta cut-off
                }
            }
            TT.insert(key, 0, bestEval, nodeType, bestMove);
            return bestEval;
        }
        else {
            if (standPad <= alpha) return alpha;
            beta = std::min(beta, standPad);
            bestEval = beta;
            for (Move& move : moves) {
                board.doMove(move);
                int eval = quienscenceSearch(alpha, beta, nodesOnDepth);
                board.undoMove(move);

                bestEval = std::min(bestEval, eval);
                beta = std::min(beta, eval);
                if (beta <= alpha) {
                    nodeType = LOWER;
                    return alpha;  // Alpha cut-off
                }
            }
            TT.insert(key, 0, bestEval, nodeType, bestMove);
            return bestEval;
        }
    }

    Move Engine::bestOnDepth(std::vector<Move>& moves, uint64_t key, int depth, int& eval, long& nodesOnDepth, long& movesOnDepth, int& positionOnDepth) {
        orderMoves(moves, key);
        Move bestMove;
        if (board.turn) {
            eval = std::numeric_limits<int>::min();
            for (Move& move : moves) {
                board.doMove(move);
                int moveValue = search(depth - 1, eval, std::numeric_limits<int>::max(), nodesOnDepth, movesOnDepth, positionOnDepth);
                board.undoMove(move);
                if (moveValue > eval) {
                    eval = moveValue;
                    bestMove = move;
                }
            }
        }
        else {
            eval = std::numeric_limits<int>::max();
            for (Move& move : moves) {
                board.doMove(move);
                int moveValue = search(depth - 1, std::numeric_limits<int>::min(), eval, nodesOnDepth, movesOnDepth, positionOnDepth);
                board.undoMove(move);
                if (moveValue < eval) {
                    eval = moveValue;
                    bestMove = move;
                }
            }
        }
        return bestMove;
    }

    Move Engine::iterativeDeepening() {
        std::cout << "\n";
        long nodesSearched = 0;
        std::vector<Move> moves = board.allMoves();
        if (moves.empty()) return Move();
        printMoves(moves);

        if (moves.size() == 1) {
            std::cout << "Nodes searched : 0\nEvaluation : ?\n";
            return moves[0];
        }

        long nodesOnDepth = 0;
        long movesOnDepth = moves.size();
        int positionOnDepth = 1;
        timer.start();

        // quick search for depth 1 :
        uint64_t key = board.computeZobristKey();
        int bestValue;
        Move bestMove = bestOnDepth(moves, key, 1, bestValue, nodesSearched, movesOnDepth, positionOnDepth);
        timer.endDepth(nodesSearched, movesOnDepth, positionOnDepth);

        // iterative deepening search
        for (int depth = 2; depth <= maxDepth && timer.hasTime(); depth++) {
            nodesOnDepth = 0;
            movesOnDepth = moves.size();
            positionOnDepth = 1;
            bestMove = bestOnDepth(moves, key, depth, bestValue, nodesOnDepth, movesOnDepth, positionOnDepth);
            std::cout << "Searched for depth " << depth << " (" << timer.runtime() << " seconds)\n";
            nodesSearched += nodesOnDepth;
            timer.endDepth(nodesOnDepth, movesOnDepth, positionOnDepth);
        }

        std::cout << "Nodes searched : " << nodesSearched << "\nTime taken: " << timer.runtime() << " seconds\nEvaluation : ";
        printEval(bestValue);
        return bestMove;
    }

    int Engine::evaluate() {
        float coeff = calcEndgameCoeff();
        int wRaw = calcWhiteScore();
        int bRaw = calcBlackScore();
        if (coeff == 3/62) {
            if ((wRaw < 400 && bRaw == 0) || (bRaw < 400 && wRaw == 0)) return 0;
        }
        return wRaw - bRaw + calcWhiteAwards(coeff) - calcBlackAwards(coeff);
    }

    int Engine::calcWhiteScore() {
        int score = 0;
        for (int i = 0; i < 64; i++) {
            score += rawPieceValues[board.whitePieceArray[i]];
        }
        return score;
    }
    int Engine::calcBlackScore() {
        int score = 0;
        for (int i = 0; i < 64; i++) {
            score += rawPieceValues[board.blackPieceArray[i]];
        }
        return score;
    }

    int Engine::calcWhiteAwards(float coeff) {
        int score = 0;
        for (int i = 0; i < 64; i++) {
            score += wStartRewards[board.whitePieceArray[i]][i] * coeff
                + wEndRewards[board.whitePieceArray[i]][i] * (1 - coeff);
        }
        return score;
    }
    int Engine::calcBlackAwards(float coeff) {
        int score = 0;
        for (int i = 0; i < 64; i++) {
            score += bStartRewards[board.blackPieceArray[i]][i] * coeff
                + bEndRewards[board.blackPieceArray[i]][i] * (1 - coeff);
        }
        return score;
    }
    float Engine::calcEndgameCoeff() {
        float score = 0;
        for (int i = 0; i < 64; i++) {
            score += endgameValues[board.whitePieceArray[i]];
            score += endgameValues[board.blackPieceArray[i]];
        }
        return score / 62;
    }

    void Engine::orderMoves(std::vector<Move>& moves, uint64_t positionKey) {
        TTEntry entry;
        if (TT.retrieve(positionKey, entry) && entry.bestMove.start != 64) {
            auto it = std::find(moves.begin(), moves.end(), entry.bestMove);
            if (it != moves.end()) {
                std::iter_swap(moves.begin(), it);
                std::sort(moves.begin() + 1, moves.end());
            }
        }
        else std::sort(moves.begin(), moves.end());
    }

    void Engine::start() {
        Move currentMove = Move();
        std::vector<Move> legalMoves;
        std::string input;
        while (true) {
            std::cout << "\nReady!\n";
            std::getline(std::cin, input);
            if (input == "fen") {
                std::getline(std::cin, input);
                if (input == "start") board = GameBoard();
                else board = GameBoard(input);
            }
            else if (input == "show moves") {
                legalMoves = board.allMoves();
                printMoves(legalMoves);
            }
            else if (input == "evaluate") {
                printEval(evaluate());
            }
            else if (input == "move") {
                currentMove = iterativeDeepening();
                if (currentMove.start == 64) {
                    endGame();
                    break;
                }
                std::cout << notation_from_move(currentMove) << "\n\n";
                board.doMove(currentMove);
            }
            else if (input == "play") {
                currentMove = iterativeDeepening();
                if (currentMove.start == 64) {
                    endGame();
                    break;
                }
                std::cout << notation_from_move(currentMove) << "\n\n";
                board.doMove(currentMove);
                while (true) {
                    legalMoves = board.allMoves();
                    if (legalMoves.empty()) {
                        endGame();
                        break;
                    }
                    std::getline(std::cin, input);
                    if (input == "exit") break;
                    if (!validateMove(currentMove, input)) {
                        std::cout << "unknown command\n";
                    }
                    else if (!isLegal(currentMove, legalMoves)) {
                        std::cout << "illigal move\n";
                    }
                    else {
                        board.doMove(currentMove);
                        currentMove = iterativeDeepening();
                        if (currentMove.start == 64) {
                            endGame();
                            break;
                        }
                        std::cout << notation_from_move(currentMove) << "\n\n";
                        board.doMove(currentMove);
                    }
                }
                std::cout << "Play mode exited";
            }
            else if (input == "show best line") {
                showBestLine();
            }
            else if (input == "search") {
                int depth;
                std::cin >> depth;
                legalMoves = board.allMoves();
                int eval;
                long nodes = 0;
                long moves = 0;
                int position = 0;
                timer.start();
                currentMove = bestOnDepth(legalMoves, board.computeZobristKey(), depth, eval, nodes, moves, position);
                board.doMove(currentMove);
                std::cout << "Nodes searched : " << nodes << "\nTime taken : " << timer.runtime() << "\nEvaluation : ";
                printEval(eval);
                std::cout<< notation_from_move(currentMove) << "\n";
                std::getline(std::cin, input);
            }
            else if (input == "finish") break;
            else {
                legalMoves = board.allMoves();
                if (!validateMove(currentMove, input)) {
                    std::cout << "unknown command\n";
                }
                else if (!isLegal(currentMove, legalMoves)) {
                    std::cout << "illigal move\n";
                }
                else {
                    board.doMove(currentMove);
                }
            }
        }
    }

    bool Engine::validateMove(Move& m, std::string input) {
        m = move_from_notation(input);
        return (m.start != 64);
    }
    bool Engine::isLegal(Move& current, std::vector<Move> &legals) {
        for (Move& m : legals) {
            if (m.start == current.start && m.end == current.end && m.promotion == current.promotion) {
                current = m;
                return true;
            }
        }
        return false;
    }

    void Engine::endGame() {
        if (board.checksFrom) {
            std::string won = board.turn ? "black" : "white";
            std::cout << won << " won by checkmate\n\n";
        }
        else std::cout << "draw by stalemate\n\n";
    }

    void Engine::showBestLine() {
        TTEntry entry;
        std::vector<Move> moves;
        std::cout << "\n";
        while (TT.retrieve(board.computeZobristKey(), entry) && entry.bestMove.start != 64) {
            board.doMove(entry.bestMove);
            moves.insert(moves.begin(), entry.bestMove);
            std::cout << notation_from_move(entry.bestMove) << "\n";
        }
        for (Move m : moves) {
            board.undoMove(m);
        }
        std::cout << "\n";
    }

    void Engine::xperft(int depth) {
        if (depth <= 0) return;
        long num = 0;
        long cur = 0;
        std::vector<Move> moves = board.allMoves();
        std::cout << moves.size() << "\n";
        for (Move m : moves) {
            board.doMove(m);
            cur = board.perft(depth - 1);
            board.undoMove(m);
            std::cout << notation_from_move(m) << " : " << cur << "\n";
            num += cur;
        }
        std::cout << num << "\n";
    }

    void Engine::printMoves(std::vector<Move>& moves) {
        std::cout << "Moves (" << moves.size() << ") :\n";
        for (Move m : moves) {
            std::cout << notation_from_move(m) << " ";
        }
        std::cout << "\n";
    }

    unsigned long Engine::index_from_notation(std::string str) {
        int c = str[0] - 'a';
        int r = str[1] - '1';
        return r * 8 + c;
    }
    std::string Engine::notation_from_index(unsigned long index) {
        int r = index / 8;
        int c = index % 8;
        char ch1 = 'a' + c;
        char ch2 = '1' + r;
        std::string str{ ch1, ch2 };
        return str;
    }
    Move Engine::move_from_notation(std::string input) {
        Move m = Move();
        if (input.length() < 4 || input.length() > 5) {
            return m;
        }
        int startIndex = index_from_notation(input.substr(0, 2));
        int endIndex = index_from_notation(input.substr(2, 2));
        if (startIndex > 64 || endIndex > 64) {
            return m;
        }
        pieceType prom;
        if (input.length() == 5) {
            switch (input[4])
            {
            case 'q':
                prom = QUEEN;
                break;
            case 'r':
                prom = ROOK;
                break;
            case 'b':
                prom = BISHOP;
                break;
            case 'n':
                prom = KNIGHT;
                break;
            default:
                prom = PAWN;
                break;
            }
        }
        else prom = EMPTY;
        if (prom == PAWN) {
            return m;
        }
        return Move(endIndex, startIndex, EMPTY, prom, false, false, false, false, 0);
    }
    std::string Engine::notation_from_move(Move& m) {
        std::string str = notation_from_index(m.start) + notation_from_index(m.end);
        switch (m.promotion)
        {
        case QUEEN:
            str += "q";
            break;
        case ROOK:
            str += "r";
            break;
        case BISHOP:
            str += "b";
            break;
        case KNIGHT:
            str += "n";
            break;
        default:
            break;
        }
        if (m.checks) str += "+";
        return str;
    }

    void Engine::printEval(int eval) {
        //std::cout << eval << "\n";
        if (eval > checkmateScore / 2) {
            std::cout << "M" << (checkmateScore + 1 - eval - board.plyCount) / 2;
        }
        else if (eval < -checkmateScore / 2) {
            std::cout << "-M" << (checkmateScore + 1 + eval - board.plyCount) / 2;
        }
        else {
            std::cout << (static_cast<float>(eval) / 100);
        }
        std::cout << std::endl;
    }

    void Engine::out(Bitboard b) {
        for (int row = 7; row >= 0; row--) {
            std::cout << row + 1 << "   ";
            for (int col = 0; col < 8; col++) {
                if (b & (1ULL << (row * 8 + col))) {
                    std::cout << "1 ";
                }
                else {
                    std::cout << "0 ";
                }
            }
            std::cout << std::endl;
        }
        std::cout << std::endl << "    a b c d e f g h" << std::endl << std::endl << std::endl;
    }