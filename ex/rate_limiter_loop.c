extern int wait();
extern int input();
void rate_limiter() {
  int x_old;
  x_old = 0;
  while (1) {
    int x = input();
    if (x < -10000) x = -10000;
    if (x > 10000) x = 10000;
    if (x > x_old+10)
        x = x_old+10;
    if (x < x_old-10)
        x = x_old-10;
	while (wait()) {}
    x_old = x;
  }
}

