int f(int x) {
   return x+2;
}

int main(int argc, char** argv) {
  int x;
  int y;
  x = argc;
  y = 2;
  x = x * x;
  do {
     x = x+2;
  } while (x < 100);

  if (x > y) {
    int  k;
    k = 8;
    x = 4 + x * 2 - 1;
  }
  else {
    x = x + 1 - f(y);
  };
  return x + 3;
}

