# Rinks #

Curling tournament planner.

Rinks is a simple tournament planner for curling events. Teams may be assigned to groups
which first should play internally. Pairings will be generated on the fly when entering
results from previous rounds. No already played pairings will occur unless an override
exists.

## Disclaimer ##

Please keep in mind that this was written in a very short time to meet a deadline.
It works™ but is far from ready for productive use. The user interface is at some
points not very intuitive (e.g. you can make all settings but nothing happens because
you forgot to create a game).

But I’m happy to mention that it was already successfully used in the celebratory
tournament of the Curling Club in Chemnitz in May 2014 as well as the 5th international
Curling Tournament in Chemnitz in May 2015.

Main critic points:

 * Only German strings:
   I wrote this explicitly for the mentioned tournament and had no time nor a windows
   copy to employ and test localization.
 * Usage of gtk2:
   The software was intended to work under windows and my cross-compiler environment only
   supported gtk2 at this time.
 * Quality: well, see above

There is still a lot to do. If you think this could be useful to you please feel free
to contact me and maybe I get some motivation to work on this again.

## Usage ##

First create a new tournament. Apply general settings and try not to change that later
(another TODO). There should be an even number of teams in each group.
Create rounds and encounters. You may also create overrides. In the overview section
you can select the games you want to print and output this list to a pdf.
Results are entered in the games section for each round.

Ranking is done points/ends/stones.

## License ##

Rinks is released under MIT license. See LICENSE.
