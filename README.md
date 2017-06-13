# Words-with-Bots
Bot that plays Words with Friends

With the ability to determine the highest scoring move on any board instantaneously!

Usage:

	./a [-b=<boardfile>] [-t=<tilefile>] [-d=<dictionaryfile>] [-c=<configfolder>] [-r=<depth>]

	-r: Forces a rebuild of the trie with given depth (max 5 for now)

Commands:

	game
		Take turns playing with the bot.
		.: pass
		! <letters>: swap letters on the rack
	pm [-f] <word> [<row>] [<column>] [d] [<blankposition>]
		Plays a move. d indicates down and row,col are 1-indexed but blankposition is 0-indexed. Default is horizontal in the center of the board.
		-f: forces the move even if illegal
	pb
		Plays the best move with the given rack, by score and rack leave heuristics
	il <word> [<row>] [<column>] [d]
		Displays info about a move, including score and legality. Uses same syntax as pm.
	rm <row> <column>
		Removes the letter at the position
	ra [+<letters>] [-<letters>]
		Adds/removes the specified letters on the rack, or a random amount from the bag if a number is passed in
	lm [-n=<number>] [-s=<minscore>] [-h]
		List legal moves, sorted decreasing by score
		-n: only top moves
		-s: moves with minimum score
		-h: calculate and list by heuristic score
	pr <flags>
		if <flags> contains:
		b: print board
		r: print rack
		l: print letter bonus squares
		w: print word bonus squares
		a: print adjacent word scores
		p: print board tile values (0 for blanks)
	word <word>
		Check if <word> is in the dictionary
	ip <string>
		Check if a string is in the trie
	file <filename>
		Take the file as input (like a pipe without EOF)
	clear
		Clears the board and resets the first move flag
