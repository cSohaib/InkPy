#pragma once
#include <stdio.h>
#include <stdbool.h>
typedef struct { FILE *file;char name[64],attributes[2048],text[1024];bool closing,empty; } ink_xml;
/* 1=tag, 2=text, 0=end, -1=malformed/oversized token. No DOM or external entities. */
int ink_xml_next(ink_xml *x);
void ink_xml_attr(const ink_xml *x,const char *key,char *value,size_t capacity);
void ink_xml_entities(char *text);
