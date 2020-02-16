# Words-with-Bots
Bot that plays Words with Friends

Finds the highest scoring move on any board instantaneously! Crush your friends and ruin the spirit of competition with this bot!

## Usage:

	./wwf [-b=<board_file>] [-t=<tile_file>] [-d=<dictionaryfile>] [-c=<configfolder>] [-r=<depth>]

	-c: The folder with config files (board, tile, dictionary). Run with -c=wwf15 to play on the 15x15 board. (default 11x11)
	-r: Forces a rebuild of the trie with given depth (max 5 for now)

To get started,
```shell script
cmake
make
./wwf

```


## Commands:

	game
		Take turns playing with the bot.
		<word>: plays <word> (see pm below for options)
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
		Prints one or more things corresponding to the characters in <flags>:
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

### Printing format:
`.` is empty space  
`?` is double word score  
`!` is triple word score  
`'` is double letter score  
`*` is triple letter score  
`_` is a blank tile
```
 0 1 2 3 4 5 6 7 8 9 0 1
 1 * . ! . . . . . ! . *
 2 . ? . . . ? . . . ? .
 3 ! . * . ' . ' . * . !
 4 . . . * . . . * . . .
 5 . . ' . . . . . ' . .
 6 . ? . . . N O . . ? .
 7 . . ' . . . X . ' . .
 8 . . . * . . . * . . .
 9 ! . * . ' . ' . * . !
 0 . ? . . . ? . . . ? .
 1 * . ! . . . . . ! . *
```
_Player 1 played NO across, and Player 2 played OX down_

## Notes
Sorry the code is so poorly commented and organized! It was hacked together during a weekend a long time ago and I'm still strugging to understand what the heck it is I did.  
Pull requests (especially refactorings) are welcome!