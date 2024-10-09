#include "Engine.h"
#include "Tables.h"
#include "TTEntry.h"
#include "GameBoard.h"
#include "DepthTT.h"
#include "AgedTT.h"
#include "MixedTT.h"
#include<iostream>
#include<vector>
#include<algorithm>
#include<sstream>
#include <string>


    Engine::Engine(GameBoard& b) : board(b) {
        useTimer = true;
        timebreak = false;
        maxDepthDefault = 12;
        ttMode = true;
        TT = new MixedTT();
    }

    SearchResult Engine::searchWithoutTT(int depth, int alpha, int beta, uint64_t& nodesSearched, uint64_t& ttHit) {
        if (useTimer && depth > 3 && timer.hasTime()) {
            timebreak = true;
            return { board.turn ? -std::numeric_limits<int>::max() : std::numeric_limits<int>::max() , {} };
        }

        nodesSearched++;

        std::vector<Move> moves = board.allMoves();
        if (moves.empty()) {
            if (board.checksFrom) { // mate
                return { board.turn ? (-checkmateScore + board.plyCount) : (checkmateScore - board.plyCount), {} };
            }
            return { 0, {} }; // stalemate
        }

        if (depth == 0) return quiescenceSearch(10, alpha, beta, nodesSearched, ttHit);

        orderMoves(moves, board.computeZobristKey());
        SearchResult moveResult, bestResult;
        EntryType type = EXACT;

        if (board.turn) {
            type = LOWER;
            bestResult = { alpha, {} };
            for (Move& move : moves) {
                board.doMove(move);
                moveResult = searchWithoutTT(depth - 1, alpha, beta, nodesSearched, ttHit);
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
        }
        else {
            type = UPPER;
            bestResult = { beta, {} };
            for (Move& move : moves) {
                board.doMove(move);
                moveResult = searchWithoutTT(depth - 1, alpha, beta, nodesSearched, ttHit);
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
        }

        return bestResult;
    }

    SearchResult Engine::search(int depth, int alpha, int beta, uint64_t& nodesSearched, uint64_t& ttHit) {
        if (useTimer && depth > 3 && !timer.hasTime()) {
            timebreak = true;
            return { board.turn ? -std::numeric_limits<int>::max() : std::numeric_limits<int>::max() , {} };
        }

        nodesSearched++;

        if (!ttMode){
            if (board.isRepetition() || board.ply50MoveRule >= 100) {
                return { 0,{} };
            }
        }

        SearchResult bestResult = { board.turn ? alpha : beta, {} };

        uint64_t key = board.computeZobristKey();
        TTEntry entry;
        if (ttMode && TT->retrieve(key, entry, false) && entry.depth >= depth) {
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
            else if (entry.type == UPPER && entry.eval > alpha) {
                alpha = entry.eval;
                bestResult = { entry.eval, {entry.bestMove} };
            }
            else if (entry.type == LOWER && entry.eval < beta) {
                beta = entry.eval;
                bestResult = { entry.eval, {entry.bestMove} };
            }
            if (alpha >= beta) return bestResult;
        }

        std::vector<Move> moves = board.allMoves();

        if (moves.empty()) {
            if (board.checksFrom) { // mate
                return { board.turn ? (-checkmateScore + board.plyCount) : (checkmateScore - board.plyCount), {} };
            }
            return { 0, {} }; // stalemate
        }

        if (depth == 0) return quiescenceSearch(10, alpha, beta, nodesSearched, ttHit);

        orderMoves(moves, key);
        SearchResult moveResult;
        EntryType type = EXACT;

        if (board.turn) {
            type = LOWER;
            for (Move& move : moves) {
                board.doMove(move);
                moveResult = search(depth - 1, alpha, beta, nodesSearched, ttHit);
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
        }
        else {
            type = UPPER;
            for (Move& move : moves) {
                board.doMove(move);
                moveResult = search(depth - 1, alpha, beta, nodesSearched, ttHit);
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
        }

        if (!timebreak) TT->store(key, depth, bestResult, type, movesOnSearchStarted);
        return bestResult;
    }

    SearchResult Engine::quiescenceSearch(int maxDepthExt, int alpha, int beta, uint64_t& nodesSearched, uint64_t& ttHit) {
        nodesSearched++;

        if (!ttMode) {
            if (board.isRepetition() || board.ply50MoveRule >= 100) {
                return { 0,{} };
            }
        }

        SearchResult bestResult = { board.turn ? alpha : beta,{} };

        uint64_t key = board.computeZobristKey();
        TTEntry entry;
        if (ttMode && TT->retrieve(key, entry, false)) {
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
            else if (entry.type == UPPER && entry.eval > alpha) {
                alpha = entry.eval;
                bestResult = { entry.eval, {entry.bestMove} };
            }
            else if (entry.type == LOWER && entry.eval < beta) {
                beta = entry.eval;
                bestResult = { entry.eval, {entry.bestMove} };
            }
            if (alpha >= beta) return bestResult;
        }

        int standPat = evaluate();
        /*if (maxDepthExt == 0) {
            std::cout << "too deep QS!\n";
            return { standPat, {} };
        }*/

        std::vector<Move> moves = board.noisyMoves();
        if (moves.empty()) {
            return { standPat, {} };
        }

        orderMoves(moves, key);
        SearchResult moveResult;
        EntryType type = EXACT;

        if (board.turn) {
            if (standPat >= beta) return { standPat,{} }; // cut-off if standing position is already too good
            alpha = std::max(alpha, standPat); // updating the alpha for following search
            for (Move& move : moves) {
                board.doMove(move);
                moveResult = quiescenceSearch(maxDepthExt - 1, alpha, beta, nodesSearched, ttHit);
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
        }
        else {
            if (standPat <= alpha) return { standPat,{} };  // cut-off if standing position is already too good
            beta = std::min(beta, standPat); // updating the beta for following search
            for (Move& move : moves) {
                board.doMove(move);
                moveResult = quiescenceSearch(maxDepthExt - 1, alpha, beta, nodesSearched, ttHit);
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
        }
        return bestResult;
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

    SearchResult Engine::iterativeDeepening(int maxDepth, uint64_t& nodesSearched, uint64_t& ttHit, double& time, int& depthSearched) {
        nodesSearched = 0;
        ttHit = 0;

        std::vector<Move> moves = board.allMoves();
        if (moves.empty() || isDrawByIM()) return { 0,{} };
        //printMoves(moves);

        if (moves.size() == 1) {
            return { 0, {moves[0]} };
        }

        SearchResult result, deepestResult;
        timer.start();

        // quick search for depth 1 :
        uint64_t key = board.computeZobristKey();
        movesOnSearchStarted = board.moveHistory.size();
        deepestResult = search(1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), nodesSearched, ttHit);

        // iterative deepening search
        for (int depth = 2; depth <= maxDepth && abs(deepestResult.eval) < checkmateScore / 2 && (!useTimer || timer.hasTime()); depth++) {
            timebreak = false;

            movesOnSearchStarted = board.moveHistory.size();
            result = search(depth, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), nodesSearched, ttHit);
            
            if (timebreak) break;
            deepestResult = result;
            std::cout << "Searched for depth " << depth << " (" << timer.runtime() << " seconds) : "<<deepestResult.eval<<"\n";
            //if (!deepestResult.bestLine.empty()) std::cout << notation_from_move(deepestResult.bestLine[deepestResult.bestLine.size() - 1]) << "\n";

            depthSearched = depth;
        }

        time = timer.runtime();
        return deepestResult;
    }

    int Engine::evaluate() {
        float coeff = calcEndgameCoeff();
        int wRaw = calcWhiteScore();
        int bRaw = calcBlackScore();
        if (coeff == (float)3 / 62) {
            if ((wRaw < 400 && bRaw == 0) || (bRaw < 400 && wRaw == 0)) return 0;
        }
        int eval = wRaw - bRaw + calcWhiteAwards(coeff) - calcBlackAwards(coeff);
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
        if (TT->retrieve(positionKey, entry, true) && entry.bestMove.start != 64) {
            //TT->retrieved--;
            auto it = std::find(moves.begin(), moves.end(), entry.bestMove);
            if (it != moves.end()) {
                std::iter_swap(moves.begin(), it);
                std::sort(moves.begin() + 1, moves.end());
            }
        }
        else {
            //TT->not_retrieved--;
            std::sort(moves.begin(), moves.end());
        }
    }

    void Engine::start() {
        Move currentMove = Move();
        SearchResult result;
        std::vector<Move> legalMoves;
        std::string input;
        while (true) {
            std::getline(std::cin, input);
            if (input.empty()) continue;
            if (input.find("position ") == 0) {
                input = input.substr(9);
                if (input.find("fen ") == 0) input = input.substr(4);
                else if (input.find("startpos") != 0) {
                    std::cout << "unknown command\n";
                    continue;
                }
                bool fenok = true;
                auto mIndex = input.find("moves ");
                if (mIndex == std::string::npos) {
                    if (!board.setFromFen(input)) {
                        fenok = false;
                        std::cout << "fen invalid\n";
                    }
                }
                else {
                    if (!board.setFromFen(input.substr(0, mIndex))) {
                        fenok = false;
                        std::cout << "fen invalid\n";
                    }
                    board.moveHistory.clear();
                    board.positionHistory.clear();
                    std::istringstream iss(input.substr(mIndex + 6));
                    std::string move;
                    while (iss >> move) {
                        legalMoves = board.allMoves();
                        if (!validateMove(currentMove, move)) {
                            fenok = false;
                            std::cout << "move invalid\n";
                            break;
                        }
                        else if (!isLegal(currentMove, legalMoves)) {
                            fenok = false;
                            std::cout << "move illegal\n";
                            break;
                        }
                        makeMove(currentMove);
                    }
                }
                if (fenok) std::cout << "fen ok\n";
            }
            else if (input.find("go depth ") == 0) {
                input = input.substr(9);
                if (!std::all_of(input.begin(), input.end(), ::isdigit)) std::cout << "depth wrong\n";
                uint64_t n, tt;
                double t;
                int d;
                useTimer = false;
                result = iterativeDeepening(std::stoi(input), n, tt, t, d);
                std::cout << "nodes : " << n << "\nttHit : " << tt << "\ntime : " << t << "\ndepth : " << d << "\n";
                if (!result.bestLine.empty()) std::cout << "bestmove " << notation_from_move(result.bestLine[result.bestLine.size() - 1]) << "\n";
                else endGame();
                useTimer = true;
            }
            else if (input.find("go time ") == 0) {
                input = input.substr(8);
                try {
                    timer.timeLimit = std::stod(input);
                }
                catch (std::invalid_argument& e) {
                    std::cout << "time wrong\n";
                    continue;
                }
                catch (std::out_of_range& e) {
                    std::cout << "time wrong\n";
                    continue;
                }
                useTimer = true;
                uint64_t n, tt;
                double t;
                int d;
                result = iterativeDeepening(100, n, tt, t, d);
                std::cout << "nodes : " << n << "\nttHit : " << tt << "\ntime : " << t << "\ndepth : " << d << "\n";
                if (!result.bestLine.empty()) std::cout << "bestmove " << notation_from_move(result.bestLine[result.bestLine.size() - 1]) << "\n";
                else endGame();
            }
            else if (input.find("go perft ") == 0) {
                input = input.substr(9);
                if (!std::all_of(input.begin(), input.end(), ::isdigit)) std::cout << "depth wrong\n";
                xperft(std::stoi(input));
            }
            else if (input.find("go") == 0) {
                uint64_t n, tt;
                double t;
                int d;
                result = iterativeDeepening(maxDepthDefault, n, tt, t, d);
                std::cout << "nodes : " << n << "\nttHit : " << tt << "\ntime : " << t << "\ndepth : " << d << "\n";
                if (!result.bestLine.empty()) std::cout << "bestmove " << notation_from_move(result.bestLine[result.bestLine.size() - 1]) << "\n";
                else endGame();
            }
            else if (input == "print") {
                board.printBoard();
            }
            else if (input == "show moves") {
                legalMoves = board.allMoves();
                printMoves(legalMoves);
            }
            else if (input == "show noisy") {
                legalMoves = board.noisyMoves();
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
                    std::cout << "move illegal\n";
                }
                else {
                    makeMove(currentMove);
                    std::cout << "move ok\n";
                }
            }
            else if (input == "compare TT") {
                int depth;
                std::cin >> depth;
                timer.start();
                uint64_t n, tt;
                SearchResult eval2 = searchWithoutTT(depth, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), n, tt);
                std::cout << "Non-TT : time = " << timer.runtime() << ", eval : ";
                printEval(eval2.eval);
                timer.start();
                movesOnSearchStarted = board.moveHistory.size();
                SearchResult eval1 = search(depth, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), n, tt);
                std::cout << "TT : time = " << timer.runtime() << ", nodes = " << n << ", hit = " << tt << "\n";
                printEval(eval2.eval);
            }
            else if (input == "self") {
                while (true) {
                    uint64_t n, tt;
                    double t;
                    int d;
                    result = iterativeDeepening(maxDepthDefault, n, tt, t,d);
                    if (result.bestLine.empty()) {
                        endGame();
                        break;
                    }
                    currentMove = result.bestLine[result.bestLine.size() - 1];
                    std::cout << notation_from_move(currentMove) << "\n\n";
                    makeMove(currentMove);
                }
            }
            else if (input == "show best line") {
                showBestLine(result.bestLine);
            }
            else if (input == "TT lookup") {
                uint64_t key = board.computeZobristKey();
                TTEntry entry;
                if (TT->retrieve(key, entry, false)) {
                    std::cout << "Type : " << static_cast<int>(entry.type) << "\nDepth : " << static_cast<int>(entry.depth) << "\nEval : " << static_cast<int>(entry.eval) << "\nBest move : " << notation_from_move(entry.bestMove) << "\n";
                }
                else std::cout << "not found\n";
            }
            else if (input == "undo move") {
                board.undoMove(board.moveHistory[board.moveHistory.size() - 1]);
            }
            else if (input == "key") {
                std::cout << board.computeZobristKey() << "\n";
            }
            else if (input == "50move") {
                std::cout << board.ply50MoveRule << "\n";
            }
            else if (input == "moves count") {
                legalMoves = board.allMoves();
                std::cout << legalMoves.size() << "\n";
            }
            else if (input == "checkers") {
                std::cout << board.checksFrom << "\n";
            }
            else if (input == "fen") {
                std::cout << board.computeFEN() << "\n";
            }
            else if (input == "tt stats") {
                std::cout << "stored " << TT->stored << " retrieved " << TT->retrieved << " not_retrieved " << TT->not_retrieved << " overwriten " << TT->overriten << " overwriten_with_new " << TT->overwritenWithDiff << " refused " << TT->refused << "\n";
            }
            else if (input.find("play ") == 0){
                input = input.substr(5);
                if (input.find("time ") == 0) {
                    input = input.substr(5);
                    try {
                        timer.timeLimit = std::stod(input);
                        useTimer = true;
                    }
                    catch (std::invalid_argument& e) {
                        std::cout << "time wrong\n";
                        continue;
                    }
                    catch (std::out_of_range& e) {
                        std::cout << "time wrong\n";
                        continue;
                    }
                }
                else if(input.find("depth ")==0) {
                    input = input.substr(6);
                    try {
                        maxDepthDefault = std::stoi(input);
                        useTimer = false;
                    }
                    catch (std::invalid_argument& e) {
                        std::cout << "time wrong\n";
                        continue;
                    }
                    catch (std::out_of_range& e) {
                        std::cout << "time wrong\n";
                        continue;
                    }
                }
                else {
                    std::cout << "wrong command format\n";
                    continue;
                }
                uint64_t n, tt;
                double t;
                int d;
                result = iterativeDeepening(maxDepthDefault, n, tt, t, d);
                if (!result.bestLine.empty()) {
                    currentMove = result.bestLine[result.bestLine.size() - 1];
                    std::cout << "bestmove " << notation_from_move(currentMove) << "\n";
                    makeMove(currentMove);
                    board.printBoard();
                    while (true) {
                        std::cin >> input;
                        if (input == "exit") break;
                        legalMoves = board.allMoves();
                        if (!validateMove(currentMove, input)) {
                            std::cout << "move invalid\n";
                        }
                        else if (!isLegal(currentMove, legalMoves)) {
                            std::cout << "move illegal\n";
                        }
                        else {
                            makeMove(currentMove);
                            board.printBoard();
                            int d;
                            result = iterativeDeepening(maxDepthDefault, n, tt, t, d);
                            if (!result.bestLine.empty()) {
                                currentMove = result.bestLine[result.bestLine.size() - 1];
                                std::cout << "bestmove " << notation_from_move(currentMove) << "\n";
                                makeMove(currentMove);
                                board.printBoard();
                            }
                            else {
                                endGame();
                                break;
                            }
                        }
                    }
                }
                else endGame();
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
                if (input == "age" && typeid(TT) != typeid(AgedTT)) {
                    delete TT;
                    TT = new AgedTT();
                }
                else if (input == "depth" && typeid(TT) != typeid(DepthTT)) {
                    delete TT;
                    TT = new DepthTT();
                }
                else if (input == "mixed" && typeid(TT) != typeid(MixedTT)) {
                    delete TT;
                    TT = new MixedTT();
                }
                else {
                    std::cout << "unknown tt policy\n";
                }
            }
            else if (input == "quit") break;
            else {
                std::cout << "unknown command\n";
            }
        }
    }

    void Engine::makeMove(Move& m) {
        board.doMove(m);
        updateTTmode();
        if (board.ply50MoveRule == 0) {
            if (MixedTT* mixed = dynamic_cast<MixedTT*>(TT)) {
                mixed->lastIrreversible = board.positionHistory.size();
                //std::cout << "last irreversible updated\n";
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
        std::cout << "gameover ";
        if (board.checksFrom) {
            std::string won = board.turn ? "black" : "white";
            std::cout << won << " won\n";
        }
        else std::cout << "draw\n";
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
        std::cout << "Moves (" << moves.size() << ") : ";
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

    bool Engine::updateTTmode() {
        if (ttMode) {
            if (board.isRepetition() || board.ply50MoveRule > 85) {
                std::cout << "Switched to non-TT mode!\n";
                board.positionHistory.pop_back();
                ttMode = false;
                return true;
            }
        }
        else {
            if (board.ply50MoveRule == 0) {
                std::cout << "Switched to TT mode!\n";
                ttMode = true;
                return true;
            }
        }
        return false;
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
