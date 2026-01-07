# RealTime-Project-3

bashar@Ubuntu:~/test ape$ make
Compiling ape_simulation.c...
gcc -Wall -Wextra -O2 -std=c11 -c ape_simulation.c -o ape_simulation.o
ape_simulation.c: In function ‘move_female_to_position’:
ape_simulation.c:1114:9: warning: implicit declaration of function ‘usleep’; did you mean ‘sleep’? [-Wimplicit-function-declaration]
 1114 |         usleep(80000);
      |         ^~~~~~
      |         sleep
ape_simulation.c: In function ‘female_ape_function’:
ape_simulation.c:1136:10: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1136 |     if (f->visited_cells == NULL) {
      |          ^~
ape_simulation.c:1137:10: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1137 |         f->visited_cells = malloc(MAZE_SIZE * sizeof(int*));
      |          ^~
ape_simulation.c:1139:14: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1139 |             f->visited_cells[x] = malloc(MAZE_SIZE * sizeof(int));
      |              ^~
ape_simulation.c:1141:18: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1141 |                 f->visited_cells[x][y] = 0;
      |                  ^~
ape_simulation.c:1193:22: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1193 |                     f->visited_cells[x][y] = 0;
      |                      ^~
ape_simulation.c:1342:22: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1342 |                     f->visited_cells[current.x][current.y] += 2;
      |                      ^~
ape_simulation.c:1348:18: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1348 |                 f->visited_cells[current.x][current.y]++; // Mark as visited
      |                  ^~
ape_simulation.c:1359:30: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1359 |                         if (f->visited_cells[x][y] > 0) {
      |                              ^~
ape_simulation.c:1360:30: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1360 |                             f->visited_cells[x][y]--; // Fade memory
      |                              ^~
ape_simulation.c:1420:47: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1420 |                         local_visit_count += f->visited_cells[nx][ny];
      |                                               ^~
ape_simulation.c:1507:34: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1507 |                                 f->visited_cells[nx][ny] = 0;
      |                                  ^~
ape_simulation.c:1552:34: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1552 |                             if (f->visited_cells[nx][ny] > 0) {
      |                                  ^~
ape_simulation.c:1553:34: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1553 |                                 f->visited_cells[nx][ny]--;
      |                                  ^~
ape_simulation.c:1585:39: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1585 |                         int visits = f->visited_cells[pos.x][pos.y];
      |                                       ^~
ape_simulation.c:1811:22: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1811 |                 if (f->visited_cells[x][y] > 0) {
      |                      ^~
ape_simulation.c:1812:22: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1812 |                     f->visited_cells[x][y]--; // Gradual forgetting
      |                      ^~
ape_simulation.c:1822:10: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1822 |     if (f->visited_cells != NULL) {
      |          ^~
ape_simulation.c:1824:19: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1824 |             free(f->visited_cells[x]);
      |                   ^~
ape_simulation.c:1826:15: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1826 |         free(f->visited_cells);
      |               ^~
ape_simulation.c:1827:10: error: ‘FemaleApe’ has no member named ‘visited_cells’
 1827 |         f->visited_cells = NULL;
      |          ^~
ape_simulation.c: In function ‘keyboard’:
ape_simulation.c:2322:38: warning: unused parameter ‘x’ [-Wunused-parameter]
 2322 | void keyboard(unsigned char key, int x, int y) {
      |                                  ~~~~^
ape_simulation.c:2322:45: warning: unused parameter ‘y’ [-Wunused-parameter]
 2322 | void keyboard(unsigned char key, int x, int y) {
      |                                         ~~~~^
make: *** [Makefile:30: ape_simulation.o] Error 1
bashar@Ubuntu:~/test ape$ ^C
bashar@Ubuntu:~/test ape$ ^C
bashar@Ubuntu:~/test ape$ make clean
Cleaning build artifacts...
✓ Clean complete
bashar@Ubuntu:~/test ape$ make
Compiling ape_simulation.c...
gcc -Wall -Wextra -O2 -std=c11 -c ape_simulation.c -o ape_simulation.o
ape_simulation.c: In function ‘update_banana_awareness’:
ape_simulation.c:466:6: error: ‘FemaleApe’ has no member named ‘preferred_direction’
  466 |     f->preferred_direction = best_direction;
      |      ^~
ape_simulation.c:439:9: warning: unused variable ‘banana_directions’ [-Wunused-variable]
  439 |     int banana_directions[8] = {0};
      |         ^~~~~~~~~~~~~~~~~
ape_simulation.c: In function ‘move_female_to_position’:
ape_simulation.c:1153:9: warning: implicit declaration of function ‘usleep’; did you mean ‘sleep’? [-Wimplicit-function-declaration]
 1153 |         usleep(80000);
      |         ^~~~~~
      |         sleep
ape_simulation.c: In function ‘init_apes’:
ape_simulation.c:2294:17: error: ‘i’ undeclared (first use in this function)
 2294 |     female_apes[i].visited_cells = malloc(MAZE_SIZE * sizeof(int*));
      |                 ^
ape_simulation.c:2294:17: note: each undeclared identifier is reported only once for each function it appears in
ape_simulation.c: In function ‘keyboard’:
ape_simulation.c:2408:38: warning: unused parameter ‘x’ [-Wunused-parameter]
 2408 | void keyboard(unsigned char key, int x, int y) {
      |                                  ~~~~^
ape_simulation.c:2408:45: warning: unused parameter ‘y’ [-Wunused-parameter]
 2408 | void keyboard(unsigned char key, int x, int y) {
      |                                         ~~~~^
make: *** [Makefile:30: ape_simulation.o] Error 1
