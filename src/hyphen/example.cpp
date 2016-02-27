#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "hyphen.h"
#include "stll/utf-8.h"

#include <fstream>

#define BUFSIZE 1000

void help()
{
  fprintf(stderr,"correct syntax is:\n");
  fprintf(stderr,"example hyphen_dictionary_file file_of_words_to_check\n");
}

/* list possible hyphenations with -dd option (example for the usage of the hyphenate2() function) */

using namespace STLL::internal;

/* get the word with all possible hyphenations (output: hyphword) */
#ifdef UTF8
void hnj_hyphen_hyphword(const char * word, int l, const std::vector<HyphenDict<char>::Hyphens> & hyphens, char * hyphword)
#endif
#ifdef UTF32
void hnj_hyphen_hyphword(const char32_t * word, int l, const std::vector<HyphenDict<char32_t>::Hyphens> & hyphens, char32_t * hyphword)
#endif
{
  int hyphenslen = l + 5;

  int j = 0;
  for (size_t i = 0; i < hyphens.size(); i++, j++)
  {
    if ((hyphens[i].hyphens % 2) == 1)
    {
      hyphword[j] = word[i];
      if (hyphens[i].rep->length() > 0)
      {
        size_t offset = j - hyphens[i].pos + 1;

        for (size_t k = 0; k < hyphens[i].rep->length(); k++)
          hyphword[offset+k] = hyphens[i].rep->at(k);
        hyphword[hyphenslen-1] = '\0';
        j += hyphens[i].rep->length() - hyphens[i].pos;
        i += hyphens[i].cut - hyphens[i].pos;
      }
      else
        hyphword[++j] = '=';
    }
    else
      hyphword[j] = word[i];
  }
  hyphword[j] = '\0';
}


int main(int /*argc*/, char** argv)
{
  int df;
  int wtc;
  int k, n, i;
  char buf[BUFSIZE + 1];
#ifdef UTF8
  char lcword[BUFSIZE + 1];
  char hword[BUFSIZE * 2];
#endif
#ifdef UTF32
  char32_t lcword[BUFSIZE + 1];
  char32_t hword[BUFSIZE * 2];
#endif
  int arg = 1;

  if (argv[arg])
  {
    df = arg++;
  }
  else
  {
    help();
    exit(1);
  }

  if (argv[arg])
  {
    wtc = arg++;
  }
  else
  {
    help();
    exit(1);
  }

  /* load the hyphenation dictionary */
  std::ifstream f(argv[df]);
  if (!f)
  {
    fprintf(stderr, "Couldn't find file %s\n", argv[df]);
    fflush(stderr);
    exit(1);
  }

#ifdef UTF8
  HyphenDict<char> dict(f);
#endif

#ifdef UTF32
  HyphenDict<char32_t> dict(f);
#endif

  /* open the words to check list */
  FILE * wtclst = fopen(argv[wtc],"r");

  if (!wtclst) {
    fprintf(stderr,"Error - could not open file of words to check\n");
    exit(1);
  }

#ifdef UTF8
  std::vector<HyphenDict<char>::Hyphens> hyphens;
#endif
#ifdef UTF32
  std::vector<HyphenDict<char32_t>::Hyphens> hyphens;
#endif

  /* now read each word from the wtc file */
  while (fgets(buf, BUFSIZE, wtclst) != NULL)
  {
    k = strlen(buf);

    // remove newlines at the end of the buffer
    if (k && buf[k - 1] == '\n') buf[k - 1] = '\0';
    if (k >=2 && buf[k - 2] == '\r') buf[k-- - 2] = '\0';

#ifdef UTF32
    std::u32string buf2 = STLL::u8_convertToU32(buf);
#endif

#ifdef UTF8
    /* basic ascii lower-case, not suitable for real-world usage*/
    for (i = 0; i < k; ++i)
    {
      lcword[i] = buf[i];
      if ( (lcword[i] >= 'A') && (lcword[i] <= 'Z') )
        lcword[i] += 32;
    }
#endif

#ifdef UTF32
    /* basic ascii lower-case, not suitable for real-world usage*/
    for (i = 0; i < k; ++i)
    {
      lcword[i] = buf2[i];
      if ( (lcword[i] >= U'A') && (lcword[i] <= U'Z') )
        lcword[i] += 32;
    }
#endif

    /* first remove any trailing periods */
    n = k-1;
#ifdef UTF8
    while((n >=0) && (lcword[n] == '.')) n--;
#endif
#ifdef UTF32
    while((n >=0) && (lcword[n] == U'.')) n--;
#endif
    n++;

    /* now actually try to hyphenate the word */

    hword[0] = '\0';

    for (int cnt = 0; cnt < 1; cnt++)
    {
      dict.hyphenate(lcword, hyphens);

      hnj_hyphen_hyphword(lcword, n-1, hyphens, hword);

      int x = 0;
      while (hword[x])
      {
#ifdef UTF8
        printf("%c", hword[x]);
#endif
#ifdef UTF32
        printf("%s", STLL::U32ToUTF8(hword[x]).c_str());
#endif
        x++;
      }
      printf("\n");
    }
  }

  fclose(wtclst);

  return 0;
}
