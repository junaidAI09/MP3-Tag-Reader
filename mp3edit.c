#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "common.h"
#include "types.h"

static unsigned int read_synchsafe(const unsigned char *b){
    return ((unsigned int)(b[0] & 0x7F) << 21) |
           ((unsigned int)(b[1] & 0x7F) << 14) |
           ((unsigned int)(b[2] & 0x7F) << 7) |
           (unsigned int)(b[3] & 0x7F);
}

static void write_synchsafe(unsigned char *b, unsigned int n){
    b[0] = (unsigned char)((n >> 21) & 0x7F);
    b[1] = (unsigned char)((n >> 14) & 0x7F);
    b[2] = (unsigned char)((n >> 7) & 0x7F);
    b[3] = (unsigned char)(n & 0x7F);
}

static const char *frame_for_option(const char *option){
    if (!strcmp(option, "-t")) return "TIT2";
    if (!strcmp(option, "-a")) return "TPE1";
    if (!strcmp(option, "-A")) return "TALB";
    if (!strcmp(option, "-y")) return "TYER";
    if (!strcmp(option, "-m")) return "TCON";
    if (!strcmp(option, "-c")) return "COMM";
    return NULL;
}

static int copy_file(FILE *in, FILE *out){
    unsigned char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        if (fwrite(buf, 1, n, out) != n) return 0;
    return !ferror(in);
}

/*
 * ID3v2.3 editor: rebuilds the tag, replacing the selected text frame.
 * It deliberately refuses unknown versions rather than corrupting the MP3.
 */
static void edit_id3v23(const char *filename, const char *target,
                        const char *value){
    FILE *in = fopen(filename, "rb");
    FILE *out = NULL;
    unsigned char header[10];
    unsigned char *old_tag = NULL, *new_tag = NULL;
    unsigned int old_size, pos = 0, new_size = 0, target_len;
    int found = 0;
    char tempname[1024];

    if (!in)
    {
        printf("ERROR: Unable to open file: %s\n", filename);
        return;
    }
    if (fread(header, 1, 10, in) != 10 || memcmp(header, "ID3", 3) != 0 ||
        header[3] != 3)
    {
        printf("ERROR: Editing currently requires an ID3v2.3 file.\n");
        fclose(in);
        return;
    }

    old_size = read_synchsafe(header + 6);
    if (old_size > MAX_FRAME_DATA) {
        printf("ERROR: Invalid or oversized ID3 tag\n");
        fclose(in);
        return;
    }

    old_tag = malloc(old_size ? old_size : 1);
    if (!old_tag || fread(old_tag, 1, old_size, in) != old_size) {
        printf("ERROR: Unable to read ID3 tag\n");
        free(old_tag);
        fclose(in);
        return;
    }

    target_len=(unsigned int)strlen(value) + 1;
    if(strcmp(target, "COMM")==0){
    target_len = (unsigned int)strlen(value) + 5;
    
    }
    /* First pass calculates required rebuilt tag size. */
    pos = 0;
    while (pos + 10 <= old_size && old_tag[pos] != 0) {
        unsigned int n = ((unsigned int)old_tag[pos+4] << 24) |
                         ((unsigned int)old_tag[pos+5] << 16) |
                         ((unsigned int)old_tag[pos+6] << 8) | old_tag[pos+7];
        if (n > old_size - pos - 10) break;
        if (!memcmp(old_tag + pos, target, 4)) found = 1;
        else new_size += 10 + n;
        pos += 10 + n;
    }
    if (found) new_size += 10 + target_len;
    else new_size += 10 + target_len;
    if (new_size > MAX_FRAME_DATA){
        printf("ERROR: New tag would be too large\n");
        free(old_tag);
        fclose(in);
        return;
    }

    new_tag = malloc(new_size ? new_size : 1);
    if (!new_tag){
        printf("ERROR: Memory allocation failed\n");
        free(old_tag);
        fclose(in);
        return;
    }

    /* Copy all frames except the selected one. */
    pos = 0;
    while (pos + 10 <= old_size && old_tag[pos] != 0) {
        unsigned int n = ((unsigned int)old_tag[pos+4] << 24) |
                         ((unsigned int)old_tag[pos+5] << 16) |
                         ((unsigned int)old_tag[pos+6] << 8) | old_tag[pos+7];
        if (n > old_size - pos - 10) break;
        if (memcmp(old_tag + pos, target, 4) != 0)
        {
            memcpy(new_tag + (new_size - new_size), old_tag, 0); /* no-op, keeps beginner-friendly flow */
            //Write below with separate output position. 
        }
        pos += 10 + n;
    }

    {
        unsigned int src = 0, dst = 0;
        while (src + 10 <= old_size && old_tag[src] != 0){
            unsigned int n = ((unsigned int)old_tag[src+4] << 24) |
                             ((unsigned int)old_tag[src+5] << 16) |
                             ((unsigned int)old_tag[src+6] << 8) | old_tag[src+7];
            if (n > old_size - src - 10) break;
            if (memcmp(old_tag + src, target, 4) != 0)
            {
                memcpy(new_tag + dst, old_tag + src, 10 + n);
                dst += 10 + n;
            }
            src += 10 + n;
        }

        
       /* Append the replacement frame */
memcpy(new_tag + dst, target, 4);

if (strcmp(target, "COMM") == 0){
    unsigned int comment_size = (unsigned int)strlen(value) + 5;

    /* Frame size */
    new_tag[dst + 4] = (unsigned char)(comment_size >> 24);
    new_tag[dst + 5] = (unsigned char)(comment_size >> 16);
    new_tag[dst + 6] = (unsigned char)(comment_size >> 8);
    new_tag[dst + 7] = (unsigned char)comment_size;

    /* Frame flags */
    new_tag[dst + 8] = 0;
    new_tag[dst + 9] = 0;

    /* Encoding: ISO-8859-1 */
    new_tag[dst + 10] = 0;

    /* Language: English */
    new_tag[dst + 11] = 'e';
    new_tag[dst + 12] = 'n';
    new_tag[dst + 13] = 'g';

    /* Empty description followed by comment text */
    new_tag[dst + 14] = 0;
    memcpy(new_tag + dst + 15, value, strlen(value));

    dst += 10 + comment_size;
}
else
{
    /* Ordinary text frame */
    new_tag[dst + 4] = 0;
    new_tag[dst + 5] = 0;
    new_tag[dst + 6] = 0;
    new_tag[dst + 7] = (unsigned char)target_len;
    new_tag[dst + 8] = 0;
    new_tag[dst + 9] = 0;
    new_tag[dst + 10] = 0;

    memcpy(new_tag + dst + 11, value, target_len - 1);

    dst += 10 + target_len;
}
new_size = dst;

    }

    snprintf(tempname, sizeof(tempname), "%s.tmp", filename);
    out = fopen(tempname, "wb");
    if (!out)
    {
        printf("ERROR: Cannot create temporary file\n");
        free(old_tag); free(new_tag); fclose(in);
        return;
    }

    memcpy(header + 6, "\0\0\0\0", 4);
    write_synchsafe(header + 6, new_size);
    if (fwrite(header, 1, 10, out) != 10 ||
        fwrite(new_tag, 1, new_size, out) != new_size ||
        !copy_file(in, out))
    {
        printf("ERROR: Failed while writing updated MP3\n");
        fclose(in); fclose(out); remove(tempname);
        free(old_tag); free(new_tag);
        return;
    }

    fclose(in);
    if (fclose(out) != 0 || remove(filename) != 0 || rename(tempname, filename) != 0)
    {
        printf("ERROR: Could not replace original file with updated file\n");
        free(old_tag); free(new_tag);
        return;
    }

    printf("Tag updated successfully.\n");
    free(old_tag);
    free(new_tag);
}

static void edit_id3v1(const char *filename, const char *option, const char *value){
    FILE *fp = fopen(filename, "r+b");
    unsigned char tag[128];
    long end;
    int offset = -1, length = 0;

    if (!fp)
    {
        printf("ERROR: Unable to open file: %s\n", filename);
        return;
    }
    if (fseek(fp, 0, SEEK_END) != 0 || (end = ftell(fp)) < 128 ||
        fseek(fp, -128L, SEEK_END) != 0 ||
        fread(tag, 1, 128, fp) != 128 || memcmp(tag, "TAG", 3) != 0)
    {
        printf("ERROR: No ID3v1 tag found. ID3v1 editing is not available for this file.\n");
        fclose(fp);
        return;
    }

    if (!strcmp(option, "-t")) { offset = 3; length = 30; }
    else if (!strcmp(option, "-a")) { offset = 33; length = 30; }
    else if (!strcmp(option, "-A")) { offset = 63; length = 30; }
    else if (!strcmp(option, "-y")) { offset = 93; length = 4; }
    else if (!strcmp(option, "-c")) { offset = 97; length = (tag[125] == 0 && tag[126] != 0) ? 28 : 30; }
    else
    {
        printf("ERROR: ID3v1 does not support this edit option.\n");
        fclose(fp);
        return;
    }

    memset(tag + offset, 0, (size_t)length);
    if (strlen(value) > (size_t)length)
        memcpy(tag + offset, value, (size_t)length);
    else
        memcpy(tag + offset, value, strlen(value));

    if (fseek(fp, -128L, SEEK_END) != 0 ||
        fwrite(tag, 1, 128, fp) != 128)
        printf("ERROR: Failed to update ID3v1 tag\n");
    else
        printf("Tag updated successfully.\n");

    fclose(fp);
}

void edit_mp3(char *option, char *value, char *filename){
    const char *target = frame_for_option(option);
    FILE *fp;
    unsigned char h[4];

    if (!target)
    {
        printf("ERROR: Unknown edit option: %s\n", option);
        return;
    }

    fp = fopen(filename, "rb");
    if (!fp)
    {
        printf("ERROR: Unable to open file: %s\n", filename);
        return;
    }

    if (fread(h, 1, 4, fp) == 4 && memcmp(h, "ID3", 3) == 0)
    {
        fclose(fp);
        edit_id3v23(filename, target, value);
    }
    else
    {
        fclose(fp);
        edit_id3v1(filename, option, value);
    }
}
