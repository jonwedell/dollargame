#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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


// Simulate a number of games.
void simulate_x_games(int x, int pipe){

    // Keep track of game winners
    unsigned long winray[num_players+1];
    for(int i=0;i<num_players+1;i++){ winray[i] = 0;}

    // Keep track of an ongoing game
    struct player players[num_players];

    for (int n=0; n<x; n++){

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


    // Send the results back
    if (!write(pipe, &winray, sizeof(winray))){
        printf("Error while returning simulation results to our parent.\n");
        exit(3);
    }
    exit(0);
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


    int num_cpu = sysconf(_SC_NPROCESSORS_ONLN);
    int pfds[num_cpu][2];

    // Start the threads
    for (int core=0; core<num_cpu; core++){

        // Open a pipe to communicate with our child
        if ((pipe(pfds[core]))){
            printf("Could not open pipe to communicate with child.\n");
            exit(3);
        }

        // Child runs a simulation
        if (!fork()) { simulate_x_games(trials/num_cpu, pfds[core][1]); }
    }

    // A place to read in the results from the children
    unsigned long tmpray[num_players+1];

    // Wait for our children to complete
    for (int core=0; core<num_cpu; core++){

        // Read results of one simulation process
        if (!read(pfds[core][0], &(tmpray), sizeof(winray))){
            printf("Could not read simulation results from child.\n");
            exit(4);
        }

        // Add this thread's results onto the total results
        for (int i=0; i<num_players+1; i++){ winray[i] += tmpray[i];}
        wait(NULL);
    }

    // Print the results
    for (int i=0; i<num_players; i++){
        printf("Player %d: %.2f\n", i, winray[i]/((double)((trials/num_cpu)*num_cpu))*100);
    }
    printf("Rollovers: %.2f\n", winray[num_players]/((double)((trials/num_cpu)*num_cpu))*100);

    return 0;
}


