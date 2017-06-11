# Words-with-Bots
Bot that plays Words with Friends

Usage: ./a [-b <boardfile>] [-t <tilefile>] [-d <dictionaryfile>]

Commands:

	pm [-f] <word> [row(1-11)] [column(1-11)] [down(d)]
		Plays a move. Default is horizontal in the center of the board.
		-f: forces the move even if illegal
	pb [-k]
		Plays the best move with the given rack (currently the highest-scoring move)
		-k: keep the tiles on the rack
	rm <row(1-11)> <column(1-11)>
		Removes the letter at the position
	ra [+<letters>] [-<letters>]
		Adds/removes the specified letters on the rack, or a random amount from/to the bag if a number is passed in
	lm [-n=<number>] [-s=<minscore>]
		List legal moves, sorted decreasing by score
		-n: top moves
		-s: moves with minimum score
	pr <flags>
		if <flags> contains:
		b: print board
		r: print rack
		l: print letter bonus squares
		w: print word bonus squares
		a: print adjacent word scores
	word <word>
		Check if <word> is in the dictionary
	file <filename>
		Take the file as input (like a pipe without EOF)
	clear
		Clears the board
