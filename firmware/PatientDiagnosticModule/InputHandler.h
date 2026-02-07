#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

void init_inputs();

int get_xDirection();
int get_yDirection();
bool button_pressed();

int get_distance();

void manage_LED(bool isNewMaze);


#endif