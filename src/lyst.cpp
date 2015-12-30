#include "lyst.h"

Puzzle::Puzzle() {
    maxPieceCounts = {0,0,0,0,0,0,0,0};
    height = width = 0;
    leftWidth = rightWidth = midWidth = 1;
    desiredLeftWidth = 0;
    max_threads = std::thread::hardware_concurrency();
}
Puzzle::Puzzle(std::vector<int> pieceCountData, int puzzleHeight, int puzzleWidth) {
    maxPieceCounts = pieceCountData;
    height = puzzleHeight;
    width = puzzleWidth;
    desiredLeftWidth  = 1;
    leftWidth = rightWidth = midWidth = 1;
    max_threads = std::thread::hardware_concurrency();
}
Puzzle::Puzzle(std::vector<int> pieceCountData, int puzzleHeight, int puzzleWidth, int desiredBlockWidth) {
    maxPieceCounts = pieceCountData;
    height = puzzleHeight;
    width = puzzleWidth;
    desiredLeftWidth = desiredBlockWidth;
    // Check that we're not trying to be bigger than the puzzle
    if (desiredLeftWidth > width)
        desiredLeftWidth = width;
    leftWidth = rightWidth = midWidth = 1;
    max_threads = std::thread::hardware_concurrency();
}

Puzzle::~Puzzle() {

}

int Puzzle::getPuzzleHeight()      { return height;            }
int Puzzle::getPuzzleWidth()       { return width;             }
int Puzzle::getDesiredLeftWidth()  { return desiredLeftWidth;  }
int Puzzle::getMaxThreads()        { return max_threads;       }

/*std::vector<std::string> Puzzle::getLeftBlocks()  { return leftBlocks;  }
std::vector<std::string> Puzzle::getMidBlocks()   { return midBlocks;   }
std::vector<std::string> Puzzle::getRightBlocks() { return rightBlocks; }
std::string Puzzle::getLeftBlocks(int index) {
    if (index > 0 && index < leftBlocks.size())
        return leftBlocks[index];
    return 0;
}
std::string Puzzle::getMidBlocks(int index) {
    if (index > 0 && index < midBlocks.size())
        return midBlocks[index];
    return 0;
}
std::string Puzzle::getRightBlocks(int index) {
    if (index > 0 && index < rightBlocks.size())
        return rightBlocks[index];
    return 0;
}*/
std::vector< std::vector<uint16_t> > Puzzle::getLeftBlocks()  { return leftBlocks;   }
std::vector< std::vector<uint16_t> > Puzzle::getRightBlocks() { return rightBlocks;  }
std::vector< std::vector<uint16_t> > Puzzle::getMidBlocks()   { return midBlocks;    }
std::vector<uint16_t> Puzzle::getLeftBlocks(int index)        { return leftBlocks[index];  }
std::vector<uint16_t> Puzzle::getRightBlocks(int index)       { return rightBlocks[index]; }
std::vector<uint16_t> Puzzle::getMidBlocks(int index)         { return midBlocks[index];   }

int Puzzle::getLeftWidth()  { return leftWidth;  }
int Puzzle::getMidWidth()   { return midWidth;   }
int Puzzle::getRightWidth() { return rightWidth; }


// Helper functions used for generating things
int Puzzle::getPiece(std::vector<uint16_t> block, int index) {
    if ( index < block.size()*height ) {
        // Getting the column out of the vector
        uint16_t tempVal = block[index/height];
        // Getting the piece from the column
        return (tempVal / (int)pow(16,index%height) ) % 16;
    }
    return 0;
}
int Puzzle::getPiece(std::string block, int index) {
    if (index < block.length())
        return (int)block[index]-96;
    return 0;
}
std::vector<int> Puzzle::getPieceCounts(std::vector<uint16_t> puzzle) {
    std::vector<int> count (height*width, 0);
    for (int i=0; i<puzzle.size()*height; i++) {
        int temp = getPiece(puzzle,i);
        count[temp]++;
    }
    return count;
}
bool Puzzle::pieceCountIsValid(std::vector<uint16_t> puzzle) {
    std::vector<int> count (height*width, 0);
    return pieceCountIsValid(puzzle, count);
}
bool Puzzle::pieceCountIsValid(std::vector<uint16_t> puzzle, std::vector<int> count) {
    for (int i=0; i<puzzle.size()*height; i++) {
        int temp = getPiece(puzzle,i);
        count[temp]++;
        if (count[temp] > maxPieceCounts[temp])
            return false;
    }
    return true;
}
bool Puzzle::checkAddition(std::vector<uint16_t> in, int inWidth, std::vector<uint16_t> added, int addedWidth) {
    for (int i=0; i<height; i++) {
        int leftPiece = getPiece(in, (i+1)*inWidth-1);
        int rightPiece = getPiece(added, i*addedWidth);
        if (pieceHasRight(leftPiece) != pieceHasLeft(rightPiece))
            return false;
        if (leftPiece == 8 && rightPiece == 4)
            return false;
    }
    // Appending the vector to in
    for (int i=0; i<added.size(); i++) {
        in.push_back(added[i]);
    }
    return pieceCountIsValid(in);
}
bool Puzzle::checkAddition(std::vector<uint16_t> in, int inWidth, std::vector<uint16_t> added, int addedWidth, std::vector<int> previousCounts) {
    for (int i=0; i<height; i++) {
        int leftPiece = getPiece(in, (i+1)*inWidth-1);
        int rightPiece = getPiece(added, i*addedWidth);
        if (pieceHasRight(leftPiece) != pieceHasLeft(rightPiece))
            return false;
        if (leftPiece == 8 && rightPiece == 4)
            return false;
    }
    return pieceCountIsValid(added,previousCounts);
}

void Puzzle::generateFirstSet() {
    generateFirstSet("");
}
void Puzzle::generateFirstSet(std::string in) {
    // Check if we're done adding to the current block
    if (in.length() >= height) {
        // Check that the bottom is still valid
        if (pieceHasBottom( getPiece(in, height-1) ))
            return;

        bool r = false,
             l = false;
        for (int i=0; i<height; i++) {
            int piece = getPiece(in, i);
            if (pieceHasRight(piece))
                r = true;
            if (pieceHasLeft(piece))
                l = true;
        }
        // If the piece doesn't have a right or a left, then it's bad
        if (!r && !l)
            return;

        // TODO: Change this to be a CMake flag for other sized puzzles
        // Converting from std::string to short. Memory saver
        uint16_t newBlock = 0;
        for (int i=0; i<height; i++) {
            newBlock += ((int)in.at(i)-96) * pow(16,i);
        }
        std::vector<uint16_t> newBlockVector;
        newBlockVector.push_back(newBlock);

        // Is a right block?
        if (!r && l) {
            rightBlocks.push_back(newBlockVector);
            return;
        }
        // Left block?
        if (r && !l) {
            leftBlocks.push_back(newBlockVector);
            return;
        }
        // Otherwise, it's a mid block
        midBlocks.push_back(newBlockVector);
        return;
    }

    for (int i=1; i<15; i++) {
        // Matches above state
        int abovePiece = 0;
        if (in.length() != 0) {
            abovePiece = getPiece( in, in.length()-1 );
        }
        if (pieceHasBottom(abovePiece) != pieceHasTop(i))
            continue;

        // Check that A is not below B
        if (i==1 && abovePiece==2)
            continue;
        // Check if we have too many of a piece
        std::vector<int> counts (16,0);
        bool broken = false;
        for (int j=0; j<in.length(); j++) {
            int temp = getPiece(in,j);
            counts[temp]++;
            if (counts[temp] > maxPieceCounts[temp]) {
                broken = true;
                break;
            }
        }

        if (broken)
            continue;

        // No errors occurred, recurse
        generateFirstSet( in + (char)(i+96) );
    }
    return;
}

// Generate all of the blocks of desired width
void Puzzle::makeBlocks() {
    // Generate all the beginning things
    generateFirstSet();
    printf("Block counts: %d\t%d\t%d\n",leftBlocks.size(),midBlocks.size(),rightBlocks.size());

//    max_threads = 1;
    printf("Max threads: %d\n",max_threads);
    while (leftWidth < desiredLeftWidth && leftWidth > 0) {
        // Create all of the left blocks first
        std::vector<std::thread> threadPool;
        for (int i=0; i<max_threads; i++) {
            threadPool.push_back(std::thread(&Puzzle::combineLeftBlocks, this));
        }
        for (int i=0; i<max_threads; i++) {
            threadPool[i].join();
        }
        // Swap temp blocks with the vector of left
        leftBlocks.swap(tempLeftBlocks);
        tempLeftBlocks.clear();
        if (leftBlocks.size() != 0) {
            leftWidth = leftBlocks[0].size();
        }
        else {
            leftWidth = 0;
        }
        printf("Left Width: %d\t Number of Left Blocks: %d\n",leftWidth,leftBlocks.size());
    }

    // Solve for the correct pieces
    if (rightWidth + leftWidth == width) {
        std::vector<std::thread> threadPool;
        for (int i=0; i<max_threads; i++) {
            threadPool.push_back(std::thread(&Puzzle::combineLeftBlocks, this));
        }
        for (int i=0; i<max_threads; i++) {
            threadPool[i].join();
        }
        // Swap vectors
        leftBlocks.swap(tempLeftBlocks);
        tempLeftBlocks.clear();
        
        printf("Left Width: %d\t Number of Left Blocks: %d\n",leftWidth,leftBlocks.size());
    }

    return;
}

void Puzzle::combineLeftBlocks() {
    std::vector< std::vector<uint16_t> > newLeft;
    // We need to loop for as long as there's items left in the vector
    while (true) {
        // Get the first item, then remove so that others don't use it
        mutex1.lock();
        if (leftBlocks.size() == 0) {
            mutex1.unlock();
            break;
        }
        std::vector<uint16_t> currentLeft = leftBlocks.back();
        leftBlocks.pop_back();
        mutex1.unlock();

        // Check whether we need to loop through all of the middle pieces or the right pieces
        bool useRight = false;
        if (leftWidth + rightWidth == width) {
            useRight = true;
        }

        // If we're using the middle pieces
        if (!useRight) {
            for (int i=0; i<midBlocks.size(); i++) {
                // Check if we can add it
                if ( !checkAddition( currentLeft, leftWidth, midBlocks[i], midWidth) )
                    continue;
                std::vector<uint16_t> temp = currentLeft;
                for (int j=0; j<midBlocks[i].size(); j++) {
                    temp.push_back(midBlocks[i][j]);
                }
                newLeft.push_back( temp );
            }
        } else {
            for (int i=0; i<rightBlocks.size(); i++) {
                // Check if we can add it
                if ( !checkAddition( currentLeft, leftWidth, rightBlocks[i], rightWidth) )
                    continue;
                std::vector<uint16_t> temp = currentLeft;
                for (int j=0; j<rightBlocks[i].size(); j++) {
                    temp.push_back(rightBlocks[i][j]);
                }
                newLeft.push_back( temp );
            }
        }
    }
    // Ran out of pieces, so add all of the newLeft to leftBlocks
    mutex1.lock();
    for (int i=0; i<newLeft.size(); i++) {
        tempLeftBlocks.push_back(newLeft[i]);
    }
    mutex1.unlock();
}
