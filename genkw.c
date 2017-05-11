/* ------------------------------------------------------------
 *
 *    Program to generate what is required for a keyword search.
 *
 *    Usage: sort <file containing one keyword per line> | genkw somename
 *
 *    It will generate somename.c and somename.h.
 *
 *    somename.h defines constants values for each of the keywords,
 *    and a function <somename>_search() that returns either a code
 *    or SOMENAME_NOT_FOUND.
 *
 *    Written by S. Faroult.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define MAXLEN      50
#define SAMPLE_SZ   20
#define OPTIONS     "rvc"

static char G_reversed = 0;

static void gen_search(FILE *fp, char *lowerpre, char *upperpre, char cs) {
    fprintf(fp, "extern int %s_search(char *w) {\n", lowerpre);
    fprintf(fp, "  int start = 0;\n");
    fprintf(fp, "  int end = %s_COUNT - 1;\n", upperpre);
    fprintf(fp, "  int mid;\n");
    fprintf(fp, "  int pos = %s_NOT_FOUND;\n", upperpre);
    fprintf(fp, "  int comp;\n\n");
    fprintf(fp, "  if (w) {\n");
    fprintf(fp, "    while(start<=end){\n");
    fprintf(fp, "      mid = (start + end) / 2;\n");
    fprintf(fp, "      if ((comp = str%scmp(G_%s_words[mid], w)) == 0) {\n",
                (cs ?"":"case"), lowerpre);
    fprintf(fp, "         pos = mid;\n");
    fprintf(fp, "         start = end + 1;\n");
    fprintf(fp, "       } else if ((mid < %s_COUNT-1)\n", upperpre);
    fprintf(fp, "               && ((comp = str%scmp(G_%s_words[mid+1], w)) == 0)) {\n",
                (cs ?"":"case"), lowerpre);
    fprintf(fp, "         pos = mid+1;\n");
    fprintf(fp, "         start = end + 1;\n");
    fprintf(fp, "      } else {\n");
    if (G_reversed) {
      fprintf(fp, "        if (comp > 0) {\n");
    } else {
      fprintf(fp, "        if (comp < 0) {\n");
    }
    fprintf(fp, "           start = mid + 1;\n");
    fprintf(fp, "        } else {\n");
    fprintf(fp, "           end = mid - 1;\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "      }\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "  }\n");
    fprintf(fp, "  return pos;\n");
    fprintf(fp, "}\n\n");
    if (G_reversed) {
      fprintf(fp, "extern int %s_best_match(char *w) {\n", lowerpre);
      fprintf(fp, "  int pos = %s_NOT_FOUND;\n", upperpre);
      fprintf(fp, "  int i = 0;\n");
      fprintf(fp, "  int comp = 1;\n\n");
      fprintf(fp, "  if (w) {\n");
      fprintf(fp, "    while ((i < %s_COUNT)\n", upperpre);
      fprintf(fp, "           && ((comp > 0)\n");
      fprintf(fp, "               || (%s*w%s == %s*(G_%s_words[i])%s))) {\n",
                  (cs?"":"toupper("),
                  (cs?"":")"),
                  (cs?"":"toupper("),
                  lowerpre,
                  (cs?"":")"));
      fprintf(fp,
              "      comp = strn%scmp(G_%s_words[i],w,strlen(G_%s_words[i]));\n",
                (cs ?"":"case"), lowerpre, lowerpre);
      fprintf(fp, "      i++;\n");
      fprintf(fp, "      if (comp == 0) {\n");
      fprintf(fp, "        break;\n");
      fprintf(fp, "      }\n");
      fprintf(fp, "    }\n");
      fprintf(fp, "    if (comp == 0) {\n");
      fprintf(fp, "      pos = i - 1;\n");
      fprintf(fp, "    }\n");
      fprintf(fp, "  }\n");
      fprintf(fp, "  return pos;\n");
      fprintf(fp, "}\n\n");
      fprintf(fp, "extern int %s_abbrev(char *w) {\n", lowerpre);
      fprintf(fp, "  int pos = %s_NOT_FOUND;\n", upperpre);
      fprintf(fp, "  int i = 0;\n");
      fprintf(fp, "  int len;\n");
      fprintf(fp, "  int comp = 1;\n\n");
      fprintf(fp, "  if (w) {\n");
      fprintf(fp, "    len = strlen(w);\n");
      fprintf(fp, "    while ((i < %s_COUNT) && (comp > 0)) {\n", upperpre);
      fprintf(fp, "      comp = strn%scmp(G_%s_words[i],w,len);\n",
                (cs ?"":"case"), lowerpre);
      fprintf(fp, "      i++;\n");
      fprintf(fp, "    }\n");
      fprintf(fp, "    if (comp == 0) {\n");
      fprintf(fp, "      pos = i - 1;\n");
      fprintf(fp, "      if ((i < %s_COUNT)\n", upperpre);
      fprintf(fp, "          && (strn%scmp(G_%s_words[i],w,len)==0)) {\n",
                    (cs ?"":"case"), lowerpre);
      fprintf(fp, "         pos = %s_AMBIGUOUS;\n", upperpre);
      fprintf(fp, "      }\n");
      fprintf(fp, "    }\n");
      fprintf(fp, "  }\n");
      fprintf(fp, "  return pos;\n");
      fprintf(fp, "}\n\n");
    }
    fprintf(fp, "extern char *%s_keyword(int code) {\n", lowerpre);
    fprintf(fp, "  if ((code >= 0) && (code < %s_COUNT)) {\n", upperpre);
    fprintf(fp, "    return G_%s_words[code];\n", lowerpre);
    fprintf(fp, "  } else {\n");
    fprintf(fp, "    return (char *)NULL;\n");
    fprintf(fp, "  }\n");
    fprintf(fp, "}\n\n");
}

static void usage(char *progname) {
   fprintf(stderr, "Usage: %s [flags] <name>\n", progname);
   fprintf(stderr, "       <name> is an arbitrary name used to name\n");
   fprintf(stderr, "       generated file and also used as prefix.\n");
   fprintf(stderr, " flags: -c  case sensitive search\n");
   fprintf(stderr, "            (default is case insensitive)\n");
   fprintf(stderr, "        -r  reversed (generates best match search)\n");
   fprintf(stderr, "        -v  verbose\n\n");
}

int main(int argc, char **argv) {
    FILE *fh;
    FILE *fc;
    char  fnameh[FILENAME_MAX];
    char  fnamec[FILENAME_MAX];
    char  uppername[MAXLEN];
    char  lowername[MAXLEN];
    char  line[MAXLEN];
    char  sample[SAMPLE_SZ+1];
    char *p;
    char *q;
    int   len;
    char  cs = 0;  // Case-sensitive
    char  verbose = 0;
    int   ch;
    int   cnt;
    int   i;

    while ((ch = getopt(argc, argv, OPTIONS)) != -1) {
       switch (ch) {
          case 'v':
               verbose = 1;
               break;
          case 'c':
               cs = 1;
               break;
          case 'r':
               G_reversed = 1;
               break;
          case '?':
          default:
               usage(argv[0]);
               return 1;
               break; /*NOTREACHED*/
       }
    }
    if ((argc - optind) != 1) {
       usage(argv[0]);
       return 1;
    }
    p = argv[optind];
    i = 0;
    while (*p) {
      uppername[i] = toupper(*p);
      lowername[i] = tolower(*p);
      p++;
      i++;
    }
    uppername[i] = '\0';
    lowername[i] = '\0';
    if (!G_reversed) {
       strcpy(sample, " ");
    } else {
       sample[0] = 127;
       sample[1] = 127;
       sample[2] = 127;
       sample[3] = '\0';
    }
    cnt = 0;
    sprintf(fnameh, "%s.h", lowername);
    sprintf(fnamec, "%s.c", lowername);
    if (verbose) {
       printf("-- generating %s and %s%s\n",
               fnameh, fnamec, (G_reversed ? " (reversed)" : ""));
    }
    if ((fc = fopen(fnamec, "w")) == NULL) {
       perror(fnamec);
       return 1;
    }
    if ((fh = fopen(fnameh, "w")) == NULL) {
       perror(fnameh);
       fclose(fc);
       return 1;
    }
    fprintf(fh, "#ifndef %s_HEADER\n\n", uppername);
    fprintf(fh, "#define %s_HEADER\n\n", uppername);
    if (G_reversed) {
       fprintf(fh, "#define %s_AMBIGUOUS\t-2\n", uppername);
    }
    fprintf(fh, "#define %s_NOT_FOUND\t-1\n", uppername);
    fprintf(fc, "#include <stdio.h>\n");
    if (!cs) {
      fprintf(fc, "#include <ctype.h>\n");
    }
    fprintf(fc, "#include <string.h>\n\n");
    fprintf(fc, "#include \"%s.h\"\n\n", lowername);
    fprintf(fc, "static char *G_%s_words[] = {\n", lowername);
    while (fgets(line, MAXLEN, stdin)) {
       p = line;
       while (isspace(*p)) {
          p++;
       }
       len = strlen(p);
       while (len && isspace(p[len-1])) {
          len--;
       }
       if (len) {
          cnt++;
          p[len] = '\0'; 
          if ((!G_reversed && strcmp(sample, p)>0)
              || (G_reversed && strcmp(sample, p)<0)) {
             fprintf(stderr, "Input data not correctly sorted\n");
             if (verbose) {
               fprintf(stderr, "-- problem encountered with %s\n", p);
             }
             fclose(fc);
             fclose(fh);
             unlink(fnamec);
             unlink(fnameh);
             return 1;
          }
          fprintf(fc, "    \"%s\",\n", p);
          strncpy(sample, p, SAMPLE_SZ);
          q = p;
          while (*q) {
             if (isalnum(*q)) {
               *q = toupper(*q);
             } else {
               *q = '_';
             }
             q++;
          }
          fprintf(fh, "#define %s_%s\t%3d\n", uppername, p, cnt - 1);
       }
    }
    fprintf(fc, "    NULL};\n\n");
    fprintf(fh, "\n#define %s_COUNT\t%d\n\n", uppername, cnt);
    fprintf(fh, "extern int   %s_search(char *w);\n", lowername);
    if (G_reversed) {
      fprintf(fh, "extern int   %s_best_match(char *w);\n", lowername);
      fprintf(fh, "extern int   %s_abbrev(char *w);\n", lowername);
    }
    fprintf(fh, "extern char *%s_keyword(int code);\n\n", lowername);
    fprintf(fh, "#endif\n");
    if (verbose) {
       printf("-- %d keywords\n", cnt);
    }
    gen_search(fc, lowername, uppername, cs);
    fclose(fc);
    fclose(fh);
    return 0;
}
