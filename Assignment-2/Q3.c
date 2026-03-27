#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int sudoku[9][9];
int validity[27];
int heatmap[9][9];

struct val_param {
    int id;
};

struct heatmap_param {
    int row;
};

void* validate_sudoku(void* arg) {
    struct val_param* param = (struct val_param*)arg;
    int id = param->id;
    int is_valid = 1;
    int seen[10] = {0};
    
    if (id < 9) {
        int r = id;
        for (int c = 0; c < 9; c++) {
            int val = sudoku[r][c];
            if (val < 1 || val > 9 || seen[val]) { is_valid = 0; break; }
            seen[val] = 1;
        }
    } else if (id < 18) {
        int c = id - 9;
        for (int r = 0; r < 9; r++) {
            int val = sudoku[r][c];
            if (val < 1 || val > 9 || seen[val]) { is_valid = 0; break; }
            seen[val] = 1;
        }
    } else {
        int sg = id - 18;
        int start_row = (sg / 3) * 3;
        int start_col = (sg % 3) * 3;
        
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                int val = sudoku[start_row + i][start_col + j];
                if (val < 1 || val > 9 || seen[val]) { is_valid = 0; break; }
                seen[val] = 1;
            }
        }
    }
    
    validity[id] = is_valid;
    pthread_exit(NULL);
}

void* generate_heatmap(void* arg) {
    struct heatmap_param* param = (struct heatmap_param*)arg;
    int r = param->row;
    
    for (int c = 0; c < 9; c++) {
        int sg = (r / 3) * 3 + (c / 3);
        
        int row_err = !validity[r];
        int col_err = !validity[9 + c];
        int sg_err = !validity[18 + sg];
        
        heatmap[r][c] = row_err + col_err + sg_err;
    }
    
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    FILE* input = stdin;
    if (argc >= 2) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            perror("Failed to open input file");
            return 1;
        }
    }

    for (int i = 0; i < 9; i++) {
        char row_str[15];
        if (fscanf(input, "%14s", row_str) != 1) {
            printf("Error reading input.\n");
            if (input != stdin) { fclose(input); }
            return 1;
        }
        for (int j = 0; j < 9; j++) {
            sudoku[i][j] = row_str[j] - '0';
        }
    }

    pthread_t val_threads[27];
    struct val_param val_params[27];
    
    for (int i = 0; i < 27; i++) {
        val_params[i].id = i;
        pthread_create(&val_threads[i], NULL, validate_sudoku, (void*)&val_params[i]);
    }
    
    int total_valid_threads = 0;
    int is_sudoku_valid = 1;
    
    for (int i = 0; i < 27; i++) {
        pthread_join(val_threads[i], NULL);
        if (validity[i] == 1) {
            total_valid_threads++;
        } else {
            is_sudoku_valid = 0;
        }
    }
    
    if (is_sudoku_valid) {
        printf("VALID\n");
    } else {
        printf("INVALID\n");
        
        double validity_score = ((double)total_valid_threads / 27.0) * 100.0;
        printf("Validity Score: %.2f%%\n", validity_score);
        
        pthread_t heat_threads[9];
        struct heatmap_param heat_params[9];
        
        for (int i = 0; i < 9; i++) {
            heat_params[i].row = i;
            pthread_create(&heat_threads[i], NULL, generate_heatmap, (void*)&heat_params[i]);
        }
        
        for (int i = 0; i < 9; i++) {
            pthread_join(heat_threads[i], NULL);
        }
        
        printf("Error Heatmap:\n");
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                printf("%d", heatmap[i][j]);
            }
            printf("\n");
        }
    }
    if (input != stdin) { fclose(input); }
    return 0;
}