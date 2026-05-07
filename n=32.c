#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// 向量生成器状态结构
typedef struct {
    int n;              // 维度
    int B;              // 分支数
    int k;              // 当前1的个数 (2 ≤ k ≤ B/2)
    unsigned long long mask;  // n位掩码
    unsigned long long current; // 当前二进制数
} VectorGenerator;

// 计算二进制数中1的个数
int count_ones(unsigned long long x) {
    int count = 0;
    while (x) {
        count += x & 1;
        x >>= 1;
    }
    return count;
}

// 计算最小循环移位形式
unsigned long long find_min_rotation(unsigned long long x, int n) {
    unsigned long long min = x;
    unsigned long long mask = (n >= 64) ? ~0ULL : (1ULL << n) - 1;
    
    for (int i = 0; i < n; i++) {
        x = (x >> 1) | ((x & 1) << (n-1));  // 循环右移一位
        x &= mask;
        if (x < min) min = x;
    }
    return min;
}

// 初始化生成器
VectorGenerator* init_generator(int n, int B) {
    if (n <= 0 || B < 4) return NULL;  // 至少需要B≥4才能有k≥2
    
    VectorGenerator* gen = (VectorGenerator*)malloc(sizeof(VectorGenerator));
    if (!gen) return NULL;
    
    gen->n = n;
    gen->B = B;
    gen->k = 2;  // 从k=2开始
    gen->mask = (n >= 64) ? ~0ULL : (1ULL << n) - 1;
    gen->current = (1ULL << gen->k) - 1;  // 初始值：最低k位为1
    
    return gen;
}

// 生成下一个有效向量（返回NULL表示结束）
int* generate_next(VectorGenerator* gen) {
    if (!gen) return NULL;
    
    while (gen->k <= gen->B/2) {
        // 使用Gosper's Hack生成下一个k位组合
        if (gen->current == 0) {
            // 重置为k个1的初始值
            gen->current = (1ULL << gen->k) - 1;
        } else {
            // 生成下一个组合
            unsigned long long c = gen->current & -gen->current;
            unsigned long long r = gen->current + c;
            if (r == 0) {
                // 处理溢出
                gen->k++;
                if (gen->k > gen->B/2) return NULL;
                gen->current = (1ULL << gen->k) - 1;
                continue;
            }
            gen->current = (((r ^ gen->current) >> 2) / c) | r;
            if (gen->current > gen->mask) {
                gen->k++;
                if (gen->k > gen->B/2) return NULL;
                gen->current = (1ULL << gen->k) - 1;
                continue;
            }
        }
        
        // 计算最小循环移位形式
        unsigned long long min_rot = find_min_rotation(gen->current, gen->n);
        
        // 仅保留最小形式的代表
        if (gen->current == min_rot) {
            // 分配并填充向量
            int* vector = (int*)malloc(gen->n * sizeof(int));
            if (!vector) return NULL;
            
            for (int i = 0; i < gen->n; i++) {
                vector[i] = (gen->current >> (gen->n - 1 - i)) & 1;
            }
            
            return vector;  // 返回新分配的向量，由调用者释放
        }
    }
    
    return NULL;  // 所有组合已生成完毕
}

// 释放生成器资源
void free_generator(VectorGenerator* gen) {
    if (gen) free(gen);
}



// 打印向量
void print_vector(int *arrays, int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", arrays[i]);
    }
    printf("\n");
}

// 释放所有向量
void free_arrays(int **arrays, int count) {
    for (int i = 0; i < count; i++) {
        free(arrays[i]);
    }
    free(arrays);
}

// 检查在指定位置插入 1 是否会导致相邻
int is_valid(int *arr, int pos, int n) {
    if (pos > 0 && arr[pos - 1] == 1) return 0;
    if (pos < n - 1 && arr[pos + 1] == 1) return 0;
    return 1;
}

// 递归生成所有可能的数组
void generate_combinations_of_best(int *arr, int n, int remaining_ones, int start, int ***result, int *count, int *capacity) {
    if (remaining_ones == 0) {
        if (*count == *capacity) {
            *capacity *= 2;
            *result = (int **)realloc(*result, *capacity * sizeof(int *));
            if (*result == NULL) {
                fprintf(stderr, "内存分配失败: 无法为结果数组扩容\n");
                return;
            }
        }
        int *temp = (int *)malloc(n * sizeof(int));
        if (temp == NULL) {
            fprintf(stderr, "内存分配失败: 无法为长度为 %d 的数组分配内存\n", n);
            return;
        }
        for (int i = 0; i < n; i++) {
            temp[i] = arr[i];
        }
        (*result)[*count] = temp;
        (*count)++;
        return;
    }

    for (int i = start; i < n - 1; i++) {
        if (is_valid(arr, i, n)) {
            arr[i] = 1;
            generate_combinations_of_best(arr, n, remaining_ones - 1, i + 2, result, count, capacity);
            arr[i] = 0;
        }
    }
}

// 生成满足条件的数组
int **generate_best_arrays(int n, int x, int t, int B, int *count) {
    if (x < 0 || x > t - 3 || t > n) {
        *count = 0;
        return NULL;
    }

    int remaining_ones = t - x - 3;

    if (remaining_ones < 0 || (x + 10 >= n - 2 && remaining_ones > 0)) {
        *count = 0;
        return NULL;
    }

    int *base_arr = (int *)calloc(n, sizeof(int));
    if (base_arr == NULL) {
        fprintf(stderr, "内存分配失败: 无法为基础数组分配内存\n");
        *count = 0;
        return NULL;
    }

    // 前 x 个元素置为 1
    for (int i = 0; i < x; i++) {
        base_arr[i] = 1;
    }

    // 固定位置置为 0
    if (x < n) base_arr[x] = 0;
    if (x + 2 < n) base_arr[x + 2] = 0;
    if (x + 3 < n) base_arr[x + 3] = 0;
    if (x + 5 < n) base_arr[x + 5] = 0;
    if (x + 6 < n) base_arr[x + 6] = 0;
    if (x + 7 < n) base_arr[x + 7] = 0;
    if (x + 9 < n) base_arr[x + 9] = 0;
    if (n > 0) base_arr[n - 1] = 0;

    // 固定位置置为 1
    if (x + 1 < n) base_arr[x + 1] = 1;
    if (x + 4 < n) base_arr[x + 4] = 1;
    if (x + 8 < n) base_arr[x + 8] = 1;

    int capacity = 10;
    int **result = (int **)malloc(capacity * sizeof(int *));
    if (result == NULL) {
        fprintf(stderr, "内存分配失败: 无法为结果数组分配内存\n");
        free(base_arr);
        *count = 0;
        return NULL;
    }

    *count = 0;

    // 处理剩余1的放置
    if (remaining_ones == 0) {
        int *temp = (int *)malloc(n * sizeof(int));
        if (temp == NULL) {
            fprintf(stderr, "内存分配失败: 无法为结果数组元素分配内存\n");
            free(base_arr);
            for (int i = 0; i < *count; i++) {
                free(result[i]);
            }
            free(result);
            *count = 0;
            return NULL;
        }
        for (int i = 0; i < n; i++) {
            temp[i] = base_arr[i];
        }
        result[*count] = temp;
        (*count)++;
    } else {
        generate_combinations_of_best(base_arr, n, remaining_ones, x + 10, &result, count, &capacity);
    }
    
    free(base_arr);

    result = (int **)realloc(result, *count * sizeof(int *));
    if (result == NULL) {
        fprintf(stderr, "内存分配失败: 无法调整结果数组大小\n");
        *count = 0;
        for (int i = 0; i < *count; i++) {
            free(result[i]);
        }
        free(result);
        return NULL;
    }

    return result;
}




void right_shift(int *arr, int n) {
    if (n <= 0) return;
    int temp = arr[n - 1];
    for (int i = n - 1; i > 0; i--) {
        arr[i] = arr[i - 1];
    }
    arr[0] = temp;
}

// 生成矩阵
int **generate_matrix(int *L, int n) {
    // 为矩阵分配内存
    int **matrix = (int **)malloc(n * sizeof(int *));
    if (matrix == NULL) {
        fprintf(stderr, "内存分配失败: 无法为矩阵分配内存\n");
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        matrix[i] = (int *)malloc(n * sizeof(int));
        if (matrix[i] == NULL) {
            fprintf(stderr, "内存分配失败: 无法为矩阵行分配内存\n");
            for (int j = 0; j < i; j++) {
                free(matrix[j]);
            }
            free(matrix);
            return NULL;
        }
    }

    // 复制原始数组到矩阵第一行
    for (int j = 0; j < n; j++) {
        matrix[0][j] = L[j];
    }

    // 生成后续行，每次循环右移一次
    for (int i = 1; i < n; i++) {
        // 复制上一行到当前行
        for (int j = 0; j < n; j++) {
            matrix[i][j] = matrix[i - 1][j];
        }
        // 对当前行进行循环右移
        right_shift(matrix[i], n);
    }

    return matrix;
}

void writeMatrixToFile(int n, int B, int **matrix) {
    FILE *file = fopen("n_32_new.txt", "a+");
    if (file == NULL) {
        perror("无法打开文件");
        return;
    }

    // 写入 n 的值
    fprintf(file, "n = %d\n", n);

    // 写入 B 的值
    fprintf(file, "B = %d\n", B);

    fprintf(file, "[");
    for(int i = 0;i<n;i++){
        if(matrix[0][i] == 1){
            fprintf(file, "%d,",i);
        }
    }
    fprintf(file, "]\n");

    // 写入矩阵
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fprintf(file, "%d ", matrix[i][j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

// 释放矩阵内存
void free_matrix(int **matrix, int n) {
    for (int i = 0; i < n; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

int wb(int arr[], int n) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (arr[i] == 1) {
            count++;
        }
    }
    return count;
}

// 判断 GF(2) 域上的矩阵是否可逆
int is_invertible(int **matrix, int n) {
    int **augmented = (int **)malloc(n * sizeof(int *));
    if (augmented == NULL) {
        fprintf(stderr, "内存分配失败: 无法为增广矩阵分配内存\n");
        return 0;
    }
    for (int i = 0; i < n; i++) {
        augmented[i] = (int *)malloc(2 * n * sizeof(int));
        if (augmented[i] == NULL) {
            fprintf(stderr, "内存分配失败: 无法为增广矩阵行分配内存\n");
            for (int j = 0; j < i; j++) {
                free(augmented[j]);
            }
            free(augmented);
            return 0;
        }
        for (int j = 0; j < n; j++) {
            augmented[i][j] = matrix[i][j];
        }
        for (int j = n; j < 2 * n; j++) {
            augmented[i][j] = (i == j - n)? 1 : 0;
        }
    }

    // 高斯消元法化为行阶梯形矩阵
    for (int i = 0; i < n; i++) {
        // 寻找主元
        if (augmented[i][i] == 0) {
            int found = 0;
            for (int k = i + 1; k < n; k++) {
                if (augmented[k][i] == 1) {
                    // 交换行
                    for (int j = 0; j < 2 * n; j++) {
                        int temp = augmented[i][j];
                        augmented[i][j] = augmented[k][j];
                        augmented[k][j] = temp;
                    }
                    found = 1;
                    break;
                }
            }
            if (!found) {
                // 无法找到主元，矩阵不可逆
                for (int i = 0; i < n; i++) {
                    free(augmented[i]);
                }
                free(augmented);
                return 0;
            }
        }

        // 消去下方元素
        for (int k = i + 1; k < n; k++) {
            if (augmented[k][i] == 1) {
                for (int j = 0; j < 2 * n; j++) {
                    augmented[k][j] = (augmented[k][j] + augmented[i][j]) % 2;
                }
            }
        }
    }

    // 回代过程
    for (int i = n - 1; i >= 0; i--) {
        for (int k = i - 1; k >= 0; k--) {
            if (augmented[k][i] == 1) {
                for (int j = 0; j < 2 * n; j++) {
                    augmented[k][j] = (augmented[k][j] + augmented[i][j]) % 2;
                }
            }
        }
    }

    // 检查是否为单位矩阵
    for (int i = 0; i < n; i++) {
        if (augmented[i][i] != 1) {
            for (int i = 0; i < n; i++) {
                free(augmented[i]);
            }
            free(augmented);
            return 0;
        }
    }

    // 释放增广矩阵的内存
    for (int i = 0; i < n; i++) {
        free(augmented[i]);
    }
    free(augmented);

    return 1;
}

void swap_rows(int **matrix, int row1, int row2, int cols) {
    for (int j = 0; j < cols; j++) {
        int temp = matrix[row1][j];
        matrix[row1][j] = matrix[row2][j];
        matrix[row2][j] = temp;
    }
}

// 高斯 - 约旦消元法求逆矩阵
int **inverse_matrix(int **matrix, int n) {
    // 创建增广矩阵 [A | I]
    int **augmented = (int **)malloc(n * sizeof(int *));
    if (augmented == NULL) {
        fprintf(stderr, "内存分配失败: 无法为增广矩阵分配内存\n");
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        augmented[i] = (int *)malloc(2 * n * sizeof(int));
        if (augmented[i] == NULL) {
            fprintf(stderr, "内存分配失败: 无法为增广矩阵行分配内存\n");
            for (int j = 0; j < i; j++) {
                free(augmented[j]);
            }
            free(augmented);
            return NULL;
        }
        for (int j = 0; j < n; j++) {
            augmented[i][j] = matrix[i][j];
        }
        for (int j = n; j < 2 * n; j++) {
            augmented[i][j] = (i == j - n)? 1 : 0;
        }
    }

    // 高斯 - 约旦消元过程
    for (int i = 0; i < n; i++) {
        // 寻找主元
        if (augmented[i][i] == 0) {
            int found = 0;
            for (int k = i + 1; k < n; k++) {
                if (augmented[k][i] == 1) {
                    swap_rows(augmented, i, k, 2 * n);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                // 矩阵不可逆
                for (int i = 0; i < n; i++) {
                    free(augmented[i]);
                }
                free(augmented);
                return NULL;
            }
        }

        // 消去当前列的其他元素
        for (int k = 0; k < n; k++) {
            if (k != i && augmented[k][i] == 1) {
                for (int j = 0; j < 2 * n; j++) {
                    augmented[k][j] = (augmented[k][j] + augmented[i][j]) % 2;
                }
            }
        }
    }

    // 提取逆矩阵
    int **inverse = (int **)malloc(n * sizeof(int *));
    if (inverse == NULL) {
        fprintf(stderr, "内存分配失败: 无法为逆矩阵分配内存\n");
        for (int i = 0; i < n; i++) {
            free(augmented[i]);
        }
        free(augmented);
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        inverse[i] = (int *)malloc(n * sizeof(int));
        if (inverse[i] == NULL) {
            fprintf(stderr, "内存分配失败: 无法为逆矩阵行分配内存\n");
            for (int j = 0; j < i; j++) {
                free(inverse[j]);
            }
            free(inverse);
            for (int i = 0; i < n; i++) {
                free(augmented[i]);
            }
            free(augmented);
            return NULL;
        }
        for (int j = 0; j < n; j++) {
            inverse[i][j] = augmented[i][j + n];
        }
    }

    // 释放增广矩阵的内存
    for (int i = 0; i < n; i++) {
        free(augmented[i]);
    }
    free(augmented);

    return inverse;
}


// 计算GF(2)域上矩阵与向量的乘法
int *vec_muti_mat(int **matrix, int *vector, int n,int t) {
    // 为结果向量分配内存
    int *result = (int *)malloc(n * sizeof(int));
    if (result == NULL) {
        fprintf(stderr, "内存分配失败: 无法为结果向量分配内存\n");
        return NULL;
    }
    int *L = (int *)malloc(t * sizeof(int));

    // 初始化结果向量为0
    for (int i = 0; i < n; i++) {
        result[i] = 0;
    }

    // 进行矩阵与向量的乘法
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            // GF(2)域上的加法和乘法
            result[i] = (result[i] + matrix[i][j] * vector[j]) % 2;
        }
    }

    return result;
}



int main() {
    int n = 32;
    int t = 11;
    int B = 12;
    int x = t+2-B/2;
    int cnt_arr;
    int **arrays = generate_best_arrays(n, x, t, B, &cnt_arr);
    printf("cnt_arr: %d\n", cnt_arr);
    fflush(stdout);
    for(int i=0;i<cnt_arr;i++){
        int **M;
        int **inv_M;
        M = generate_matrix(arrays[i], n);
        if (M == NULL) {
                continue;
        }
        print_vector(arrays[i],n);
        fflush(stdout);
        free(arrays[i]);

        int miniw = 2 * n;
        if (is_invertible(M, n)){
            inv_M = inverse_matrix(M, n);
            if (inv_M == NULL) {
                free_matrix(M, n);
                continue;
            }
            VectorGenerator* gen = init_generator(n, B);
            if (!gen) {
                printf("Failed to initialize generator\n");
                return 1;
            }
            int* vector;
            while ((vector = generate_next(gen)) != NULL) {
                // 使用当前向量（计算分支数）
                int w1 = 0;
                for(int k =0;k<n;k++){
                    int res = 0;
                    for(int j=0;j<n;j++){
                        res = res^(M[k][j]*vector[j]);
                    }
                    if(vector[k] == 1){
                        w1 +=1;
                    }
                    if(res==1){
                        w1 +=1;
                    }
                }
                if(w1<miniw){
                    miniw = w1;
                }
                int w2 = 0;
                for(int k =0;k<n;k++){
                    int res = 0;
                    for(int j=0;j<n;j++){
                        res = res^(inv_M[k][j]*vector[j]);
                    }
                    if(vector[k] == 1){
                        w2+=1;
                    }
                    if(res==1){
                        w2 +=1;
                    }
                }
                if(w2<miniw){
                    miniw = w2;
                }
                free(vector);
            }
            
            printf("i = %d,w = %d\n", i,miniw);
            fflush(stdout);
            free_generator(gen);
            if (miniw == B) {
                printf("find: n = %d , B = %d\n", n, B);
                fflush(stdout);
                writeMatrixToFile(n,B,M);
                free_matrix(M, n);
                free_matrix(inv_M, n);
                

            } 
            else {
                free_matrix(M, n);
                free_matrix(inv_M, n);
            }

        }
        else{
            free_matrix(M, n);
            printf("i = %d,can not reverse \n",i);
            fflush(stdout);

        }



    }

    return 0;
}