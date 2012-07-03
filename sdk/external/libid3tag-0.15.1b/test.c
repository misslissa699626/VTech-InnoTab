/*******************************************************************************
 * Copyright (C) 2009 SunMedia Technology (ChengDu) Co., Ltd.
 * All right reserved
 * 
 *    File Name: test.c
 *   Created on: 2011-01-183
 *       Author: mm.li
 * Descriptions: 
 * This file show how to use libid3tag - ID3 tag manipulation library
 *
 * To compile libid3tag:
 * 			./configure
 * 			make
 *
 * To compile test:
 * gcc test.c .libs/libid3tag.a -o test -lz -Wall
 *	
 * Change log  :
 * Date             Mender               Cause
*******************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "id3tag.h"

int main(int argc,char* argv[])
{
    struct id3_file* file;
    struct id3_tag* tag;
    struct id3_frame* frame;
    int groupid;
    int i,iRet = 0;
    if (argc < 2) {
        printf("Please input filename!\n");
        printf("./test xxx.mp3\n");
        iRet = -1;
    }
    else {
	    file = id3_file_open(argv[1],0);
	    if (file) {
	        tag = id3_file_tag(file);
	        if (tag) {
	            for (i = 0;i < tag->nframes;i++) {
	                char const* desc;
	                frame = tag->frames[i];
	                desc = frame->description;
	                groupid = frame->group_id;
	                union id3_field const *field ;
	                if (strcmp(frame->id,ID3_FRAME_COMMENT) == 0) {
	                    field = &frame->fields[3];
	                } 
	                else {
	                    field = &frame->fields[1];
	                }
	                printf("%s (%s): ",frame->id,desc);
	
	                switch (field->type) {
	                case ID3_FIELD_TYPE_INT32:
	
	                case ID3_FIELD_TYPE_INT24:
	
	                case ID3_FIELD_TYPE_INT16:
	                case ID3_FIELD_TYPE_INT8: {
	                    signed long vlue = id3_field_getint(field);
	                    printf("%ld ",vlue);
	                    break;
	                }
	                case ID3_FIELD_TYPE_TEXTENCODING: {
	                    enum id3_field_textencoding encoding = id3_field_gettextencoding(field);
	                    switch (encoding) {
	                    case ID3_FIELD_TEXTENCODING_ISO_8859_1:
	                        printf("ISO_8859_1 ");
	                        break;
	                    case ID3_FIELD_TEXTENCODING_UTF_16  :
	                        printf("UTF_16 ");
	                        break;
	                    case ID3_FIELD_TEXTENCODING_UTF_16BE :
	                        printf("UTF_16BE ");
	                        break;
	                    case ID3_FIELD_TEXTENCODING_UTF_8  :
	                        printf("UTF_UTF_8 ");
	                        break;
	                    }
	                    break;
	                }
	
	
	                case ID3_FIELD_TYPE_LATIN1: {
	                    id3_latin1_t const* latin = id3_field_getlatin1(field);
	                    printf("%s ",latin);
	                    break;
	                }
	                case ID3_FIELD_TYPE_LATIN1FULL: {
	                    id3_latin1_t const* latin = id3_field_getfulllatin1(field);
	                    printf("%s ",latin);
	                    break;
	                }
	                case ID3_FIELD_TYPE_LATIN1LIST: {
	                    id3_latin1_t const * latin = id3_field_getlatin1s(field,0);
	                    printf("%s ",latin);
	                    break;
	                }
	
	                case ID3_FIELD_TYPE_STRING: {
	                    id3_latin1_t * latin;
	                    id3_ucs4_t const* string = id3_field_getstring(field);
	                    if (string != NULL) {
	                        latin  = id3_ucs4_latin1duplicate(string);
	                    }
	                    printf("%s ",latin);
	                    break;
	                }
	                case ID3_FIELD_TYPE_STRINGFULL: {
	                    id3_latin1_t* latin;
	                    id3_ucs4_t const* string = id3_field_getfullstring(field);
	                    if (string != NULL) {
	                        latin  = id3_ucs4_latin1duplicate(string);
	                    }
	                    printf("%s ",latin);
	                    break;
	                }
	                case ID3_FIELD_TYPE_STRINGLIST: {
	                    id3_latin1_t* latin;
	                    id3_ucs4_t const* string = id3_field_getstrings(field,0);
	                    latin = id3_ucs4_latin1duplicate(string);
	                    printf("%s ",latin);
	                    break;
	                }
	                case ID3_FIELD_TYPE_LANGUAGE:
	
	                case ID3_FIELD_TYPE_FRAMEID:
	
	                case ID3_FIELD_TYPE_DATE: {
	                    char const* string = id3_field_getframeid(field);
	                    printf("%s ",string);
	                    break;
	                }
	                case ID3_FIELD_TYPE_INT32PLUS:
	                case ID3_FIELD_TYPE_BINARYDATA: {
	                    id3_length_t data_len,i;
	                    id3_byte_t const* data =
	                        id3_field_getbinarydata(field,&data_len);
	                    for (i=0;i<data_len;i++) {
	                        printf("%c",data[i]);
	                    }
	                    printf(" ");
	                    break;
	                }
	                }
	                printf("\n");
	            }
	        } 
	        else {
	            printf("get tag from %s failed!\n",argv[1]);
	            iRet = -2;
	        }
	        if (id3_file_close(file) == -1) {
	            printf("close %s failed!\n",argv[1]);
	            iRet = -3;
	        }
	    } 
	    else {
	        printf("open %s failed!\n",argv[1]);
	        iRet = -4;
	    }
  	}
    return iRet;
}
