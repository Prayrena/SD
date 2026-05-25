A2:
Player Control:
	Press key 'E' to control the spaceship to thrust
	Press key 'S' to control the spaceship rotate counter clock-wise
	Press key 'f' to control the spaceship rotate clock-wise
	Use xbox controller's left joystick to control the thrust direction and ship orientation

	Press key 'N' to respawn the spaceship, which will be relocated to the 	center of the window
	Use xbox controller's start button to respawn the player ship

	Press down key 'T' to enter slow-mode which all enemies' velocity will be slown down, release key 'T' to back to normal
	Press down xbox controller's right shoulder button to enter slow mode, release will back to normal

	Press down key spacebar to shoot continuously 
	Press down and hold xbox controller's button A to shoot continuously

	Press and hold xbox controller's left shoulder button to turn on the energy distribution system, then use right joystick to select which system 	you want to boost. Release the left shoulder button will turn off the energy distribution system manual.
	

Debug Control:
	Press 'P' to pause the game. Second 'p' press will make the game resume.

	Press 'o' to enter sigle frame mode. Another time 'o' press will only 	runs the next frame. Another time 'P' press will make the game back on 	normal running

Developer Mode Control:
	At any time, press 'F1' will enter the developer mode, which all current existing entities will show its physics and cosmetic radii, current 	velocity (units/second), and XY axis in local space.

	Press key 'K' to clear current level's enemies and enter next level.

Quit Game:
	Press 'Esc' to exit game

Bug:
	No deadly bug will cause the game to crash.

	When open the energy distribution system, it should not have the option selected unless player push the right joystick. But it seems like the 	right analog is not reseted

	The boids has the flock simulation effect as well as a attraction towards the player ship. But in some occasions, they will stuck together or 	shaking in the spot. I'll ask how could I solve this problem in office hour.

Deep Learning:

There is really a lot to talk about as I have not worked on any C++ project before this semester. But I would like to use this moment as a reflection for the past two months' study.

I think that the way I treated the debug process has changed a lot. I think I have spent nearly half my studying time on debugging and this is the way how I learn. I learned to know how C++ works, what logic problem I created, and how I misunderstood the mechanism of C++ and the project's framework. As time went by, I started to figure out which problem I needed to solve when I encountered certain bugs and it helped me to have a deeper understanding of Prof. Eiserloh's framework for the engine as well as coding conventions. Debugging should always be a part of programming work.

At the same time, I find it I am still unable to have better time management due to I have the relatively high amount of bugs. I usually overlook the time I need to deal with them and this makes me always hand in the assignment to the system last minute like right now. The architectural design work I did before depends on how much time and effort I put in purely. However, the programming work depends on the time as well as bugs created along the way. I don't have the ability to foresee how easy it is to finish one work because I could not predict the difficulty of building one feature as well as potential bugs in the way. I think I'll need to promote my skills in this field specifically in the future.
	