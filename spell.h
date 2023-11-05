int suggest_count = 0;

// file name
#define DICT_FILE "dictionary.txt"
#define PARA_FILE "input.txt"

// multithreading parameters
#define N_THREADS 10

#define MAX_SUGGEST_COUNT 500

#define FILTER_SIZE 3600000 // sizeof bit array used for bloom filter
#define K 7 // number of hash functions

#define MAX_LENGTH 50 // maximum length of a word in a dictionary
#define MAX_SUGGESTIONS 10 // maximum suggestions to feed for incorrect words
#define MAX_INCRT_WORDS 1000 // maximum incorrect words while spell checking in terminal inputs
#define CACHE_SIZE 100 // cache size for spell checking
#define N 26    // number of distinct characters in English Alphabets

// Dummy value for treating Infinity
#define INT_MAX 2147483647

// Defining colors for formatting
#define COLOR_BLUE "\x1b[34m"
#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_RESET "\x1b[0m"

// Function to return greater of 2 integers
int greater(int x, int y){
    return x>y?x:y;
}

// Function to return smaller of 2 integers
int smaller(int x, int y){
    return x>y?y:x;
}

// Function to return smallest of 3 integers
int smallest(int a, int b, int c) {
    return a < b ? (a < c ? a : c) : (b < c ? b : c);
}

// Function to create DJB_2 Hash of a string
uint32_t djb2(const char* string) {     
    uint32_t hash = 5381;
    int c;
    while ((c= *string++)) hash = (hash << 5) + hash + c;   
    return hash % FILTER_SIZE;
}

// Function to calculate Jenkin's Hash of a string 
uint32_t jenkin(const char* str) {      
    uint32_t hash = 0;
    while(*str) {
        hash += *str++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash % FILTER_SIZE;
}

// Function to insert a word in Bloom Filter
void insertFilter(bool* filter, char* word){   
    for(int i=0; i<K; i++) filter[(djb2(word)*i + jenkin(word))%FILTER_SIZE] = 1;
}

// Function to search a word in Bloom Filter
bool searchFilter(bool* filter, char* word){     
    for(int i=0; i<K; i++) if(!filter[(djb2(word)*i + jenkin(word))%FILTER_SIZE]) return false;
    return true;
}

// Structure of a Trie node
typedef struct node { 
    struct node *child[N];
    bool isEOW;
} TRIE_NODE;

// Function to create a Trie node
TRIE_NODE* createNode() {
    TRIE_NODE* pNode = (TRIE_NODE*)malloc(sizeof(TRIE_NODE));
    pNode->isEOW = false;
    for(int i=0; i<N; i++) pNode->child[i] = NULL;
    return pNode;
}

// Function to insert a word in a Trie
void insertTrie(TRIE_NODE* root, char* word){
    TRIE_NODE* pCrawl = root;
    int len = strlen(word), idx;
    for(int i=0; i<len; i++) {
        idx = word[i] - 'a';
        if(!pCrawl->child[idx]) pCrawl->child[idx] = createNode();
        pCrawl = pCrawl->child[idx];
    }
    pCrawl->isEOW = true;
}

// Function to search a word in a Trie
bool searchTrie(TRIE_NODE* root, char* word){
    TRIE_NODE* pCrawl = root;
    int len = strlen(word), idx;
    for(int i=0; i<len; i++) {
        idx = word[i] - 'a';
        if(!pCrawl->child[idx]) return false;
        pCrawl = pCrawl->child[idx];
    }
    return (pCrawl->isEOW);
}

// Function to display all the words in a Trie
void display(TRIE_NODE* root, char* word, int level){
    if(root->isEOW) word[level] = '\0';
    for(int i=0; i<N; i++){
        if(root->child[i]!=NULL) {
            word[level] = i + 'a';
            display(root->child[i], word, level+1);
        }
    }
}

void* filterThread(void* filter) {
    char word[50];      // maximum word lenght
    FILE* dict_ptr = fopen(DICT_FILE, "r");
    if(dict_ptr == NULL) {
        perror(COLOR_RED "Error loading Dictionary\n" COLOR_RESET);
        exit(1);
    }
    while(fscanf(dict_ptr, "%s", word) != EOF) insertFilter((bool*) filter, word);
    printf(COLOR_GREEN "Dictionary loaded on filter successfully\n" COLOR_RESET);
    fclose(dict_ptr);
    return NULL;
}

void* trieThread(void* root) {
    char word[50];      // maximum word lenght
    FILE* dict_ptr = fopen(DICT_FILE, "r");
    if(dict_ptr == NULL) {
        perror(COLOR_RED "Error loading Dictionary or wrong path/file for dictionary\n" COLOR_RESET);
        exit(1);
    }
    while(fscanf(dict_ptr, "%s", word) != EOF) insertTrie((TRIE_NODE*) root, word);
    printf(COLOR_GREEN "Dictionary loaded on trie successfully\n" COLOR_RESET);
    fclose(dict_ptr);
    return NULL;
}

// populating bloom filter and trie with Dictionary words
void loadDictionary(bool* filter, TRIE_NODE* root) {
    pthread_t filter_thread, trie_thread;
    if(pthread_create(&filter_thread, NULL, filterThread, filter) & pthread_create(&trie_thread, NULL, trieThread, root)) printf(COLOR_RED "Error occured in threading\n" COLOR_RESET);
    if(pthread_join(filter_thread, NULL) & pthread_join(trie_thread, NULL)) printf(COLOR_RED "Error occured in threading\n" COLOR_RESET);
}


// Levenshtein edit distance between two words s and t
int levenshteinDistance(const char *s, const char *t){
    int n = strlen(s);
    int m = strlen(t);

    if (n == 0)
        return m;
    if (m == 0)
        return n;

    int dp[MAX_LENGTH + 1][MAX_LENGTH + 1];
    for (int i = 0; i <= n; i++)
        dp[i][0] = i;
    for (int j = 0; j <= m; j++)
        dp[0][j] = j;

    for (int i = 1; i <= n; i++)
    {
        for (int j = 1; j <= m; j++)
        {
            int cost = (s[i - 1] == t[j - 1]) ? 0 : 1;
            dp[i][j] = smallest(dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost);
        }
    }
    return dp[n][m];
}

// Jaro Winkler distance between two words s1 and s2
double jaroWinklerDistance(char* s1, char* s2){
    if (s1 == s2)
        return 1.0;

    int len1 = strlen(s1), len2 = strlen(s2);
    int max_dist = floor(greater(len1, len2) / 2) - 1;
    int match = 0;
 
    int* hash_s1 = calloc(len1, sizeof(int));
    int* hash_s2 = calloc(len2, sizeof(int));
 
    for(int i=0; i<len1; i++) {
        for(int j=greater(0, i - max_dist);
             j<smaller(len2, i + max_dist + 1); j++)
            if (s1[i] == s2[j] && hash_s2[j] == 0) {
                hash_s1[i] = 1;
                hash_s2[j] = 1;
                match++;
                break;
            }
    }
    if (match == 0) return 0.0;
    
    double t = 0; 
    int point = 0;

    for(int i=0; i < len1; i++)
        if(hash_s1[i]) {
            while(hash_s2[point] == 0) point++;
            if(s1[i] != s2[point++]) t++;
        }

    t /= 2;
    return (((double)match) / ((double)len1) + ((double)match) / ((double)len2) + ((double)match - t) / ((double)match)) / 3.0;
}

// Structure of a queue node of LRU Cache using doubly linked lists
struct LRUCacheQueueNode{
    char val[MAX_LENGTH];
    struct LRUCacheQueueNode* prev;
    struct LRUCacheQueueNode* forw;
};

// Structure of a hash map node of LRU Cache
struct LRUCacheMapNode{
    struct LRUCacheQueueNode* nd;
    struct LRUCacheMapNode* prev;
    struct LRUCacheMapNode* forw;
};

// Create a queue node for LRU Cache
struct LRUCacheQueueNode* createQueueNode(char* string) {
    struct LRUCacheQueueNode* ans = (struct LRUCacheQueueNode*)malloc(sizeof(struct LRUCacheQueueNode));
    strcpy(ans->val, string);
    ans->forw = NULL;
    ans->prev = NULL;
    return ans;
}

// Create a hash map node for LRU Cache
struct LRUCacheMapNode* createHashNode(char* string){
    struct LRUCacheMapNode* ans=(struct LRUCacheMapNode*)malloc(sizeof(struct LRUCacheMapNode));
    ans->nd=createQueueNode(string);
    ans->prev=NULL;
    ans->forw=NULL;
    return ans;
}

// Structure of a LRU Cache object including hash map and queue
struct LRUCache{
    struct LRUCacheQueueNode *head, *tail;
    struct LRUCacheMapNode** map;
    int size;
    int maxSize;
};

// Create an empty LRU Cache object with both queue and hash map with size 'sz'
struct LRUCache* createLRUCache(int size){
    struct LRUCache* res=(struct LRUCache*)malloc(sizeof(struct LRUCache));
    res->map=(struct LRUCacheMapNode**)malloc(size * sizeof(struct LRUCacheMapNode*));
    for (int i=0; i<size; i++) res->map[i]=NULL;
    res->size=0;
    res->maxSize=size;
    res->head=createQueueNode("");
    res->tail=createQueueNode("");
    res->head->forw=res->tail;
    res->tail->prev=res->head;
    return res;
};

// Insert a queue node in front of LRU Cache queue
void insert(struct LRUCache* obj, struct LRUCacheQueueNode* nd){
    nd->prev = obj->head;
    nd->forw = obj->head->forw;
    obj->head->forw->prev = nd;
    obj->head->forw = nd;
}

// Delete the last word in queue of LRU Cache once full
struct LRUCacheQueueNode* deleteQueue(struct LRUCache* obj){
    struct LRUCacheQueueNode* previousNode = obj->tail->prev;
    previousNode->prev->forw = obj->tail;
    obj->tail->prev = previousNode->prev;
    return previousNode;
}

// Delete the word from hash map once removed from LRU Cache queue
void deleteHsh(struct LRUCache* obj, struct LRUCacheQueueNode* QNode){
    int ind=djb2(QNode->val)%(obj->maxSize);
    struct LRUCacheMapNode* temp=obj->map[ind];
    while (temp->nd!=QNode) temp=temp->forw;
    if (obj->map[ind]==temp) obj->map[ind]=temp->forw;
    else{
        temp->prev->forw=temp->forw;
        if (temp->forw!=NULL) temp->forw->prev=temp->prev;
    }
    free(temp);
}

// Put a new word or replace an already existing word in the front of LRU Cache
void lRUCachePut(struct LRUCache* obj, char* string) {
    int ind=djb2(string)%(obj->maxSize);
    
    if (obj->map[ind]!=NULL){
        struct LRUCacheMapNode* temp=obj->map[ind];
        obj->map[ind]=createHashNode(string);
        obj->map[ind]->forw=temp;
        temp->prev=obj->map[ind];
    }
    else{
        obj->map[ind]=createHashNode(string);
    }
    insert(obj, obj->map[ind]->nd);
    if (obj->size<obj->maxSize) obj->size++;
    else{
        struct LRUCacheQueueNode* QNode=deleteQueue(obj);
        deleteHsh(obj, QNode);
        free(QNode);
    }
}

// Print all the words currently in the queue of LRU Cache
void printQueue(struct LRUCache* obj){
    struct LRUCacheQueueNode* temp=obj->head->forw;
    while (temp!=obj->tail){
        printf(COLOR_CYAN "%s " COLOR_RESET, temp->val);
        temp=temp->forw;
    }
    printf("\n");
}

// Search the hash map of LRU Cache for the word
struct LRUCacheMapNode* searchCache(struct LRUCache* obj, char* string){
    int ind=djb2(string)%(obj->maxSize);
    struct LRUCacheMapNode* temp=obj->map[ind];
    while (temp!=NULL && strcmp(temp->nd->val, string)!=0) temp=temp->forw;
    return temp;
}

// To return the word in queue of LRU Cache if present in hash map
void LRUCacheGet(struct LRUCache* obj, struct LRUCacheMapNode* temp){
    temp->nd->prev->forw=temp->nd->forw;
    temp->nd->forw->prev=temp->nd->prev;
    insert(obj, temp->nd);
}

// To remove all the non alphabets characters from a word
void removeNonAlphabetical(char *word) {
    int i, j = 0;

    for (i = 0; word[i] != '\0'; i++) {
        if (isalpha(word[i])) {
            word[j] = tolower(word[i]);
            j++;
        }
    }
    word[j] = '\0';
}

// To return a list of MAX_SUGGESTIONS words for an incorrect word
void suggest(char *word, struct LRUCache* obj, FILE* dict_ptr){
    fseek(dict_ptr, 0, SEEK_SET);
    char dict_word[MAX_LENGTH + 1];
    double tempJaroWinklerValue = 0;
    int tempLevenshteinValue = INT_MAX;
    while (fscanf(dict_ptr, "%s", dict_word) != EOF){
        int distance = levenshteinDistance(word, dict_word);
        double jaroWinklerValue = jaroWinklerDistance(word, dict_word);
        if (distance < tempLevenshteinValue){
            tempLevenshteinValue=distance;
            lRUCachePut(obj, dict_word);
        }
        else if (distance == tempLevenshteinValue && jaroWinklerValue >= tempJaroWinklerValue){
            tempJaroWinklerValue=jaroWinklerValue;
            lRUCachePut(obj, dict_word);
        }
    }
    printQueue(obj);
}

// To check if word is present in dictionary or not
bool checkWord(TRIE_NODE* root, bool* filter, char *word){
    if (!searchFilter(filter, word)) return false;
    if (!searchTrie(root, word)) return false;
    return true;
}

typedef struct suggest_thread {
    int thread_num;
    struct LRUCache* cache;
    bool* filter;
    char** suggestions;
} THREAD_INP;

void processPart(char* part, struct LRUCache* cache, bool* filter, char** suggestions) {
    char word[MAX_LENGTH];
    const char* delimiters = " \t\n";
    const char* token = strtok(part, delimiters);

    while (token != NULL) {
        strncpy(word, token, MAX_LENGTH);
        removeNonAlphabetical(word);
        if(!searchFilter(filter, word) && suggest_count < MAX_SUGGEST_COUNT) strcpy(suggestions[suggest_count++], word); 
        token = strtok(NULL, delimiters);
    }
    // printf("**im done**");
}

void* processFile(void* thread_inp) {
    // int thread_num = *(int*)arg;
    THREAD_INP* input = (THREAD_INP*) thread_inp;
    FILE* file = fopen(PARA_FILE, "r");
    // printf("done");
    if(file == NULL) {
        perror(COLOR_RED "Error opening paragraph file" COLOR_RESET);
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    long part = file_size / N_THREADS;
    long start = part * input->thread_num;
    long end = input->thread_num < N_THREADS - 1 ? start + part : file_size;
    if(input->thread_num > 0) {
        fseek(file, start, SEEK_SET);
        while(start < end) {
            int c = fgetc(file);
            if(c == ' ' || c == '\t' || c == '\n') break;
            start++;
        }
    }
    // Read and process the part
    fseek(file, start, SEEK_SET);
    char* chunk = (char*) malloc((end-start+1) * sizeof(char));
    fread(chunk, 1, end - start, file);
    chunk[end-start] = '\0';
    fclose(file);
    processPart(chunk, (struct LRUCache*) input->cache, (bool*) input->filter, (char**) input->suggestions);
    free(chunk);
    return NULL;
}