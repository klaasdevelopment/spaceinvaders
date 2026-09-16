# Space Invaders

A standalone C/raylib recreation of the original arcade game's core rules.
Requires raylib, a C99 compiler, Make, and optionally pkg-config.

```sh
make run
```

Press Enter to start or restart, left/right arrows to move, Space to fire,
P to pause, and Escape to quit. Holding Space fires again when the previous
shot disappears. Only one player shot can be active.

Five rows of eleven invaders march sideways, descend at the edges, and
accelerate as their numbers fall. Bottommost invaders fire at the player.
Four destructible shields absorb shots from either side and are eroded by
the descending formation. Losing all three lives or letting the invaders
reach the cannon ends the game. Clearing a wave starts a harder formation.

The top row awards 30 points, the middle two 20, and the bottom two 10.
The mystery ship awards 50, 100, 150, or 300 points on a shot-count cycle.
One extra life is awarded at 1,500 points. High score lasts for the session.

Sound effects are synthesized approximations of the arcade shot, explosion,
four-note marching rhythm, and UFO sound; they are not original recordings.
This is a recreation, not a cycle-accurate emulation. Timing, artwork, enemy
shot patterns, and collision details may differ from the original hardware.

Run `make check` for game-state, shield, score, bonus-life, and reset checks.

Rule references:
- https://strategywiki.org/wiki/Space_Invaders/Gameplay
- https://shmups.wiki/library/Space_Invaders
