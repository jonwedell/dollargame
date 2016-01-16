#!/usr/bin/env python2.7

import os,sys
import random
import argparse
import subprocess
from multiprocessing import Process, Pipe, cpu_count

# Set up the parser
parser = argparse.ArgumentParser(version="0.0.1")

# Get the start up arguments
parser.add_argument("-t", "--trials", help="How many simulations to run?", action="store", type=int, default=1000)
parser.add_argument("-p", "--players", help="How many players are there?", action="store", type=int, default=10)
parser.add_argument("-d", "--dollars", help="How many dollars do players start with.", action="store", type=int, default=3)
parser.add_argument("-i", "--dice", help="The maximum number of dice rolled.", action="store", type=int, default=3)
parser.add_argument("--ordering", help="Clockwise or counter clockwise?", action="store", type=str, choices={"clockwise", "counterclockwise"}, default="clockwise")
parser.add_argument("-c", "--num_processors", help="The number of processors to use.", action="store", type=int, default=cpu_count())
parser.add_argument("-s", "--static", help="Specify a PRNG seed.", action="store", type=int, default=None)

# Parse the arguments
args = parser.parse_args()

rand_source = random.Random()
rand_source.seed(args.static)

class Player(object):

    def __init__(self, index):
        self.dollars = args.dollars
        self.prevp = None
        self.nextp = None
        self.index = index

    def roll(self):
        pot_change = 0
        for dollar in range(0,min(self.dollars,args.dice)):
            action = rand_source.randint(1,6)
            if action > 3:
                self.dollars -= 1
                if action == 4:
                    self.nextp.dollars += 1
                elif action == 5:
                    self.prevp.dollars += 1
                elif action == 6:
                    pot_change += 1

        return pot_change

    def __str__(self):
        nextp_index = None
        if self.nextp != None:
            nextp_index = self.nextp.index

        prevp_index = None
        if self.prevp != None:
            prevp_index = self.prevp.index
        return "player(%s) dollars(%d) next(%s) previous(%s)" % (self.index, self.dollars, nextp_index, prevp_index)

class EliminationPlayer(Player):

    def roll(self):
        pot_change = super(EliminationPlayer, self).roll()

        # Remove ourself if out of dollars
        if self.dollars == 0:
            self.prevp.nextp = self.nextp
            self.nextp.prevp = self.prevp

        return pot_change

class Game():

    def __init__(self, num_players):
        """ Initialize a game. """
        self.dollars = num_players * args.dollars

        # The players are a doubly-linked list
        self.firstplayer = Player(0)
        curplayer = self.firstplayer
        for x in range(1,num_players):
            curplayer.nextp = Player(x)
            curplayer.nextp.prevp = curplayer
            curplayer = curplayer.nextp

        curplayer.nextp = self.firstplayer
        curplayer.nextp.prevp = curplayer

    def playerIterator(self):
        """ Iterate through the active players. """

        curplayer = self.firstplayer
        while curplayer.nextp != self.firstplayer:
            yield curplayer
            curplayer = curplayer.nextp
        yield curplayer

    def __str__(self):
        """ Print the state of the game. """

        retstr = "game() dollars(%d)" % self.dollars
        for player in self.playerIterator():
            retstr += "\n\t" + str(player)
        return retstr

    def executeRound(self):
        """ Go through one round of the dollar game. """
        for player in self.playerIterator():
            self.dollars -= player.roll()

    def getActivePlayers(self):
        active_players = 0
        for player in self.playerIterator():
            if player.dollars > 0:
                active_players += 1
        return active_players

    def getWinner(self,player=None):
        # Either start with the first player or the designated player
        curplayer = self.firstplayer
        if player is not None:
            curplayer = player

        # Get down to one player
        if self.getActivePlayers() > 1:
            self.dollars -= curplayer.roll()
            while self.getActivePlayers() > 1:
                curplayer = curplayer.nextp
                self.dollars -= curplayer.roll()

        # Get the "last" player
        while curplayer.nextp.dollars == 0:
            curplayer = curplayer.nextp
        curplayer = curplayer.nextp

        # Can the last player hold on to their winnings?
        self.dollars -= curplayer.roll()

        # See if there is a winner
        num_players = self.getActivePlayers()
        # Rollover
        if num_players == 0:
            return None
        # A winner!
        elif num_players == 1:
            return curplayer.index
        # Two people have money again, repeat
        else:
            return self.getWinner(curplayer)











# We'll have multiple threads doing calculations
winning_stats = [0] * args.players
rollovers = 0

# Change behavior based on threads
if args.num_processors == 1:

    # Do all the simulations here
    for x in range(0,args.trials):
        winner = Game(args.players).getWinner()
        if winner is None:
            rollovers += 1
        else:
            winning_stats[winner] += 1
else:

    # For a thread for each core and do simulations there
    def simulateXGames(conn,x):
        """ Something to call from a thread to run a bunch of games. """

        winning_stats = [0] * args.players
        rollovers = 0

        for x in range(0,x):
            winner = Game(args.players).getWinner()
            if winner is None:
                rollovers += 1
            else:
                winning_stats[winner] += 1

        conn.send([winning_stats, rollovers])
        conn.close()


    processes = []

    for x in range(0,args.num_processors):
        # Set up the pipes
        parent_conn, child_conn = Pipe()
        # Start the process
        p = Process(target=simulateXGames, args=(child_conn,args.trials/args.num_processors))
        processes.append([p, parent_conn, child_conn])
        p.start()

    for process in processes:
        # Get the results of the run
        results = process[1].recv()
        winning_stats = [x + y for x, y in zip(winning_stats, results[0])]
        rollovers += results[1]
        p.join()


# Print results
print "Trials: %d" % args.trials
for x in range(0,args.players):
    print "Player %d: %2.2f" % (x, float(winning_stats[x])/args.trials*100)
print "Rollovers: %2.2f" % (float(rollovers)/args.trials*100)

