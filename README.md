# Chess engine
This chess engine was primarily designed to test effectiveness of Transposition Table policies. It is simple but fully functioning engine you can use or play against.
Its overall strength is equivalent to about 2000 rapid on lichess, although there are quite a few lacks and fields for improvement. The engine is a command line program, there is no GUI. Find the info how to interact with the program below.
## Engine supported commands
The engine support standart UCI commands, like go, go time/depth/perft, position, etc. Below there are some non standart commands you may find usefull. If you want to check all supported commands or edit them, check out Engine::start() function in Engine.cpp file. 
### print
Prints current board state, together with turn right and checkers.
### fen
Computes FEN notation for current position.
### show moves
Prints all legal moves in current position.
### show best line
Prints best line in current position according to the last search if any.
### move [arg]
Makes a specified move on the board if legal. The argument is a move in UCI notation, like e2e4, e1g1 (for castling), d7d8q (for promotion).
### undo move
Undoes last played move if any. We do not recommend to use this feature too often, because it may affect Transposition Table workflow.
### play time/depth [arg]
Enters play mode, which you may wish to use to play against engine conviniently. The argument is either time to think per move (must be double), or depth to search for (must be integer). The engine will think for specified time each move, or search up to specified depth. The mode does not limit your thinking time. Input your move in UCI notation after each response. All moves will be made automatically. Command "exit" to exit this mode.
### tt policy [arg]
Switches Transposition Table to the new one with specified policy type. Checkout workflow info for TT policies supported. Note that this command deletes the whole previous TT, so all the data saved will be lost. Consequently, we recommend to use this command only at the start of the game if required. 
## Engine workflow
The engine is a single-thread program. It uses iterative deepening, minimax recursion with alpha-beta pruning and quiescence search only to search the position in depth. Static evaluation function accounts only on matereal and piece-square tables. There is no opening book, so the engine will probably play the same moves with the same settings. Consider setting up the start position manually if you want to get different game scenarios, you may wish to use some from silver_op_suite.txt file. Both bitboards and piece arrays are used to hold a board state, magic bitboard technique is used for move generation. 
Transposition table is implemented and is allowed to be switched off or change the policy. Supported policy types are : "depth" (prefer deepest entry), "age" (prefer newest entry), "mixed" (prefers new entry if the irreversible move has been played since the old entry's age, or the deepest entry otherwise). Default policy is mixed one. 
