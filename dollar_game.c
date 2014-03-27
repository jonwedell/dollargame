#include <stdio.h>
#include <stdlib.h>
#include <time.h>

unsigned long trials = 3000000;

int init_dollars = 3;
int dice = 3;
int num_players;

//#define skiprandom 1
#ifdef skiprandom
int rolls[100000];
int next_roll = 0;
#endif

// Print the number of dollar on the table
void print_dollars(int players[]){
    for (int i=0; i<num_players; i++){
        printf("Player %d has %d dollars left.\n", i, players[i]);
    }
    printf("------------------------------\n");
}

// Check if there is only one player left
int active_players(int players[]){
    int active = 0;
    for (int i=0; i<num_players; i++){
        if (players[i] > 0){ active++; }
    }
    return active;
}

// Check if adjacent players have money
int adjacent_money(int players[], int active_player){

    // Check left player
    if (active_player > 0){
        if (players[active_player-1] > 0){ return 1; }
    } else {
        if (players[num_players-1] > 0){ return 1; }
    }

    // Check right player
    if (active_player < num_players -1){
        if (players[active_player+1] > 0){ return 1;}
    } else {
        if (players[0] > 0){ return 1; }
    }

    // No money
    return 0;
}

// Do one roll for the given player
void do_roll(int players[], int active_player, int * dollars_remaining){

    // Only roll the number of dollars, or dice, whichever is lower
    int cutoff = players[active_player] < dice ? players[active_player] : dice;

    for (int each_dollar=0; each_dollar < cutoff; each_dollar++){

        // Roll the dice
        #ifdef skiprandom
        int roll = rolls[next_roll];
        if (++next_roll >= 100000){next_roll = 0;}
        #else
        int roll = rand() % 6;
        #endif

        // Only proceed here if they lost money
        if (roll > 2){

            players[active_player]--;

            // Left player gets buck
            if (roll == 3){
                if (active_player > 0){
                    players[active_player-1]++;
                } else {
                    players[num_players-1]++;
                }
            // Right player gets buck
            } else if (roll == 4){
                if (active_player < num_players -1){
                    players[active_player+1]++;
                } else {
                    players[0]++;
                }
            } else if (roll == 5){
                (*dollars_remaining)--;
            }
        }
    }
}

void move_on(int players[], int * active_player){
    while (players[(*active_player)] == 0){ if (++(*active_player) == num_players){ (*active_player) = 0;} }
}

// Little recursive method to see a game to it's conclusion. Return winning player, or numplayers if draw.
int simulate_game(int playray[], int active_player, int remaining_dollars){

    // Narrow it down to one player
    while ((remaining_dollars > 5) || (active_players(playray) > 1)){

        // Skip to the next player with money
        move_on(playray, &active_player);

        // Do a roll
        do_roll(playray, active_player, &remaining_dollars);
        if (++active_player == num_players){ active_player = 0;}
    }

    // Go to the last remaining player
    move_on(playray, &active_player);

    // Do the "final" role
    int play_dollars = playray[active_player];
    do_roll(playray, active_player, &remaining_dollars);

    // Rollover!
    if (remaining_dollars == 0) {
        return num_players;
    }
    // They didn't lose any money - they win
    else if (playray[active_player] == play_dollars){
        return active_player;
    }
    // There is someone else
    else if (adjacent_money(playray, active_player)){
        if (++active_player == num_players){ active_player = 0;}
        return simulate_game(playray, active_player, remaining_dollars);
    }
    // They still have money but put a bit in the center
    else {
        return active_player;
    }
}

// Main method
int main(int argc, char *argv[]){

    // Initialize the PRNG
    srand(time(NULL));
    //srand(0);

    #ifdef skiprandom
    // Test fixed roll array
    for (int i=0; i<100000; i++){ rolls[i] = rand() % 6;}
    #endif

    // Verify the user input and convert it to a long
    if ((argc != 2) || (!sscanf (argv[1],"%d",&num_players))){
        printf("Specify number of players as argument!\n");
        return 1;
    }

    // Keep track of the results
    unsigned long winray[num_players+1];
    for(int i=0;i<num_players+1;i++){ winray[i] = 0;}

    // Keep track of an ongoing game
    int playray[num_players];

    // Loop through the trials
    for (int trial=0; trial<trials; trial++){

        // Initialize the game
        for(int i=0;i<num_players;i++){ playray[i] = init_dollars;}

        #ifdef skiprandom
        // Go to a random point in the random simulator
        next_roll = rand() % 99999;
        #endif

        // Roll until there is one person left
        winray[simulate_game(playray, 0, num_players * init_dollars)]++;
    }

    // Print the results
    for (int i=0; i<num_players; i++){
        printf("Player %d: %.2f\n", i, winray[i]/((double)trials)*100);
    }
    printf("Rollovers: %.2f\n", winray[num_players]/((double)trials)*100);

    return 0;
}


