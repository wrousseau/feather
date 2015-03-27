int main(int argc, char** argv) {
   int x;
   int y;
   x =  argc; /* x8 */
   x = x*x-2*x; /* x10 */
   y = 4+argc; /* y11 */
   do { /* y4 = Phi(y11, y6), x1 = Phi(x10, x12) */
      if (x > 2) { 
         --x; /* x19 */
      }
      else {
         if (2*x > y) {
            y = 2; /* y18 */
         }
         else {
            --x; /* x17 */
         };
         /* y5 = phi(y18, y4), x2 = phi(x1, x17) */
         --x; /* x16 */
      };
      /* y6 = phi(y4, y5), x3 = phi(x19, x16) */
      --x; /* x12 */
   } while (x > 0);
   return 0;
}

