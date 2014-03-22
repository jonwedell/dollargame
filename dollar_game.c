#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int init_dollars = 3;
int dice = 3;

// Print the number of dollar on the table
void print_dollars(int players[], int num_players){
    for (int i=0; i<num_players; i++){
        printf("Player %d has %d dollars left.\n", i, players[i]);
    }
    printf("------------------------------\n");
}

// Check if there is only one player left
int active_players(int players[], int num_players){
    int active = 0;
    for (int i=0; i<num_players; i++){
        if (players[i] > 0){ active++; }
    }
    return active;
}

// Do one roll for the given player
int do_roll(int players[], int num_players, int active_player){
    int play_dollars = players[active_player];
    int removed_dollars = 0;

    for (int each_dollar=0; (each_dollar < play_dollars) && (each_dollar < dice); each_dollar++){

        // Roll the dice
        int roll = rand() % 6;

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
                removed_dollars++;
            }
        }
    }

    return removed_dollars;
}

// Little recursive method to see a game to it's conclusion. Return winning player, or numplayers if draw.
int simulate_game(int playray[], int num_players, int active_player, int remaining_dollars){

    // Narrow it down to one player
    while ((remaining_dollars > 5) || (active_players(playray, num_players) > 1)){
        remaining_dollars -= do_roll(playray, num_players, active_player);

        if (++active_player == num_players){ active_player = 0;}
    }


    // Go to the last remaining player
    while (playray[active_player] == 0){ if (++active_player == num_players){ active_player = 0;} }

    // Do the "final" role
    int play_dollars = playray[active_player];
    int lost_dollars = do_roll(playray, num_players, active_player);
    remaining_dollars -= lost_dollars;

    // Rollover!
    if (remaining_dollars == 0) {
        return num_players;
    }
    // The player who rolled kept their dollars (or maybe put some in the center too)
    else if (playray[active_player] + lost_dollars == play_dollars){
        return active_player;
    }

    // There is at least one other player now
     else {
        if (++active_player == num_players){ active_player = 0;}
        return simulate_game(playray, num_players, active_player, remaining_dollars);
    }

    // Find out how many players remain
    int remaining_players = active_players(playray, num_players);

    // There is more than one player again. Call ourselves.
    if ( remaining_players > 1){
        if (++active_player == num_players){ active_player = 0;}
        return simulate_game(playray, num_players, active_player, remaining_dollars);

    // There is one player left. They won!
    } else if (remaining_players == 1) {

        // The player who rolled still has the dollar(s)
        if (playray[active_player] > 0){
            return active_player;

        // There is still only one player, but it isn't the one who just rolled!
        } else {
            if (++active_player == num_players){ active_player = 0;}
            return simulate_game(playray, num_players, active_player, remaining_dollars);
        }

    // Rollover!
    } else { return num_players; }
}

// Main method
int main(int argc, char *argv[]){

    int players;
    unsigned long trials = 3000000;

    // Initialize the PRNG
    srand(time(NULL));

    // Verify the user input and convert it to a long
    if ((argc != 2) || (!sscanf (argv[1],"%d",&players))){
        printf("Specify number of players as argument!\n");
        return 1;
    }

    // Keep track of the results
    unsigned long winray[players+1];
    for(int i=0;i<players+1;i++){ winray[i] = 0;}

    // Keep track of an ongoing game
    int playray[players];
    int active_player;

    // Loop through the trials
    for (int trial=0; trial<trials; trial++){

        // Initialize the game
        for(int i=0;i<players;i++){ playray[i] = init_dollars;}

        // Roll until there is one person left
        int winner = simulate_game(playray, players, 0, players * init_dollars);
        winray[winner]++;
    }

    // Print the results
    for (int i=0; i<players; i++){
        printf("Player %d: %.2f\n", i, winray[i]/((double)trials)*100);
    }
    printf("Rollovers: %.2f\n", winray[players]/((double)trials)*100);


    return 0;
}

