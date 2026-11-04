#include <stdio.h>
#include <string.h>
#include "main.h"

void show_help(void){

    printf("\n-------------------------HELP MENU-------------------------\n\n");

    printf("1. -v -> to view mp3 file contents\n");
    printf("2. -e -> to edit mp3 file contents\n");
    printf("     2.1. -t -> to edit song title\n");
    printf("     2.2. -a -> to edit artist name\n");
    printf("     2.3. -A -> to edit album name\n");
    printf("     2.4. -y -> to edit year\n");
    printf("     2.5. -m -> to edit content\n");
    printf("     2.6. -c -> to edit comment\n");

    printf("\n-----------------------------------------------------------\n");
}

int main(int argc, char *argv[]){
    if (argc == 1){
        printf("\n---------------------------------------------------------------\n\n");
        printf("ERROR: ./a.out : INVALID ARGUMENTS\n");
        printf("USAGE :\n");
        printf("To view please pass like: ./a.out -v filename.mp3\n");
        printf("To edit please pass like: ./a.out -e -t/-a/-A/-m/-y/-c changing_text/tags filename.mp3\n");
        printf("To get help please pass like: ./a.out --help\n\n");
        printf("---------------------------------------------------------------\n");
        return 1;
    }
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 ||
                      strcmp(argv[1], "--help") == 0)){
        show_help();
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "-v") == 0){
        view_mp3(argv[2]);
        return 0;
    }

    if (argc == 5 && strcmp(argv[1], "-e") == 0){
        edit_mp3(argv[2], argv[3], argv[4]);
        return 0;
    }

    printf("ERROR: Invalid arguments.\n");
    show_help();
    return 1;
}
