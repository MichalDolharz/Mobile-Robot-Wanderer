# Mobile Robot "Wanderer" – RM # {#mainpage}

## Project status

Finished.

For more information check out [the final report](https://github.com/Repti993/Mobile-Robot-Wanderer/blob/main/reports/Report_stage_3_final.pdf) (only polish language version).

## Project description
The goal of this project is to build a wandering mobile robot "Wanderer" with two operating modes: autonomical and remote.

The autonomical mode is based on a simple ambient sensor HC-SR04. After detecting an obstacle in front of it, the robot will make a turn and continue its wanderings. 

The remote mode is perform by a dedicated remote control with a joystick and three LED diodes. 

## Dokumentacja i archiwizacja

A source code documentation is created with Doxygen with graphic interface Doxywizard. It is available in [this repository as a HTML file](https://github.com/Repti993/Mobile-Robot-Wanderer/tree/main/doc/html), but is should be also available online at [the homepage ~mdolharz at WUST panamint server](http://panamint.ict.pwr.wroc.pl/~mdolharz/rm/).

## Electronics

A robot scheme
!(A robot scheme)[https://github.com/Repti993/Mobile-Robot-Wanderer/blob/main/img/robot_scheme.png]

A remote controle scheme
!(A remote controle scheme)[https://github.com/Repti993/Mobile-Robot-Wanderer/blob/main/img/remote_control_scheme.png]

For a PCB project check out [the final report](https://github.com/Repti993/Mobile-Robot-Wanderer/blob/main/reports/Report_stage_3_final.pdf)
 
## Photos

A robot photo
!(A robot scheme)[https://github.com/Repti993/Mobile-Robot-Wanderer/blob/main/img/robot_photo.jpg]

A remote controle photo
!(A remote controle scheme)[https://github.com/Repti993/Mobile-Robot-Wanderer/blob/main/img/remote_control_photo.jpg]

## Issues

To be honest, this was my first time with that kind of project, so some problems occured.

1. Due to a typo in schemes, a generated PCB projects don't include one connection (CNS instead of CSN).
2. Robot motors are controlled by the same PWM signal, so the robot is not able to turn by wheels different speed. It can do an in place turn or on the one wheel only turn. 

