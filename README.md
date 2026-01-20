# DE10-Lite / DE1-SoC VGA Two-Player Game
## Overview

This project implements a real-time two-player game on an FPGA platform (DE10-Lite or DE1-SoC), using VGA output, 7-segment displays, LEDs, switches, and push buttons. The game is controlled through hardware interrupts for the push buttons and uses a software timer for movement updates.

Players navigate colored pixels (red and blue) around the screen, avoiding obstacles and walls. The game keeps track of scores using 7-segment displays and indicates turns using LEDs.

The game is implemented in C for a Nios II / RISC-V soft-core CPU running on the FPGA.

![Game Demo](Tron.png)

## How It Works

__Initialization__

* Sets up VGA display, borders, and obstacles

* Resets player positions

* Configures interrupts for timer and push buttons

__Gameplay__

* Players move automatically in their current direction

* KEY0 and KEY1 allow players to turn left or right

* Collision detection prevents movement into walls or obstacles
 
__Timer Interrupt__

* Controls the game loop

* Updates player positions and draws them on VGA

Key Interrupt

* Detects button presses

* Updates pending_turn variable

* Displays turn on LEDs

__Scoring__

* When a player collides with a wall or obstacle, the round ends

* Scores are updated on the 7-segment displays

* First player to reach 9 points wins
