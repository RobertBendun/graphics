%.out: %.cc *.hh
	g++ -std=c++20 -Wall -Wextra -O2 $< -o $@
