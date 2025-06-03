#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define MAX_SYMBOLS 256 //все возм байты
#define MAX_TREE_HEIGHT 100 // макс глубина дерева
#define MAX_INPUT_SIZE 1024 // макс допустимая строка
#define MAX_BITSTREAM_SIZE 8192 // для результ

// Структура узла дерева Хаффмана
typedef struct Node{
    unsigned char symbol;
    int freq;
    struct Node* left;
    struct Node* right;
} Node;

// Структура для хранения кодов символов
typedef struct{
    unsigned char symbol;
    char code[MAX_TREE_HEIGHT]; // 0/1
} HuffCode;

// Структура очереди с приоритетом
typedef struct{
    Node** nodes; 
    int size;// текущий размер
    int capacity; // макс вместимость
} PriorityQueue;

// Создание нового узла
Node* create_node(unsigned char symbol, int freq, Node* left, Node* right){
    Node* node = (Node*)malloc(sizeof(Node));
    if (!node) {
        perror("Не удалось выделить память");
        exit(EXIT_FAILURE);
    }
    node->symbol = symbol;
    node->freq = freq;
    node->left = left;
    node->right = right;
    return node;
}

// Инициализация очереди с приоритетом
PriorityQueue* create_priority_queue(int capacity) {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    pq->nodes = (Node**)malloc(capacity * sizeof(Node*));
    pq->size = 0;
    pq->capacity = capacity;
    return pq;
}

// Добавление элемента в очередь
void enqueue(PriorityQueue* pq, Node* node){ // добавляет в очередь минимальный элемент
    if (pq->size >= pq->capacity) {
        fprintf(stderr, "Переполнение\n");
        exit(EXIT_FAILURE);
    }
    
    // Вставка с сохранением порядка
    int i = pq->size;
    while (i > 0 && node->freq < pq->nodes[(i-1)/2]->freq) {
        pq->nodes[i] = pq->nodes[(i-1)/2];
        i = (i-1)/2;
    }
    pq->nodes[i] = node;
    pq->size++;
}

// Извлечение минимального элемента
Node* dequeue(PriorityQueue* pq) {
    if (pq->size == 0) {
        fprintf(stderr, "Переполнение\n");
        exit(EXIT_FAILURE);
    }
    
    Node* min_node = pq->nodes[0];
    pq->size--;
    
    // Перемещаем последний элемент в начало
    Node* last_node = pq->nodes[pq->size];
    int i = 0;
    int child = 1;
    
    while (child < pq->size) {
        // Выбираем меньшего потомка
        if (child + 1 < pq->size && 
            pq->nodes[child + 1]->freq < pq->nodes[child]->freq) {
            child++;
        }
        
        // Если последний элемент меньше потомка - нашли место
        if (last_node->freq <= pq->nodes[child]->freq) break;
        
        // Перемещаем потомка вверх
        pq->nodes[i] = pq->nodes[child];
        i = child;
        child = 2 * i + 1;
    }
    pq->nodes[i] = last_node;
    return min_node;
}

// Построение дерева Хаффмана
Node* build_huffman_tree(int freq[]){
    // Создаем очередь с приоритетом
    PriorityQueue* pq = create_priority_queue(MAX_SYMBOLS);
    
    // Добавляем все символы с ненулевой частотой
    for (int i = 0; i < MAX_SYMBOLS; i++){
        if (freq[i] > 0) {
            enqueue(pq, create_node((unsigned char)i, freq[i], NULL, NULL));
        }
    }
    
    // Специальный случай: только один символ
    if (pq->size == 1){
        Node* only_node = dequeue(pq);
        Node* dummy_node = create_node(0, only_node->freq, only_node, NULL);
        free(pq->nodes);
        free(pq);
        return dummy_node;
    }
    
    // Строим дерево
    while (pq->size > 1){ // идет пока не оставнется один узел - то есть корень
        Node* left = dequeue(pq);
        Node* right = dequeue(pq);
        Node* parent = create_node(0, left->freq + right->freq, left, right);
        enqueue(pq, parent);
    }
    
    Node* root = dequeue(pq);
    free(pq->nodes);
    free(pq);
    return root;
}

// Рекурсивная генерация кодов Хаффмана
void generate_codes(Node* root, HuffCode* codes, char* buffer, int depth) {
    if (!root) return;
    
    // Если достигли листа
    if (!root->left && !root->right) {
        buffer[depth] = '\0';
        codes[root->symbol].symbol = root->symbol;
        strncpy(codes[root->symbol].code, buffer, MAX_TREE_HEIGHT);
        return;
    }
    
    // Рекурсивный обход левого поддерева
    if (root->left) {
        buffer[depth] = '0';
        generate_codes(root->left, codes, buffer, depth + 1);
    }
    
    // Рекурсивный обход правого поддерева
    if (root->right) {
        buffer[depth] = '1';
        generate_codes(root->right, codes, buffer, depth + 1);
    }
}

// Освобождение памяти дерева
void free_tree(Node* root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

// Вывод битов в hex
void print_bitstream_hex(const char* bitstream) {
    int len = strlen(bitstream);
    if (len == 0) {
        printf("00\n");
        return;
    }
    
    // Рассчитываем необходимое дополнение
    int padding = (8 - (len % 8)) % 8;
    
    // Создаем буфер с дополнением для hex
    char full[MAX_BITSTREAM_SIZE] = {0};
    memset(full, '0', padding);
    strcat(full, bitstream);
    
    // Преобразуем биты в байты
    int byte_count = strlen(full) / 8;
    for (int i = 0; i < byte_count; i++) {
        char byte_str[9] = {0};
        strncpy(byte_str, full + i*8, 8);
        uint8_t byte = (uint8_t)strtol(byte_str, NULL, 2);
        printf("%02X ", byte);
    }
}

// ASCII hex-печать
void print_ascii_hex(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        printf("%02X ", (unsigned char)str[i]);
    }
}


// сколько раз символ встречается в строке
void calculate_frequencies(const char* input, int freq[]) {
    size_t len = strlen(input);
    for (size_t i = 0; i < len; ++i) {
        freq[(unsigned char)input[i]]++;
    }
}

// Декодирование битовой последовательности (то есть восстанавлвает ориг текст)
void decode_bitstream(const char* bitstream, Node* root, char* output) {
    Node* current = root;
    int pos = 0;
    int len = strlen(bitstream);
    
    for (int i = 0; i < len; i++) {
        if (bitstream[i] == '0') {
            current = current->left;
        } else {
            current = current->right;
        }
        
        // Достигли листа
        if (!current->left && !current->right) {
            output[pos++] = current->symbol;
            current = root;
        }
    }
    output[pos] = '\0';
}

// Таблица кодов (просто увеличим тут код)
void print_code_table(HuffCode* codes) {
    printf("\nHuffman Code Table:\n");
    printf("+-------+----------+---------------------+\n");
    printf("| Char  | ASCII    | Code                |\n");
    printf("+-------+----------+---------------------+\n");
    
    for (int i = 0; i < MAX_SYMBOLS; i++) {
        if (codes[i].code[0] != '\0') {
            char display[10] = {0};
            if (isprint(i) && !isspace(i)) {
                snprintf(display, sizeof(display), "'%c'", i);
            } else {
                snprintf(display, sizeof(display), "Ctrl-%d", i);
            }
            
            printf("| %-5s | 0x%02X     | %-19s |\n", 
                   display, i, codes[i].code);
        }
    }
    printf("+-------+----------+---------------------+\n");
}

// Основная функция
int main() {
    char input[MAX_INPUT_SIZE];
    char decoded[MAX_INPUT_SIZE];
    int freq[MAX_SYMBOLS] = {0};
    HuffCode codes[MAX_SYMBOLS] = {0};
    char buffer[MAX_BITSTREAM_SIZE] = {0};
    char bitstream[MAX_BITSTREAM_SIZE] = {0};

    // Ввод данных
    printf("Введите слово: ");
    if (!fgets(input, sizeof(input), stdin)) {
        fprintf(stderr, "Ошибка чтения ввода\n");
        return EXIT_FAILURE;
    }
    input[strcspn(input, "\n")] = '\0'; // Удаляем символ новой строки
    
    if (strlen(input) == 0) {
        fprintf(stderr, "Ошибка: введена пустая строка\n");
        return EXIT_FAILURE;
    }

    // Шаг 1: Подсчет частот символов
    calculate_frequencies(input, freq);

    // Шаг 2: Построение дерева Хаффмана
    Node* root = build_huffman_tree(freq); // создание приоритетной очереди
    if (!root) {
        fprintf(stderr, "Ошибка построения дерева Хаффмана\n");
        return EXIT_FAILURE;
    }

    // Шаг 3: Генерация кодов
    generate_codes(root, codes, buffer, 0);
    
    // Шаг 4: Кодирование входной строки
    size_t len = strlen(input);
    for (size_t i = 0; i < len; ++i) {
        strcat(bitstream, codes[(unsigned char)input[i]].code);
    }

    // Вывод результатов
    printf("\nИсходное слово: \"%s\"\n", input);
    printf("Длина исходной строки: %zu байт\n", len);
    
    printf("\nШестнадцатеричное представление (ASCII):\n");
    print_ascii_hex(input);
    
    printf("\n\nСжатое представление (Хаффман):\n");
    printf("Битовая последовательность: %s\n", bitstream);
    printf("Шестнадцатеричный дамп: ");
    print_bitstream_hex(bitstream);
    
    // Дополнительная информация
    int bitstream_len = strlen(bitstream);
    int compressed_bytes = (bitstream_len + 7) / 8;
    printf("\n\nСтатистика сжатия:");
    printf("\nБитов исходных: %zu", len * 8);
    printf("\nБитов сжатых: %d", bitstream_len);
    printf("\nЭкономия: %.2f%%", 
           (1 - (float)bitstream_len/(len*8)) * 100);
    printf("\nРазмер сжатых данных: %d байт", compressed_bytes);
    
    // Печатаем таблицу кодов
    print_code_table(codes);
    
    // Проверка декодированием
    decode_bitstream(bitstream, root, decoded);
    printf("\nРезультат декодирования: \"%s\"", decoded);
    printf("\nСовпадение с исходным: %s", 
           strcmp(input, decoded) == 0 ? "Да" : "Нет");
    
    // Освобождение ресурсов
    free_tree(root);
    
    printf("\n\nПрограмма успешно завершена.\n");
    return EXIT_SUCCESS;
}