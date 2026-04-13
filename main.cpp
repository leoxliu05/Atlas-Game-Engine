#include <iostream>
#include "engine.h"

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    engine my_engine;
    my_engine.game_loop();
    return 0;
}
