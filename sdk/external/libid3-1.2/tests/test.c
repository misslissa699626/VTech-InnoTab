#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <id3.h>
#include "list.h"

#define WORD 1000
#define HUGE_STR_LEN 8192

id3flags display= ARTIST_TAG;

int result= 0;
char *export_dir;

FILE *openfile= NULL;

char *compare_file= "/tmp/id3-compare";

#define ERROR_FILE {fprintf(stderr, "ERROR %s %d : %s(%d)\n", __FILE__, __LINE__, strerror(errno), errno);fflush(stderr);exit(1);};

void diff_file(char *filename) 
{
  char newfile[HUGE_STRING_LEN];
  caddr_t result, test;
  struct stat test_buf, result_buf;
  int fd;

  snprintf(newfile, HUGE_STRING_LEN, "%s/%.*sresult", export_dir, (int)((strlen(filename))-3), filename);

  printf("Diffing %s:%s\n", compare_file, newfile);

  if (stat(compare_file, &result_buf)) 
    ERROR_FILE;

  if (stat(newfile, &test_buf)) 
    ERROR_FILE;


  if (result_buf.st_size != test_buf.st_size || !result_buf.st_size)
  {
    fprintf(stderr, "Died, files were of different size, on results from %s\n", newfile);
    exit(1);
  }

  if ((fd= open(newfile, O_RDONLY)) ==  -1) 
    ERROR_FILE;

  result= mmap(NULL, result_buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (result ==(caddr_t)-1)
    ERROR_FILE;

  close(fd);

  if ((fd= open(compare_file, O_RDONLY)) ==  -1) 
    ERROR_FILE;

  test= mmap(NULL, test_buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (test ==(caddr_t)-1)
    ERROR_FILE;

  close(fd);

  if (memcmp(test, result, result_buf.st_size))
  {
    fprintf(stderr, "Died on results from %s\n", newfile);
    exit(1);
  }
  munmap(result, result_buf.st_size);
  munmap(test, test_buf.st_size);
}

void print_tag(ID3 *info, const char *name, const char *ptr, size_t length) 
{
  fprintf(openfile, "TAG %s(%d):", name, (int)length);
  fwrite(ptr, sizeof(char), (int)length, openfile);
  fprintf(openfile,"\n");
}

void printFailedID3(char *filename) 
{
  FILE *file;
  char newfile[HUGE_STRING_LEN];

  if (result)
    snprintf(newfile, HUGE_STRING_LEN, "%s/%.*sresult", export_dir, (int)((strlen(filename))-3), filename);
  else
    snprintf(newfile, HUGE_STRING_LEN, "%s", compare_file);

  if ((file= fopen(newfile, "w")) == NULL)
  {
    printf("Error creating result for %s(%s)\n", newfile, strerror(errno));
    exit(1);
  }
  fprintf(file, "Failed\n");

  fclose(file);
}

void printID3(ID3 *info, char *filename) 
{
  FILE *file;
  char newfile[HUGE_STRING_LEN];


  if (result)
    snprintf(newfile, HUGE_STRING_LEN, "%s/%.*sresult", export_dir, (int)((strlen(filename))-3), filename);
  else
    snprintf(newfile, HUGE_STRING_LEN, "%s", compare_file);

  if ((file= fopen(newfile, "w")) == NULL)
  {
    printf("Error creating result for %s(%s)\n", newfile, strerror(errno));
    exit(1);
  }

  if (info->mask & TITLE_TAG )
    fprintf(file, "Title %s \n", info->title);
  if (info->mask & ARTIST_TAG )
    fprintf(file, "Artist %s \n", info->artist);
  if (info->mask & ALBUM_TAG )
    fprintf(file, "Album %s \n", info->album);
  if (info->mask & YEAR_TAG )
    fprintf(file, "Year %s \n", info->year);
  if (info->mask & COMMENT_TAG )
    fprintf(file, "Comment %s \n", info->comment);
  if (info->mask & TRACK_TAG )
    fprintf(file, "Track %s\n", info->track);
  if (info->mask & GENRE_TAG )
    fprintf(file, "Genre %s \n", info->genre);
  if (info->mask & SIGNATURE_TAG )
    fprintf(file, "Signature %s \n", info->signature);
  fprintf(file, "Version %s \n", info->version);

  fclose(file);
}

void load_directory(char *param, ID3 *info) 
{
  DIR *dir;
  struct dirent *file= NULL;
  id3_return rc= ID3_OK;


  if (!(dir= opendir(param))) 
  {
    printf("No such directory as %s\n", param);
    return;

  } 

  while ((file= readdir(dir))) 
  {
    char buffer[HUGE_STRING_LEN];
    struct stat sbuf;

    snprintf(buffer, HUGE_STRING_LEN, "%s/%s", param, file->d_name);

    if (stat(buffer, &sbuf) == 0)
    {
      if (S_ISREG(sbuf.st_mode)) 
      {
        printf("Found file %s\n", file->d_name);
        if (!(info= create_ID3(info))) 
        {
          printf("Create Failed\n");
          goto error;
        }

        if ((rc= parse_file_ID3(info, buffer))) 
        {
          printf("File: %s\n  Failed with code %d\n", buffer, (int)rc);

        } 
        else 
        {
          printID3(info, file->d_name);
        }

      } 
      else if (S_ISDIR(sbuf.st_mode)) 
      {
        if (file->d_name[0] != '.') 
        {
          printf("Recursing into %s\n", file->d_name);
          load_directory(buffer, info);
        }
      }
    } 
    else 
    {
      printf("Could not stat %s\n", buffer);
    }
  }
error:
  closedir(dir);
}

int main(int argc, char *argv[]) 
{
  ID3 id3;
  ID3 *info= create_ID3(NULL);
  char **file;
  id3_return rc= ID3_OK;
  char file_buffer[HUGE_STRING_LEN];
  unsigned char buffer[HUGE_STRING_LEN];
  char export_buffer[HUGE_STRING_LEN];
  char tags_buffer[HUGE_STRING_LEN];

  if (argc > 1 && !strcmp(argv[1], "--result"))
    result++;

  export_dir= "result-normal";
  for (file= list; *file; file++)
  {
    if (!(info= create_ID3(info))) 
    {
      printf("Create Failed\n");
      return 0;
    }

    sprintf(file_buffer, "tags/%s", *file);

    if ((rc= parse_file_ID3(info, file_buffer))) 
      printFailedID3(*file);
    else 
      printID3(info, *file);

    if (!result)
      diff_file(*file);
  }

  if (destroy_ID3(info)) 
  {
    printf("Destroy Failed\n");
  }
  info= NULL;

  export_dir= "result-memory";
  for (file= list; *file; file++)
  {
    char file_buffer[HUGE_STR_LEN];
    unsigned char buffer[HUGE_STR_LEN];
    bzero(buffer, HUGE_STR_LEN);

    if (!(info= create_ID3(info))) 
    {
      printf("Create Failed\n");
      return 0;
    }
    set_memory_ID3(info, buffer, HUGE_STR_LEN);

    sprintf(file_buffer, "tags/%s", *file);
    if ((rc= parse_file_ID3(info, file_buffer))) 
      printFailedID3(*file);
    else 
      printID3(info, *file);

    if (!result)
      diff_file(*file);
  }

  if (destroy_ID3(info)) 
  {
    printf("Destroy Failed\n");
  }
  info= NULL;

  export_dir= "result-function";
  for ( file= list; *file; file++)
  {
    char newfile[HUGE_STRING_LEN];

    if (!(info= create_ID3(info))) 
    {
      printf("Create Failed\n");
      return 0;
    }
    info->processor= print_tag;

    if (result)
      snprintf(newfile, HUGE_STRING_LEN, "%s/%.*sresult", export_dir, (int)((strlen(*file))-3), *file);
    else
      snprintf(newfile, HUGE_STRING_LEN, "%s", compare_file);
    if ((openfile= fopen(newfile, "w")) == NULL)
    {
      printf("Error creating result for %s(%s)\n", newfile, strerror(errno));
      exit(1);
    }

    sprintf(newfile, "tags/%s", *file);
    if ((rc= parse_file_ID3(info, newfile))) 
      printFailedID3(*file);

    fclose(openfile);

    if (!result)
      diff_file(*file);
  }

  if (destroy_ID3(info)) 
    printf("Destroy Failed\n");

#ifdef NOT_DONE
  export_dir= "result-preset-memory";
  memset(&id3, 0, sizeof(ID3));
  for ( file= list; *file; file++)
  {
    char newfile[HUGE_STRING_LEN];

    if (!(info= create_ID3(&id3))) 
    {
      printf("Create Failed\n");
      return 0;
    }
    info->processor= print_tag;

    if (result)
      snprintf(newfile, HUGE_STRING_LEN, "%s/%.*sresult", export_dir, (int)((strlen(*file))-3), *file);
    else
      snprintf(newfile, HUGE_STRING_LEN, "%s", compare_file);
    if ((openfile= fopen(newfile, "w")) == NULL)
    {
      printf("Error creating result for %s(%s)\n", newfile, strerror(errno));
      exit(1);
    }

    sprintf(buffer, "tags/%s", *file);
    if ((rc= parse_file_ID3(info, buffer))) 
      printFailedID3(*file);

    fclose(openfile);

    if (!result)
      diff_file(*file);
  }

  if (destroy_ID3(info)) 
    printf("Destroy Failed\n");
#endif

  return 0;
}
