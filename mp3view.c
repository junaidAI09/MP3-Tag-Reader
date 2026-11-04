#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "types.h"

static unsigned int synchsafe_size(const unsigned char b[4]){
    return ((unsigned int)(b[0] & 0x7F) << 21) |
           ((unsigned int)(b[1] & 0x7F) << 14) |
           ((unsigned int)(b[2] & 0x7F) << 7) |
           (unsigned int)(b[3] & 0x7F);
}

static unsigned int be32(const unsigned char b[4]){
    return ((unsigned int)b[0] << 24) | ((unsigned int)b[1] << 16) |
           ((unsigned int)b[2] << 8) | (unsigned int)b[3];
}


static void view_id3v1(FILE *fp){
    unsigned char tag[128];
    if (fseek(fp, -128L, SEEK_END) != 0 ||
        fread(tag, 1, sizeof(tag), fp) != sizeof(tag) ||
        memcmp(tag, "TAG", 3) != 0)
    {
        printf("ERROR: ID3 tag not found\n");
        return;
    }

    printf("\nID3 version: ID3v1");
    if (tag[125] == 0 && tag[126] != 0) printf(".1");
    printf("\n");
    printf("%-15s: %.30s\n", "Title", tag + 3);
    printf("%-15s: %.30s\n", "Artist", tag + 33);
    printf("%-15s: %.30s\n", "Album", tag + 63);
    printf("%-15s: %.4s\n", "Year", tag + 93);
    if (tag[125] == 0 && tag[126] != 0)
        printf("%-15s: %.28s\n", "Comment", tag + 97);
    else
        printf("%-15s: %.30s\n", "Comment", tag + 97);
    printf("%-15s: %u\n", "Genre code", tag[127]);
}

typedef struct{
    char title[256];
    char artist[256];
    char album[256];
    char year[32];
    char music[128];
    char comment[512];
} MP3Tag;

static void copy_field(char *dest, size_t dest_size,
                       const unsigned char *data, unsigned int size){
    unsigned int i = 0;
    unsigned int j = 0;

    if (size > 0 && data[0] == 0)
    {
        i = 1;
    }

    while (i < size && data[i] != '\0' &&
           data[i] != '\n' && data[i] != '\r' &&
           j < dest_size - 1)
    {
        dest[j] = (char)data[i];
        i++;
        j++;
    }

    dest[j] = '\0';
}

void view_mp3(char *filename){
    FILE *fp = fopen(filename, "rb");
    unsigned char header[10];
    unsigned char *tag_data = NULL;
    unsigned int tag_size, pos = 0;
    int major, minor;
    MP3Tag tag={0};

    if (!fp)
    {
        printf("ERROR: Unable to open file: %s\n", filename);
        return;
    }

    if (fread(header, 1, 3, fp) != 3)
    {
        printf("ERROR: Invalid or empty file\n");
        fclose(fp);
        return;
    }

    if (memcmp(header, "ID3", 3) != 0)
    {
        view_id3v1(fp);
        fclose(fp);
        return;
    }

    if (fseek(fp, 0, SEEK_SET) != 0 || fread(header, 1, 10, fp) != 10)
    {
        printf("ERROR: Invalid ID3 header\n");
        fclose(fp);
        return;
    }

    major = header[3];
    minor = header[4];
    if (major != 2 && major != 3)
    {
        printf("ERROR: Unsupported ID3 version 2.%d.%d\n", major, minor);
        fclose(fp);
        return;
    }

    tag_size = synchsafe_size(header + 6);
    if (tag_size > MAX_FRAME_DATA)
    {
        printf("ERROR: ID3 tag is too large or invalid\n");
        fclose(fp);
        return;
    }

    tag_data = (unsigned char *)malloc(tag_size ? tag_size : 1);
    if (!tag_data)
    {
        printf("ERROR: Memory allocation failed\n");
        fclose(fp);
        return;
    }
    if (fread(tag_data, 1, tag_size, fp) != tag_size)
    {
        printf("ERROR: Incomplete ID3 tag\n");
        free(tag_data);
        fclose(fp);
        return;
    }

  printf("\nID3 version: 2.%d.%d\n", major, minor);

while (pos < tag_size)
{
    char id[5] = {0};
    unsigned int frame_size, frame_header;
    const unsigned char *payload;

    if (tag_data[pos] == 0)
    {
        break;
    }

    if (major == 2)
    {
        if (tag_size - pos < 6)
        {
            break;
        }

        memcpy(id, tag_data + pos, 3);

        frame_size = ((unsigned int)tag_data[pos + 3] << 16) |
                     ((unsigned int)tag_data[pos + 4] << 8) |
                     tag_data[pos + 5];

        frame_header = 6;
    }
    else
    {
        if (tag_size - pos < 10)
        {
            break;
        }

        memcpy(id, tag_data + pos, 4);
        frame_size = be32(tag_data + pos + 4);
        frame_header = 10;
    }

    if (frame_size > tag_size - pos - frame_header)
    {
        break;
    }

    payload = tag_data + pos + frame_header;

    if (!strcmp(id, "TIT2") || !strcmp(id, "TT2"))
    {
        copy_field(tag.title, sizeof(tag.title), payload, frame_size);
    }
    else if (!strcmp(id, "TPE1") || !strcmp(id, "TP1"))
    {
        copy_field(tag.artist, sizeof(tag.artist), payload, frame_size);
    }
    else if (!strcmp(id, "TALB") || !strcmp(id, "TAL"))
    {
        copy_field(tag.album, sizeof(tag.album), payload, frame_size);
    }
    else if (!strcmp(id, "TYER") || !strcmp(id, "TYE") ||
             !strcmp(id, "TDRC"))
    {
        copy_field(tag.year, sizeof(tag.year), payload, frame_size);
    }
    else if (!strcmp(id, "TCON") || !strcmp(id, "TCO"))
    {
        copy_field(tag.music, sizeof(tag.music), payload, frame_size);
    }
    else if (!strcmp(id, "COMM") || !strcmp(id, "COM")){
        unsigned int skip = 4;

        if (frame_size >= 4) {
            if (payload[0] == 1 || payload[0] == 2)
            {
                while (skip + 1 < frame_size &&
                       !(payload[skip] == 0 &&
                         payload[skip + 1] == 0))
                {
                    skip += 2;
                }

                if (skip + 1 < frame_size)
                {
                    skip += 2;
                }
            }
            else
            {
                while (skip < frame_size && payload[skip] != 0)
                {
                    skip++;
                }

                if (skip < frame_size)
                {
                    skip++;
                }
            }

            if (skip <= frame_size)
            {
                copy_field(tag.comment, sizeof(tag.comment),
                           payload + skip, frame_size - skip);
            }
        }
    }

    pos += frame_header + frame_size;
}

printf("--------------------------------------------------\n");
printf("MP3 TAG READER AND EDITOR FOR ID3v2\n");
printf("--------------------------------------------------\n");

printf("%-15s : %s\n", "TITLE", tag.title);
printf("%-15s : %s\n", "ARTIST", tag.artist);
printf("%-15s : %s\n", "ALBUM", tag.album);
printf("%-15s : %s\n", "YEAR", tag.year);
printf("%-15s : %s\n", "MUSIC", tag.music);
printf("%-15s : %s\n", "COMMENT", tag.comment);

printf("--------------------------------------------------\n");
printf("DETAILS DISPLAYED SUCCESSFULLY\n");
printf("--------------------------------------------------\n");

free(tag_data);
fclose(fp);

}