#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FRAMES 10
#define MAX_PAGES 50

// Node structure for singly linked list
typedef struct Node {
    int page;
    struct Node* next;
} Node;

// Hash table for LRU
Node* hash_table[MAX_PAGES];

// Global variables
int frame_size;
int page_sequence[MAX_PAGES];
int num_pages;
int page_faults_fifo = 0;
int page_faults_lru = 0;

// FIFO linked list
Node* fifo_head = NULL;
Node* fifo_tail = NULL;

// LRU linked list
Node* lru_head = NULL;

// Function prototypes
void simulateFIFO(HWND hwnd);
void simulateLRU(HWND hwnd);
void enqueueFIFO(int page);
void dequeueFIFO();
void moveToHeadLRU(Node* node);
void addToLRU(int page);

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static HWND hwndFrameSize, hwndPageSequence, hwndSimulateFIFO, hwndSimulateLRU, hwndOutput;
    static char output_text[10000];

    switch (msg) {
        case WM_CREATE:
            CreateWindow("STATIC", "Frame Size:", WS_VISIBLE | WS_CHILD, 20, 20, 100, 20, hwnd, NULL, NULL, NULL);
            hwndFrameSize = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 120, 20, 100, 20, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Page Sequence (space-separated):", WS_VISIBLE | WS_CHILD, 20, 60, 200, 20, hwnd, NULL, NULL, NULL);
            hwndPageSequence = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 220, 60, 200, 20, hwnd, NULL, NULL, NULL);

            hwndSimulateFIFO = CreateWindow("BUTTON", "Simulate FIFO", WS_VISIBLE | WS_CHILD, 20, 100, 150, 30, hwnd, (HMENU)1, NULL, NULL);
            hwndSimulateLRU = CreateWindow("BUTTON", "Simulate LRU", WS_VISIBLE | WS_CHILD, 200, 100, 150, 30, hwnd, (HMENU)2, NULL, NULL);

            hwndOutput = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                                      20, 150, 400, 300, hwnd, NULL, NULL, NULL);
            break;

        case WM_COMMAND:
            if (LOWORD(wp) == 1 || LOWORD(wp) == 2) {
                // Get frame size
                char frame_size_text[10];
                GetWindowText(hwndFrameSize, frame_size_text, 10);
                frame_size = atoi(frame_size_text);

                // Get page sequence
                char page_sequence_text[500];
                GetWindowText(hwndPageSequence, page_sequence_text, 500);
                char *token = strtok(page_sequence_text, " ");
                num_pages = 0;
                while (token != NULL && num_pages < MAX_PAGES) {
                    page_sequence[num_pages++] = atoi(token);
                    token = strtok(NULL, " ");
                }

                // Clear output
                strcpy(output_text, "");

                if (LOWORD(wp) == 1) {
                    strcat(output_text, "Simulating FIFO:\r\n\r\n");
                    simulateFIFO(hwnd);
                } else {
                    strcat(output_text, "Simulating LRU:\r\n\r\n");
                    simulateLRU(hwnd);
                }

                SetWindowText(hwndOutput, output_text);
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

void enqueueFIFO(int page) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->page = page;
    new_node->next = NULL;
    if (fifo_tail) {
        fifo_tail->next = new_node;
    } else {
        fifo_head = new_node;
    }
    fifo_tail = new_node;
}

void dequeueFIFO() {
    if (fifo_head) {
        Node* temp = fifo_head;
        fifo_head = fifo_head->next;
        if (!fifo_head) {
            fifo_tail = NULL;
        }
        free(temp);
    }
}

void simulateFIFO(HWND hwnd) {
    fifo_head = fifo_tail = NULL;
    page_faults_fifo = 0;

    for (int i = 0; i < num_pages; i++) {
        int page = page_sequence[i];
        int found = 0;

        Node* temp = fifo_head;
        while (temp) {
            if (temp->page == page) {
                found = 1;
                break;
            }
            temp = temp->next;
        }

        if (!found) {
            page_faults_fifo++;
            if (frame_size > 0) {
                if (frame_size > 0 && frame_size-- > 0) {
                    enqueueFIFO(page);
                } else {
                    dequeueFIFO();
                    enqueueFIFO(page);
                }
            }
        }

        // Append step to output
        strcat(output_text, "FIFO ");
    }
}

// Code continuation from the previous response

void moveToHeadLRU(Node* node) {
    if (node == lru_head) return;

    Node* temp = lru_head;
    while (temp && temp->next != node) {
        temp = temp->next;
    }

    if (temp) {
        temp->next = node->next;
        node->next = lru_head;
        lru_head = node;
    }
}

void addToLRU(int page) {
    if (hash_table[page]) {
        moveToHeadLRU(hash_table[page]);
    } else {
        Node* new_node = (Node*)malloc(sizeof(Node));
        new_node->page = page;
        new_node->next = lru_head;
        lru_head = new_node;
        hash_table[page] = new_node;

        if (frame_size-- <= 0) {
            Node* temp = lru_head;
            while (temp && temp->next && temp->next->next) {
                temp = temp->next;
            }

            if (temp && temp->next) {
                hash_table[temp->next->page] = NULL;
                free(temp->next);
                temp->next = NULL;
            }
        }
    }
}

void simulateLRU(HWND hwnd) {
    memset(hash_table, 0, sizeof(hash_table));
    lru_head = NULL;
    page_faults_lru = 0;

    for (int i = 0; i < num_pages; i++) {
        int page = page_sequence[i];

        if (!hash_table[page]) {
            page_faults_lru++;
        }
        addToLRU(page);

        strcat(output_text, "LRU step"); // Simplified output update
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpszClassName = "PageReplacementSimulator";
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpfnWndProc = WindowProcedure;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName, "Page Replacement Simulator", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              100, 100, 500, 500, NULL, NULL, hInstance, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
