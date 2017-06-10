# Words-with-Bots
Bot that plays Words with Friends

Commands:
	pm [word] [row(1-11)] [column(1-11)] [down(d) OR across(default)]
		Plays a move
	ra [+<letters>] [-<letters>]
		Adds/removes letters to the rack
	lm
		List legal moves, sorted decreasing by score
	pr [<flags>]
		Prints board if flags contains 'b', rack if flags contains 'r', adjacent word scores if flags contains 'a' (dev)
	word [word]
		Check if word is in the dictionary