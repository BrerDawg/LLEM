# LLEM
Lunar lander game with the regular features found in this type of game, it was inspired by the 1970's Atari vector graphics CRT arcade game.  Features high quality line rendition which minimizes jaggies.

Ship lines can be either just a black drawn line you see belonging to the ship. Or, they can be normally invisible landing feet (i.e. the leg's foot pad) collision detection lines (green) or crash collision detections lines (red), these are only visible while editing. The ship's green landing feet/pad collision detections lines must be around the actual drawn black landing feet/pad lines, the more there are the better the collision detection will work (due to sampling nature of ship position update code). They must be completely horizontal (dy = 0, use 'l' key), and be in left and right pairs that share the same y value.

