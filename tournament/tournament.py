#----------------------------------------------------------------------------
# Plays the tournament.
#----------------------------------------------------------------------------

import os, string, time, random

from program import Program 
from game import Game
from gameplayer import GamePlayer

#----------------------------------------------------------------------------

# Stores summary information about each game played.
class ResultsFile:
    def __init__(self, name, header):
        self._name = name
        self._wasExisting = self._checkExisting()
        if self._wasExisting:
            self._lastIndex = self._findLastIndex()
            self._printTimeStamp()
        else:
            self._lastIndex = -1;
            self._printHeader(header)

    def addResult(self, currentRound, opening, blackName, whiteName,
                  resultBlack, resultWhite, gameLen,
                  elapsedBlack, elapsedWhite,
                  error, errorMessage):
       
        self._lastIndex += 1
        f = open(self._name, "a")
        f.write("%04i\t%i\t%s\t%s\t%s\t%s\t%s\t%i\t%.1f\t%.1f\t%s\t%s\n" \
                % (self._lastIndex, int(currentRound), opening,
                   blackName, whiteName, resultBlack, resultWhite,
                   gameLen, elapsedBlack, elapsedWhite,
                   error, errorMessage))
        f.close()

    def wasExisting():
        return _wasExisting

    def clear(self):
        f = open(self._name, "w")
        f.close()
        self._printHeader()
        self._gameIndex = -1;
        self._wasExisting = 0

    def getLastIndex(self):
        return self._lastIndex

    def _checkExisting(self):
        try:
            f = open(self._name, "r")
        except IOError:
            return 0
        f.close()
        return 1

    def _findLastIndex(self):
        f = open(self._name, "r")
        last = -1
        line = f.readline()
        while line != "":
            if line[0] != "#":
                array = string.split(line, "\t")
                last = int(array[0])
            line = f.readline()
        f.close()
        return last

    def _printHeader(self, infolines):
        f = open(self._name, "w")
        f.write("# Game results file generated by twogtp.py.\n" \
                "#\n")
        for l in infolines:
            f.write("# %s\n" % l);
        f.write("#\n" \
                "# GAME\tROUND\tOPENING\tBLACK\tWHITE\tRES_B\tRES_W\tLENGTH\tTIME_B\tTIME_W\tERR\tERR_MSG\n" \
                "#\n")
        f.close()
        self._printTimeStamp()

    def _printTimeStamp(self):
        f = open(self._name, "a")
        timeStamp = time.strftime("%Y-%m-%d %X %Z", time.localtime())
        f.write("# Date: " + timeStamp + "\n");
        f.close()

#----------------------------------------------------------------------------

# Base tournament class.
# Contains useful functions for all types of tournaments.
class Tournament:
    def __init__(self,
                 p1name, p1cmd, p2name, p2cmd, size, rounds, outdir,
                 openings, verbose):

        self._p1name = p1name
        self._p1cmd = p1cmd
        self._p2name = p2name
        self._p2cmd = p2cmd
        self._size = size
        self._rounds = rounds
        self._outdir = outdir
        self._verbose = verbose
        
        info = ["p1name: " + p1name, \
                "p1cmd: " + p1cmd, \
                "p2name: " + p2name, \
                "p2cmd: " + p2cmd, \
                "Boardsize: " + `size`, \
                "Rounds: " + `rounds`, \
                "Openings: " + openings, \
                "Directory: " + outdir, \
                "Start Date: "
                + time.strftime("%Y-%m-%d %X %Z", time.localtime())]
        
        if verbose:
            for line in info:
                print line
            
        self._resultsFile = ResultsFile(outdir + "/results", info)
        self.loadOpenings(openings);

    def loadOpenings(self, openings):
        assert(False)

    def playTournament(self):
        assert(False)
        
    def handleResult(self, swapped, result):
        ret = result
        if swapped:
            if (result[0:2] == 'B+'):
                ret = 'W+' + result[2:]
            elif (result[0:2] == 'W+'):
                ret = 'B+' + result[2:]
        return ret

    def playGame(self, gameIndex, currentRound, 
                 blackName, blackCmd, whiteName, whiteCmd,
                 opening, verbose):
        if verbose:
            print
            print "==========================================================="
            print "Game ", gameIndex
            print "==========================================================="
            
        bcmd = "nice " + blackCmd + " --seed %SRAND"
        wcmd = "nice " + whiteCmd + " --seed %SRAND"
        bLogName = self._outdir + "/" + blackName + "-" + str(gameIndex) \
                          + "-stderr.log"
        wLogName = self._outdir + "/" + whiteName + "-" + str(gameIndex) \
                          + "-stderr.log"
        black = Program("B", bcmd, bLogName, verbose)
        white = Program("W", wcmd, wLogName, verbose)
        
        resultBlack = "?"
        resultWhite = "?"
        error = 0
        errorMessage = ""
        game = Game()  # just a temporary
        gamePlayer = GamePlayer(black, white, self._size)
        try:
            game = gamePlayer.play(opening, verbose)
            swapped = game.playedSwap()
            resultBlack = self.handleResult(swapped, black.getResult())
            resultWhite = self.handleResult(swapped, white.getResult())
        except GamePlayer.Error:
            error = 1
            errorMessage = gamePlayer.getErrorMessage()
        except Program.Died:
            error = 1
            errorMessage = "program died"

        name = "%s/%04i" % (self._outdir, gameIndex)

        # save the result
        # recall it has been flipped if a swap move was played
        result = "?"
        if resultBlack == resultWhite:
            result = resultBlack
        game.setResult(result)

        # save it to the results file
        self._resultsFile.addResult(currentRound, opening,
                                    blackName, whiteName,
                                    resultBlack, resultWhite,
                                    # -1 so we don't count "resign" as a move
                                    game.getLength()-1,
                                    game.getElapsed("black"),
                                    game.getElapsed("white"),
                                    error, errorMessage)

        # save game
        gamePlayer.save(name + ".sgf", name, resultBlack, resultWhite)
        if error:
            print "Error: Game", gameIndex
        for program in [black, white]:
            try:
                program.sendCommand("quit");
            except Program.Died:
                pass
            
        return game

#----------------------------------------------------------------------------

# Plays a standard iterative tournament:
#   For each round, each program takes each opening as black.
class IterativeTournament(Tournament):
    def loadOpenings(self, openings):
        if (openings != ''):
            self._openings = []
            f = open(openings, 'r')
            lines = f.readlines()
            f.close()
            for line in lines:
                self._openings.append(string.strip(line))

    def playTournament(self):
        gamesPerRound = 2*len(self._openings);
        first = self._resultsFile.getLastIndex() + 1
        for i in range(first, self._rounds*gamesPerRound):
            currentRound = i / gamesPerRound;
            gameInRound = i % gamesPerRound;
            openingIndex = gameInRound / 2;
            
            opening = self._openings[openingIndex];
            
            if ((i % 2) == 0):
                self.playGame(i, currentRound,
                              self._p1name, self._p1cmd,
                              self._p2name, self._p2cmd,
                              opening, self._verbose)
            else:
                self.playGame(i, currentRound,
                              self._p2name, self._p2cmd,
                              self._p1name, self._p1cmd,
                              opening, self._verbose)

        
#----------------------------------------------------------------------------

# Plays a random tournament:
#   For each round, pick a random opening (weighted), then:
#     if round is even, program one takes black, otherwise
#     program two takes black.
class RandomTournament(Tournament):
    def loadOpenings(self, openings):
        if (openings != ''):
            self._openings = []
            f = open(openings, 'r')
            lines = f.readlines()
            f.close()
            sum = 0
            for line in lines:
                stripped = line.strip();
                array = stripped.split(' ')
                sum = sum + float(array[0])
                moves = stripped[len(array[0]):].strip()
                self._openings.append([sum, moves])
            self._maxWeight = sum;
            print self._openings;
            
    def pickOpening(self):
        randomWeight = random.random() * self._maxWeight;
        for i in range(len(self._openings)):
            if randomWeight < self._openings[i][0]:
                return self._openings[i][1]
        assert(False);
        
    def playTournament(self):
        first = self._resultsFile.getLastIndex() + 1
        for currentRound in range(first, self._rounds):
            opening = self.pickOpening();
            if ((currentRound % 2) == 0):
                self.playGame(currentRound, currentRound,
                              self._p1name, self._p1cmd,
                              self._p2name, self._p2cmd,
                              opening, self._verbose)
            else:
                self.playGame(currentRound, currentRound,
                              self._p2name, self._p2cmd,
                              self._p1name, self._p1cmd,
                              opening, self._verbose)

#----------------------------------------------------------------------------
