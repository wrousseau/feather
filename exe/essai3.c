int main(int argc, char** argv) {
   int x;
   int y;
   x =  argc; /* x8 */
   x = x*x-2*x; /* x10 */
   y = 4+argc; /* y11 */
   do { /* y4 = Phi(y11, y6), x1 = Phi(x10, x12) */
      if (x > 2) { 
         x=x-1; /* x19 */
      }
      else {
         x = x-1; /* x16 */
         if (2*x > y) {
            y = 2; /* y18 */
         }
         else {
            y = 3; /* y18 */
         };
         /* y5 = phi(y18, y4), x2 = phi(x1, x17) */
      };
      /* y6 = phi(y4, y5), x3 = phi(x19, x16) */
      x=x-1; /* x12 */
   } while (x > 0);
   return 0;
}

