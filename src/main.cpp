#include <iostream>
#include "Game.h"

int main() {
    Game game;
    
    if (!game.Initialize()) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return -1;
    }
    
    game.Run();
    game.Shutdown();
    
    return 0;
} 