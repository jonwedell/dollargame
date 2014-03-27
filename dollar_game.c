#include <stdio.h>
#include <stdlib.h>
#include <time.h>

unsigned long trials = 5000000;

int init_dollars = 3;
int dice = 3;
int num_players;


struct player {
    int dollars;
    struct player *next;
    struct player *previous;
    struct player *next_money;
    struct player *previous_money;
};


// Print the number of dollar on the table
void print_dollars(struct player * active_player){
    struct player * cur_player = active_player;
    int ordinal = 0;

    do {
        printf("Player cur+%d: Money: %d\n", ordinal, cur_player->dollars);
        cur_player = cur_player->next;
        ordinal++;
    } while (cur_player != active_player);
}

// Do one roll for the given player
void do_roll(struct player * active_player){

    // Only roll the number of dollars, or dice, whichever is lower
    int cutoff = active_player->dollars < dice ? active_player->dollars : dice;

    for (int each_dollar=0; each_dollar < cutoff; each_dollar++){

        // Roll the dice
        int roll = rand() % 6;

        // Only proceed here if they lost money
        if (roll > 2){

            active_player->dollars--;

            // Left player gets buck
            if (roll == 3){

                if (active_player->previous_money != active_player->previous){
                    active_player->previous_money->next_money = active_player->previous;
                    active_player->previous->previous_money = active_player->previous_money;
                }

                if (active_player->dollars == 0){
                    active_player->previous->next_money = active_player->next_money;
                    active_player->next_money->previous_money = active_player->previous;
                } else {
                    active_player->previous->next_money = active_player;
                }

                active_player->previous->dollars++;
                active_player->previous_money = active_player->previous;

            // Right player gets buck
            } else if (roll == 4){

                if (active_player->next_money != active_player->next){
                    active_player->next_money->previous_money = active_player->next;
                    active_player->next->next_money = active_player->next_money;
                }

                if (active_player->dollars > 0){
                    active_player->next->previous_money = active_player;
                } else {
                    active_player->next->previous_money = active_player->previous_money;
                    active_player->previous_money->next_money = active_player->next;
                }

                active_player->next->dollars++;
                active_player->next_money = active_player->next;

            // Dollar to the center
            } else if (roll == 5){
                if (active_player->dollars == 0){
                    active_player->previous_money->next_money = active_player->next_money;
                    active_player->next_money->previous_money = active_player->previous_money;
                }
            }
        }
    }
}


// Little recursive method to see a game to it's conclusion. Return winning player, or numplayers if draw.
struct player * simulate_game(struct player *  active_player){

    // Have people take turns until there is only one player left
    while (active_player->next_money != active_player){
        do_roll(active_player);
        active_player = active_player->next_money;
    }

    // Do the "final" role
    do_roll(active_player);

    // If they are still the only player
    if (active_player->next_money == active_player){
        if (active_player->dollars != 0){
            return active_player;
        } else {
            return NULL;
        }

    // We need to do more rolls, somebody else has money
    } else {
        return simulate_game(active_player->next_money);
    }
}

// Main method
int main(int argc, char *argv[]){

    // Initialize the PRNG
    srand(time(NULL));
    //srand(0);

    // Verify the user input and convert it to a long
    if ((argc < 2) || (!sscanf (argv[1],"%d",&num_players))){
        printf("Specify number of players as argument!\n");
        return 1;
    }

    // Allow them to specify the number of trials
    if (argc == 3){
        if (!sscanf(argv[2],"%lu",&trials)){
            printf("If you specify a second argument it must be the number of trials.\n");
            return 2;
        }
    }

    // Keep track of the results
    unsigned long winray[num_players+1];
    for(int i=0;i<num_players+1;i++){ winray[i] = 0;}

    // Keep track of an ongoing game
    struct player players[num_players];

    // Loop through the trials
    for (int trial=0; trial<trials; trial++){

        // Initialize the game
        for(int i=0;i<num_players;i++){players[i].dollars = init_dollars; }
        players[0].previous = &(players[num_players-1]);
        players[0].previous_money = &(players[num_players-1]);
        players[num_players-1].next = &(players[0]);
        players[num_players-1].next_money = &(players[0]);

        for (int i=0; i<num_players-1; i++){
            players[i].next = &(players[i+1]);
            players[i].next_money = &(players[i+1]);
        }
        for (int i=num_players-1; i>0; i--){
            players[i].previous = &(players[i-1]);
            players[i].previous_money = &(players[i-1]);
        }

        // Roll until there is one person left
        struct player * winrar = simulate_game((&players[0]));
        if (winrar == NULL){ winray[num_players]++;} else {
            for (int i=0; i<num_players; i++){
                if (winrar == &(players[i])){
                    winray[i]++;
                    break;
                }
            }
        }
    }

    // Print the results
    for (int i=0; i<num_players; i++){
        printf("Player %d: %.2f\n", i, winray[i]/((double)trials)*100);
    }
    printf("Rollovers: %.2f\n", winray[num_players]/((double)trials)*100);

    return 0;
}


