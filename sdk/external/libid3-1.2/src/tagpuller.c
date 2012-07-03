#include <stdio.h>
#include <dirent.h>
#include <limits.h>
#include "id3.h"

char *export_dir;
char *original_dir;

void load_directory(char *param) 
{
  DIR *dir;
  struct dirent *file = NULL;

  if (!(dir = opendir(param))) 
  {
    printf("No such directory as %s\n", param);
    return;

  } 

  while ((file = readdir(dir))) 
  {
    char buffer[PATH_MAX];
    struct stat sbuf;
    snprintf(buffer, PATH_MAX, "%s/%s", param, file->d_name);
    if (stat(buffer, &sbuf) == 0)
    {
      if (S_ISREG(sbuf.st_mode)) 
      {
        size_t length = strlen(file->d_name);
        /* Less then 3 characters means no file extension */
        if ( length < 3 )
          continue;

        char *ptr = file->d_name + length -3;


        if (!(strcasecmp(ptr, "mp3")))
        {
          char newfile[PATH_MAX];
          if (export_dir)
            snprintf(newfile, PATH_MAX, "%s/%.*stag", export_dir, (int)(length-3), file->d_name );
          else
            snprintf(newfile, PATH_MAX, "%s/%.*stag", param, (int)(length-3), file->d_name );
          ID3_to_file(buffer, newfile) ;
          printf("Found file %s (%s)\n%s\n%s\n\n", file->d_name, ptr, buffer, newfile);
        }
      } 
      else if (S_ISDIR(sbuf.st_mode)) 
      {
        if (file->d_name[0] != '.') 
        {
          printf("Recursing into %s\n", file->d_name);
          load_directory(buffer);
        }
      }
    } 
    else 
    {
      printf("Could not stat %s\n", buffer);
    }
  }
  closedir(dir);
}

int main(int argc, char *argv[]) 
{
  export_dir = argc >= 3 ? argv[2] : NULL;
  printf("Doing %s\n", argv[1]);
  load_directory(argv[1]);

  return 0;
}

