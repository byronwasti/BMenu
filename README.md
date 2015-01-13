BMenu
=====

A simple, flexible, stand-alone pie-menu for linux based on xcb.

# Installation

Build the binary.

	make

Create config file 

	.config/BMenu/BMenurc

# Usage
Map to a hotkey to run and the file output is piped to. 
Example:
	
	BMenu | bmenu_out

You can then use these commands (with a bash script) to run programs, or do fancy things.

# Syntax

# Debugging
Just run `BMenu` in terminal and see the output. 

# Options
See the sample config file `BMenurc` and sample bash script `BMenubash` in `config/BMenu`.

# TODO
Make it functional.

# Bugs
It is not currently functional.
