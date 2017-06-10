# Words-with-Bots
Bot that plays Words with Friends

Commands:
	pm [word] [row(1-11)] [column(1-11)] [down(d) OR across(default)]
		Plays a move
	rm [row(1-11)] [column(1-11)]
		Removes the letter at the position
	ra [+<letters>] [-<letters>]
		Adds/removes the specified letters on the rack, or a random amount from/to the bag if a number is passed in
	lm
		List legal moves, sorted decreasing by score
	pr [<flags>]
		Prints board if flags contains 'b', rack if flags contains 'r', adjacent word scores if flags contains 'a' (dev)
	word [word]
		Check if word is in the dictionary
	file [filename]
		take the file as input (like a pipe without EOF)
	clear
		Clears the board