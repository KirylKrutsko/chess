#include "Engine.h"
#include "Tables.h"
#include "TTEntry.h"
#include "GameBoard.h"
#include "DepthTT.h"
#include "AgedTT.h"
#include<iostream>
#include<vector>
#include<algorithm>
#include<sstream>
#include <chrono>
#include <string>


    Engine::Engine(GameBoard& b) : board(b) {
        maxDepthDefault = 12;
        ttMode = true;
        TT = new DepthTT();
    }

    SearchResult Engine::searchWithoutTT(int depth, int alpha, int beta) {

        std::vector<Move> moves = board.allMoves();
        if (moves.empty()) {
            if (board.checksFrom) { // mate
                return { board.turn ? (-checkmateScore + board.plyCount) : (checkmateScore - board.plyCount), {} };
            }
            return { 0, {} }; // stalemate
        }

        if (depth == 0) return quiescenceSearch(alpha, beta);

        orderMoves(moves, board.computeZobristKey());
        SearchResult moveResult, bestResult;
        EntryType type = EXACT;

        if (board.turn) {
            type = LOWER;
            bestResult = { alpha, {} };
            for (Move& move : moves) {
                board.doMove(move);
                moveResult = searchWithoutTT(depth - 1, alpha, beta);
                moveResult.bestLine.push_back(move);
                board.undoMove(move);

                if (moveResult.eval > bestResult.eval) {
                    bestResult = moveResult;
                    type = EXACT;
                }
                alpha = std::max(alpha, moveResult.eval);

                if (beta <= alpha) {
                    type = UPPER;
                    break;  // Beta cut-off
                }
            }

            return bestResult;
        }
        else {
            type = UPPER;
            bestResult = { beta, {} };
            for (Move& move : moves) {
                board.doMove(move);
                moveResult = searchWithoutTT(depth - 1, alpha, beta);
                moveResult.bestLine.push_back(move);
                board.undoMove(move);

                if (moveResult.eval < bestResult.eval) {
                    bestResult = moveResult;
                    type = EXACT;
                }
                beta = std::min(beta, moveResult.eval);

                if (beta <= alpha) {
                    type = LOWER;
                    break;  // Alpha cut-off
                }
            }
            return bestResult;
        }
    }

    SearchResult Engine::search(int depth, int alpha, int beta, long& nodesOnDepth, long& movesOnDepth, int& positionOnDepth, long& ttHit) {
        nodesOnDepth++;

        if (!ttMode){
            if (board.isRepetition() || board.ply50MoveRule >= 100) {
                return { 0,{} };
            }
        }

        uint64_t key = board.computeZobristKey();
        TTEntry entry;
        if (ttMode && TT->retrieve(key, entry) && entry.depth >= depth) {
                ttHit++;
                if (entry.type == EXACT) {
                    //if (entry.depth == depth) {
                    //    SearchResult res = searchWithoutTT(entry.depth, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
                    //    //SearchResult res = searchWithoutTT(entry.depth, alpha, beta);
                    //    if (res.eval != entry.eval) {
                    //        //std::cout << res.eval << "\t!= TT :\t" << entry.eval << "\n";
                    //        if (abs(res.eval - entry.eval) > 100) {
                    //            std::cout << "Suspicious TT result : " << res.eval << "\tvs TT " << entry.eval << "\n";
                    //        }
                    //    }
                    //}
                    return { entry.eval,{entry.bestMove} };
                }
                if (entry.type == UPPER && entry.eval > alpha)
                    alpha = entry.eval;
                if (entry.type == LOWER && entry.eval < beta)
                    beta = entry.eval;
                if (alpha >= beta) return { entry.eval,{} };
        }

        std::vector<Move> moves = board.allMoves();
        movesOnDepth += moves.size();
        positionOnDepth++;

        if (moves.empty()) {
            if (board.checksFrom) { // mate
                return { board.turn ? (-checkmateScore + board.plyCount) : (checkmateScore - board.plyCount), {} };
            }
            return { 0, {} }; // stalemate
        }

        if (depth == 0) return quiescenceSearch(alpha, beta);

        orderMoves(moves, key);
        SearchResult moveResult, bestResult;
        EntryType type = EXACT;
        std::vector<Move> be;

        if (board.turn) {
            type = LOWER;
            bestResult = { alpha, {} };
            for (Move& move : moves) {
                board.doMove(move);
                moveResult = search(depth - 1, alpha, beta, nodesOnDepth, movesOnDepth, positionOnDepth, ttHit);
                moveResult.bestLine.push_back(move);
                board.undoMove(move);

                if (moveResult.eval > bestResult.eval) {
                    bestResult = moveResult;
                    type = EXACT;
                }
                alpha = std::max(alpha, moveResult.eval);

                if (beta <= alpha) {
                    type = UPPER;
                    break;  // Beta cut-off
                }
            }

            /*if(type == EXACT) {
                SearchResult check = searchWithoutTT(depth, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
                if (check.eval != bestResult.eval) {
                    std::cout << check.eval << "\t!= best :\t" << bestResult.eval << "\n";
                }
            }*/

            TT->store(key, depth, bestResult, type, history.size());
            return bestResult;
        }
        else {
            type = UPPER;
            bestResult = { beta, {} }; 
            for (Move& move : moves) {
                board.doMove(move);
                moveResult = search(depth - 1, alpha, beta, nodesOnDepth, movesOnDepth, positionOnDepth, ttHit);
                moveResult.bestLine.push_back(move);
                board.undoMove(move);

                if (moveResult.eval < bestResult.eval) {
                    bestResult = moveResult;
                    type = EXACT;
                }
                beta = std::min(beta, moveResult.eval);

                if (beta <= alpha) {
                    type = LOWER;
                    break;  // Alpha cut-off
                }
            }


            /*if (type == EXACT) {
                SearchResult check = searchWithoutTT(depth, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
                if (check.eval != bestResult.eval) {
                    std::cout << check.eval << "\t!= best :\t" << bestResult.eval << "\n";
                }
            }*/

            TT->store(key, depth, bestResult, type, history.size());
            return bestResult;
        }
    }

    SearchResult Engine::quiescenceSearch(int alpha, int beta) {

        if (!ttMode) {
            if (board.isRepetition() || board.ply50MoveRule >= 100) {
                return { 0,{} };
            }
        }

        if (board.checksFrom) {
            return searchWithoutTT(1, alpha, beta);
        }

        int standPat = evaluate();

        std::vector<Move> moves = board.noisyMoves();
        if (moves.empty()) {
            return { standPat, {} };
        }

        uint64_t key = board.computeZobristKey();
        orderMoves(moves, key);
        SearchResult moveResult, bestResult;
        EntryType type = EXACT;

        if (board.turn) {
            if (standPat >= beta) return { standPat,{} }; // cut-off if standing position is already too good
            alpha = std::max(alpha, standPat); // updating the alpha for following search
            bestResult = { alpha, {} };
            for (Move& move : moves) {
                board.doMove(move);
                moveResult = quiescenceSearch(alpha, beta);
                moveResult.bestLine.push_back(move);
                board.undoMove(move);

                if (moveResult.eval > bestResult.eval) {
                    bestResult = moveResult;
                }
                alpha = std::max(alpha, moveResult.eval);

                if (beta <= alpha) {
                    type = UPPER;
                    break;  // Beta cut-off
                }
            }
            return bestResult;
        }
        else {
            if (standPat <= alpha) return { standPat,{} };  // cut-off if standing position is already too good
            beta = std::min(beta, standPat); // updating the beta for following search
            bestResult = { beta, {} };
            for (Move& move : moves) {
                board.doMove(move);
                moveResult = quiescenceSearch(alpha, beta);
                moveResult.bestLine.push_back(move);
                board.undoMove(move);

                if (moveResult.eval < bestResult.eval) {
                    bestResult = moveResult;
                }
                beta = std::min(beta, moveResult.eval);

                if (beta <= alpha) {
                    type = LOWER;
                    break;  // Alpha cut-off
                }
            }
            return bestResult;
        }
    }

//    SearchResult Engine::bestOnDepth(std::vector<Move>& moves, uint64_t key, int depth, long& nodesOnDepth, long& movesOnDepth, int& positionOnDepth) {
//        orderMoves(moves, key);
//        std::vector<Move> bestLine, moveLine;
//        int moveEval;
//        if (board.turn) {
//            bestEval = std::numeric_limits<int>::min();
//            for (Move& move : moves) {
//                moveLine.clear();
//                board.doMove(move);
//                moveEval = search(moveLine, depth - 1, bestEval, std::numeric_limits<int>::max(), nodesOnDepth, movesOnDepth, positionOnDepth);
//                board.undoMove(move);
//                if (moveEval > bestEval) {
//                    bestEval = moveEval;
//                    bestLine = moveLine;
//                    bestLine.push_back(move);
//                }
//            }
//        }
//        else {
//            bestEval = std::numeric_limits<int>::max();
//            for (Move& move : moves) {
//                board.doMove(move);
//                moveEval = search(moveLine, depth - 1, std::numeric_limits<int>::min(), bestEval, nodesOnDepth, movesOnDepth, positionOnDepth);
//                board.undoMove(move);
//                if (moveEval < bestEval) {
//                    bestEval = moveEval;
//                    bestLine = moveLine;
//                    bestLine.push_back(move);
//                }
//            }
//        }
//        /*std::vector<Move> be;
//        int eval = searchWithoutTT(be, depth, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
//        if (eval != bestEval) {
//            std::cout << eval << "\t!= best :\t" << bestEval << "\n";
//        }*/
//        //TT->insert(key, board.reserveKey(), depth, bestEval, EXACT, bestLine[bestLine.size() - 1]);
//        return bestLine;
//    }
//

    SearchResult Engine::iterativeDeepening(int maxDepth, bool useTimer) {

        //std::cout << "\n";
        std::vector<Move> moves = board.allMoves();
        if (moves.empty() || isDrawByIM()) return { 0,{} };
        //printMoves(moves);

        if (moves.size() == 1) {
            return { 0, {moves[0]} , 0, 0};
        }

        long nodesSearched = 0;
        long ttHit = 0;

        long ttHitOnDepth = 0;
        long nodesOnDepth = 1;
        long movesOnDepth = moves.size();
        int positionOnDepth = 1;
        timer.start();

        SearchResult result;

        // quick search for depth 1 :
        uint64_t key = board.computeZobristKey();
        result = search(1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), nodesSearched, movesOnDepth, positionOnDepth, ttHit);
        timer.endDepth(1, movesOnDepth, positionOnDepth);

        // iterative deepening search
        for (int depth = 2; depth <= maxDepth && abs(result.eval) < checkmateScore / 2; depth++) {

            #ifndef NDEBUG // exclude the timer in Debug mode
            if (useTimer && !timer.hasTime(depth)) {
                break;
            }
            #endif

            nodesOnDepth = 1;
            movesOnDepth = moves.size();
            positionOnDepth = 1;
            result = search(depth, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), nodesOnDepth, movesOnDepth, positionOnDepth, ttHitOnDepth);
            //std::cout << "Searched for depth " << depth << " (" << timer.runtime() << " seconds)\n";
            nodesSearched += nodesOnDepth;
            ttHit += ttHitOnDepth;
            timer.endDepth(depth, movesOnDepth, positionOnDepth);
        }
        result.nodesSearched = nodesSearched;
        result.ttHit = ttHit;
        result.runtime = timer.runtime();
        return result;
    }

    int Engine::evaluate() {
        float coeff = calcEndgameCoeff();
        int wRaw = calcWhiteScore();
        int bRaw = calcBlackScore();
        if (coeff == (float)3 / 62) {
            if ((wRaw < 400 && bRaw == 0) || (bRaw < 400 && wRaw == 0)) return 0;
        }
        int eval = wRaw - bRaw + calcWhiteAwards(coeff) - calcBlackAwards(coeff);
        /*if (eval > 0) eval += activeKingAward(coeff);
        else eval -= activeKingAward(coeff);*/
        //eval += activeKingAward(coeff);
        return eval;
    }

    int Engine::activeKingAward(float coeff) {
        int award = 14 - abs(static_cast<int>(board.bKingPos % 8 - board.wKingPos % 8)) - abs(static_cast<int>(board.bKingPos / 8 - board.wKingPos / 8));
        award *= (1 - coeff) * 2;
        //if (!board.turn) award = -award;
        return award;
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
        if (TT->retrieve(positionKey, entry) && entry.bestMove.start != 64) {
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
        SearchResult result;
        std::vector<Move> legalMoves;
        std::string input;
        while (true) {
            std::getline(std::cin, input);
            if (input.find("position ") == 0) {
                history.clear();
                input = input.substr(9);
                auto mIndex = input.find("moves ");
                if (mIndex == std::string::npos) {
                    if (!board.setFromFen(input)) std::cout << "fen invalid\n";
                }
                else {
                    if (!board.setFromFen(input.substr(0, mIndex))) std::cout << "fen invalid\n";
                    std::istringstream iss(input.substr(mIndex + 6));
                    std::string move;
                    while (iss >> move) {
                        legalMoves = board.allMoves();
                        if (!validateMove(currentMove, move)) {
                            std::cout << "move invalid\n";
                            break;
                        }
                        else if (!isLegal(currentMove, legalMoves)) {
                            std::cout << "move illegal\n";
                            break;
                        }
                        board.doMove(currentMove);
                        history.push_back(currentMove);
                    }
                }
            }
            else if (input.find("go depth ") == 0) {
                input = input.substr(9);
                if (!std::all_of(input.begin(), input.end(), ::isdigit)) std::cout << "depth wrong\n";
                result = iterativeDeepening(std::stoi(input), false);
                if (!result.bestLine.empty()) std::cout << "bestmove " << notation_from_move(result.bestLine[result.bestLine.size() - 1]) << "\n";
                else endGame();
            }
            else if (input.find("go") == 0) {
                result = iterativeDeepening(maxDepthDefault, true);
                if (!result.bestLine.empty()) std::cout << "bestmove " << notation_from_move(result.bestLine[result.bestLine.size() - 1]) << "\n";
                else endGame();
            }
            else if (input.find("go perft") == 0) {
                input = input.substr(9);
                if (!std::all_of(input.begin(), input.end(), ::isdigit)) std::cout << "depth wrong\n";
                xperft(std::stoi(input));
            }
            else if (input == "print") {
                board.printBoard();
            }
            else if (input == "show moves") {
                legalMoves = board.allMoves();
                printMoves(legalMoves);
            }
            else if (input == "evaluate") {
                printEval(evaluate());
            }
            else if (input.find("move ") != std::string::npos) {
                legalMoves = board.allMoves();
                if (!validateMove(currentMove, input.substr(input.find("move ") + 5))) {
                    std::cout << "move invalid\n";
                }
                else if (!isLegal(currentMove, legalMoves)) {
                    std::cout << "move illigal\n";
                }
                else {
                    board.doMove(currentMove);
                    history.push_back(currentMove);
                }
            }
            else if (input == "compare TT") {
                int depth;
                std::cin >> depth;
                long n, m, h;
                int p;
                timer.start();
                SearchResult eval2 = searchWithoutTT(depth, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
                std::cout << "Non-TT : time = " << timer.runtime() << ", eval : ";
                printEval(eval2.eval);
                timer.start();
                SearchResult eval1 = search(depth, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), n, m, p, h);
                std::cout << "TT : time = " << timer.runtime() << ", nodes = " << n << ", hit = " << h << "\n";
                printEval(eval2.eval);
            }
            else if (input == "self") {
                while (true) {
                    result = iterativeDeepening(maxDepthDefault, true);
                    if (result.bestLine.empty()) {
                        endGame();
                        break;
                    }
                    currentMove = result.bestLine[result.bestLine.size() - 1];
                    std::cout << notation_from_move(currentMove) << "\n\n";
                    board.doMove(currentMove);
                    history.push_back(currentMove);
                    updateTTmode();
                }
            }
            else if (input == "show best line") {
                showBestLine(result.bestLine);
            }
            else if (input == "TT lookup") {
                uint64_t key = board.computeZobristKey();
                TTEntry entry;
                if (TT->retrieve(key, entry)) {
                    std::cout << "Type : " << static_cast<int>(entry.type) << "\nDepth : " << static_cast<int>(entry.depth) << "\nEval : " << static_cast<int>(entry.eval) << "\nBest move : " << notation_from_move(entry.bestMove) << "\n";
                }
                else std::cout << "not found\n";
            }
            else if (input == "undo move") {
                board.undoMove(history[history.size() - 1]);
                history.pop_back();
            }
            else if (input.find("tt policy = ") == 0) {
                input = input.substr(12);
            }
            else if (input == "uci") {
                std::cout << "id name kk.ab\n";
                std::cout << "id author KirylKrutsko\n";
                std::cout << "uciok\n";
            }
            else if (input == "isready") {
                std::cout << "readyok\n";
            }
            else if (input.find("tt policy ") == 0) {
                input = input.substr(10);
                if (input == "age" && typeid(TT) != typeid(AgedTT)) TT = new AgedTT();
                else if (input == "depth" && typeid(TT) != typeid(DepthTT)) TT = new DepthTT();
            }
            else if (input == "quit") break;
            else {
                std::cout << "unknown command\n";
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

    bool Engine::isDrawByIM() {
        int ws = calcWhiteScore();
        int bs = calcBlackScore();
        if ((ws == 330 || ws == 320 || ws == 0) && (bs == 330 || bs == 320 || bs == 0)) return true; // each side has no more than one knight/bishop
        if ((ws == 640 && bs == 0) || (ws == 0 && bs == 640)) return true; // 2 knight vs king
        return false;
    }
    void Engine::endGame() {
        if (board.checksFrom) {
            std::string won = board.turn ? "black" : "white";
            std::cout << won << " won\n\n";
        }
        else std::cout << "draw\n\n";
    }

    void Engine::showBestLine(std::vector<Move> line) {
        for (int i = line.size() - 1; i >= 0; i--) {
            std::cout << "\n" << notation_from_move(line[i]);
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
        //if (m.checks) str += "+";
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

    void Engine::updateTTmode() {
        if (ttMode) {
            if (board.isRepetition() || board.ply50MoveRule > 85) {
                //std::cout << "Switched to non-TT mode!\n";
                board.positionHistory.pop_back();
                ttMode = false;
            }
        }
        else {
            if (board.ply50MoveRule == 0) {
                //std::cout << "Switched to TT mode!\n";
                ttMode = true;
            }
        }
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
