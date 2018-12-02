int main ()
{
  int i;
  int j;
  int k;

  i = 1;
  j = 1;
  k = 0;

  while (k < 100) {
    if (j < 20) {
      j = i;
      k = k + 1;
    } else {
      j = k;
      k = k + 2;
    }
  }

  printf("%d\n",j);
}

void main(void) {
  int k;
  k = 0;
  while (k < 100) {
    k = k + 1;
  }
  printf("%d\n", 1);
}