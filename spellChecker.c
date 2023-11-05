# include <stdio.h>     // standard C library
# include <stdlib.h>    // for dynamic memory allocation
# include <stdbool.h>   // for bool datatype
# include <stdint.h>    // for uint32 types etc
# include <string.h>    // for string manipulation
# include <ctype.h>     // for testing character types
# include <math.h>      // for some math functions
# include <pthread.h>   // for multithreading
# include <windows.h>   // for computational analysis and comparision analysis
# include "spell.h"     // our header file with bloom filter, trie and lru cache etc functionality

int main() {
    TRIE_NODE* root=createNode(); // creating Trie node
    bool* filter = calloc(FILTER_SIZE, sizeof(bool)); // creating bloom filter bit array
    loadDictionary(dict_file, filter, root); // populating both trie and bloom filter with dictionary words

    int ch; // choice mode of user 
    char str[100]; // string for storing users sentence input 
    char word[MAX_LENGTH + 1]; // string for storing users word input
    char suggestions[MAX_SUGGESTIONS][MAX_LENGTH + 1]; // array of strings to store all the suggestions for incorrect words
    double accuracyFilter=0; // storing accuracy of bloom filter
    LARGE_INTEGER start, end, freq; // variables to accurate time measurements of functions
    double time_used_trie; // to store time used by trie for spell checking
    double time_used_filter; // to store time used by filter for spell checking 
    double faster; // to store difference of the time taken by filter and trie
    int accurate_comp=0; // number of accuarte spell checking instances of filter 
    int num_comp=0; // total instances of spell checking
    bool checkTrie; // storing if trie contains a word of not
    bool checkFilter; // storing if a filter contains a word or not
    FILE *file;

    struct LRUCache* cache=createLRUCache(MAX_SUGGESTIONS); // first LRU Cache to store and update the suggestions given by suggest algorithm

    while (1){
        printf(COLOR_YELLOW "Select the mode you want to enter:-\n1. Spell checking and autocorrect\n2. Analysis mode\n3. Optimisation mode\n4. Quit\n" COLOR_RESET);
        scanf("%d", &ch);
        getchar();
        if (ch==1){
            while (1){
                printf(COLOR_MAGENTA "Enter a sentence or type exit to leave:\n" COLOR_RESET);
                fgets(str, sizeof(str), stdin);
                str[strcspn(str, "\n")] = 0;
                if (!strcmp(str, "exit")) break;
                printf(COLOR_MAGENTA "The sentence after checking is: \n" COLOR_RESET);
                int index = 0, incrt_words = 0;

                char display_suggest[MAX_INCRT_WORDS][MAX_LENGTH];

                for (int i = 0, l = strlen(str); i < l; i++){
                    char c = str[i];
                    if (isalpha(c)){
                        word[index] = tolower(c);
                        index++;
                    }
                    else if (isdigit(c)){
                        printf(COLOR_MAGENTA "\n Error no numbers...!" COLOR_RESET);
                    }
                    else if (index > 0){
                        word[index] = '\0';
                        index = 0;
                        if (!checkWord(root, filter, word)){
                            printf(COLOR_RED "%s " COLOR_RESET, word);
                            strcpy(display_suggest[incrt_words], word);
                            incrt_words++;
                            printf("%s", " ");
                        }
                        else {
                            printf(COLOR_GREEN "%s " COLOR_RESET, word);
                            printf("%s", " ");
                        }
                    }
                }
                if (index > 0){
                    word[index] = '\0';
                    if (!checkWord(root, filter, word)){
                        printf(COLOR_RED "%s " COLOR_RESET, word);
                        strcpy(display_suggest[incrt_words], word);
                        incrt_words++;
                    }
                    else printf(COLOR_GREEN "%s " COLOR_RESET, word);
                }
                if (incrt_words > 0){
                    printf(COLOR_MAGENTA "\nSuggestions for incorrect words:\n" COLOR_RESET);
                    for (int i = 0; i < incrt_words; i++){
                        printf(COLOR_RED "%d. %s -> " COLOR_RESET, i + 1, display_suggest[i]);
                        suggest(display_suggest[i], cache);
                    }
                }
                printf("\n");
            }
        }
        else if (ch==2){
            accurate_comp=0;
            num_comp=0;
            time_used_trie=0;
            time_used_filter=0;
            file = fopen(input_file, "r");
            while (fscanf(file, "%s", word) == 1){
                removeNonAlphabetical(word);

                QueryPerformanceCounter(&start); 
                checkTrie=searchTrie(root, word); 
                QueryPerformanceCounter(&end);
                QueryPerformanceFrequency(&freq);
                time_used_trie+= (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
                
                QueryPerformanceCounter(&start); 
                checkFilter=searchFilter(filter, word);
                QueryPerformanceCounter(&end);
                QueryPerformanceFrequency(&freq);
                time_used_filter+= (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;

                if (!checkTrie){
                    num_comp++;
                    if (!checkFilter) accurate_comp++;
                }
            }
            fclose(file);
            accuracyFilter=accurate_comp*100.0/num_comp;
            printf(COLOR_CYAN "\nThe accuracy of Tries is %f percent\n", num_comp*100.0/num_comp);
            printf("The accuracy of Bloom Filter %f percent\n\n" COLOR_RESET, accuracyFilter);
            faster=fabs(time_used_trie-time_used_filter)*100/time_used_trie;
            if (time_used_trie>time_used_filter) printf(COLOR_GREEN "Bloom Filter is %f percent faster\n" COLOR_RESET, faster);
            else printf(COLOR_RED "Bloom Filter is %f percent slower\n" COLOR_RESET, faster);
            printf("\n");
        }
        else if (ch==3){
            char** suggestions = (char**) malloc(MAX_SUGGEST_COUNT * sizeof(char *));
            for(int i=0; i<MAX_SUGGEST_COUNT; i++) suggestions[i] = (char*) malloc(MAX_LENGTH * sizeof(char));
            printf(COLOR_MAGENTA "The incorrect words in the file and their suggestions are:-\n" COLOR_RESET);
            pthread_t threads[N_THREADS];
            THREAD_INP thread_inp[N_THREADS];
            for(int i=0; i<N_THREADS; i++) {
                thread_inp[i].thread_num = i;
                thread_inp[i].cache = cache;
                thread_inp[i].filter = filter;
                thread_inp[i].suggestions = suggestions;
                pthread_create(&threads[i], NULL, processFile, &thread_inp[i]);
            }
            for(int i=0; i<N_THREADS; i++) pthread_join(threads[i], NULL);
            printf(COLOR_GREEN "%d Incorrect words found in text.\n" COLOR_RESET, suggest_count);
            for(int i=0; i<MAX_SUGGEST_COUNT; i++) {
                printf(COLOR_RED "%s-> " COLOR_RESET, suggestions[i]);
                suggest(suggestions[i], cache);
                printf("\n");
            }
            for(int i=0; i<MAX_SUGGEST_COUNT; i++) free(suggestions[i]);
            free(suggestions);
        }
        else if (ch==4) break;
        else printf("Invalid value!\n");
        printf("\n");
    }
    
    return 0;
}