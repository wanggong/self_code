/*
      KMP Pattern matching algorithm
      dev.kpyc@gmail.com
*/

#include <iostream>
#include <fstream>
using namespace std;

void computeFailureArray(int* F, string P) {
   F[0] = 0;
   int i = 1;
   int j = 0;
   int m = P.length();
   
   while (i < m) {
      if (P[i] == P[j]) {
         F[i] = j + 1;
         i++;
         j++;
      } else if (j > 0) {
         j = F[j-1];
      } else {
         F[i] = 0;
         i++;
      }
   }
}

int main(int argc, char* argv[]) {

   // multi flag -m
   bool multi = false;
   bool noresults = true;
   
   argv++;
   argc--;

   for (int i=0; i<argc; i++) {
      if (string(argv[0]) == "-m") {
         multi = true;
         argv++;
         argc--;
      }
   }

   if (argc != 2) { 
      cerr << "Invalid arguments" << endl;
      cerr << "Usage ./kmp pattern file" << endl;
      return 3;
   }
   string P = argv[0];
   int m = P.length();
   
   ifstream fin;
   fin.open(argv[1]);
   
   if (!fin.is_open()) {
      cerr << "Unable to open file " << argv[1] << endl;
      cerr << "Exiting" << endl;
   }

   int* F = new int[m];
   computeFailureArray(F, P);

   int j = 0;
   
   int line_num = 0;
   streampos line_pos = 0;
   
   char c = fin.get();
   
   while (fin.good()) {     
      if (c == '\n') { 
         line_pos = fin.tellg();
         line_num ++;
      }
      if (c == P[j]) {
         if (j == m - 1) {
            noresults = false;
            char buf[256];
            fin.seekg(line_pos);
            fin.getline(buf,256);
            cout << line_num << ": " << buf << endl;
            if (!multi) {
               delete [] F;
               return 0;
            } else {
               j = 0;
               fin.getline(buf,256);
               c = fin.get();
            }          
         } else {
            c = fin.get();
            j++;
         }
      } else {
         if (j > 0) {
            j = F[j-1];
         } else {
            c = fin.get();
         }
      }
   }
   if (noresults) {
      delete [] F;
      return 1;
   } else {
      delete [] F;
      return 0;
   }
}
