Doomenstein Gold:
Player Control:
Press down 'Q' to enter listen mode. Nearby enemies will show on the screen when you enter listen mode.
Press down Number 3 to equip the new Spear auto-locking weapon. Put enemies in the aiming circle and wait to shoot
The rest of the control is the same as the Doomenstein Alpha.
	

New Weapon: Spear Target Locking Missile Launcher

1. During reloading, the aiming will turn white.
2. During locking, the aiming will turn yellow. The locking dot will also slowly move towards the locking target.
3. After locking the target for a period of time. The weapon will remember the target and aiming will turn green, showing it is ready to shoot.
4. If the player escapes and misses the target, the weapon will memorize the target for a period of time before forgetting its locking target.
5. The movement of the missile is controlled by a cubic Bezier curve which adjusts its trajectory every frame based on its position and target position.

New Enemy: Ghost and energy shield

1. Ghost has a smaller and floating-in-air collision box, which makes it harder to shoot. 
2. After the ghost gets damaged, it will be invisible for a few seconds and teleport to a random direction for a small amount of distance. Therefore, it is hard for the player to predict where to shoot after it disappears. But players could use the spear to detect and lock the invisible enemy. Players could also use listen mode to see which direction the enemy coming at them.
3. Energy shield becomes a component that every enemy could use and specified by the XML document. If the enemy is defined as "shielded", an actor with a shield will spawn in front of the enemy actor and move with it.
4. Using the pistol to shoot at the shield you will see a ricochet VFX. The shield will check its orientation and incoming collision or raycast normal to decide whether the player hits the shield or the enemy behind it.

New Game Mode: Listen mode

1. All the enemies are hard to kill and the idea is to let the player use listen mode and the spear-locking weapon to detect where the enemies are and kill in advance before getting in trouble.
2. Otherwise, the enemies with higher attack damage will kill the player easily.
3. Listen mode allows nearby enemies to show on the screen and highlight in red. If the player can also hear their active sound but not see them on the screen, it means the player needs to hear the 3D sound and tell which direction they are in.
4. Half-height walls are introduced in the maze. Its idea is to allow players to use the spear-locking weapon to shoot homing missiles over the wall to kill the enemy.
5. Once the player gets to the end of the maze, he/she will see a shining star. By reaching it, the player wins, and the game ends.
