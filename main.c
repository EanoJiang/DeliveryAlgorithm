#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_NODES 200  // 最大节点数量，包括店面和客户
#define MAX_STORES 10  // 最大店面数量
#define MAX_CUSTOMERS 100  // 最大客户数量
#define MAX_PICKUPS 5  // 最大取餐数量
#define INF 9999  // 表示无穷大距离

// 存储店面和客户之间的距离
int graph[MAX_NODES][MAX_NODES];
// 存储客户订单对应的店面
int customer_store[MAX_NODES];
// 所有节点的访问状态
int visited[MAX_NODES];
// 最优路径
int optimal_path[MAX_NODES * 2];
int path_length = 0;
// 配送员当前取餐状态
int curr_orders[MAX_PICKUPS];
int order_count = 0;
// 待配送的订单
int pending_orders[MAX_CUSTOMERS];
int pending_count = 0;

// 初始化图
void init_graph() {
    for (int i = 0; i < MAX_NODES; i++) {
        for (int j = 0; j < MAX_NODES; j++) {
            if (i == j) {
                graph[i][j] = 0;
            } else {
                graph[i][j] = INF;
            }
        }
        visited[i] = 0;
        customer_store[i] = -1;
    }
}

// 从delivery_data.txt文件读取配送网络数据
void read_delivery_data(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("无法打开文件: %s\n", filename);
        exit(1);
    }

    char line[100];
    int node1, node2, distance;

    while (fgets(line, sizeof(line), file)) {
        // 跳过注释行
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        // 解析行数据
        if (sscanf(line, "%d %d %d", &node1, &node2, &distance) == 3) {
            graph[node1][node2] = distance;
            graph[node2][node1] = distance;  // 无向图
        }
    }

    fclose(file);
}

// 从客户拼好饭点餐店面信息.txt文件读取客户点餐信息
void read_customer_store_data(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("无法打开文件: %s\n", filename);
        exit(1);
    }

    char line[100];
    int customer_id, store_id;

    while (fgets(line, sizeof(line), file)) {
        // 跳过注释行
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        // 解析行数据
        if (sscanf(line, "%d %d", &customer_id, &store_id) == 2) {
            customer_store[customer_id] = store_id;
            pending_orders[pending_count++] = customer_id;
            printf("读取到客户 %d 从店面 %d 点餐\n", customer_id, store_id);
        }
    }

    fclose(file);
}

// 保存最优路径到CSV文件
void save_to_csv(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("无法创建文件: %s\n", filename);
        return;
    }

    // 写入邻接矩阵形式的CSV
    fprintf(file, "Source,Destination,Distance\n");
    for (int i = 0; i < path_length - 1; i++) {
        fprintf(file, "%d,%d,%d\n", optimal_path[i], optimal_path[i+1], graph[optimal_path[i]][optimal_path[i+1]]);
    }

    fclose(file);
    printf("已保存最优路径到 %s\n", filename);
}

// 检查客户的餐是否已经取了
int is_customer_order_picked(int customer_id) {
    for (int i = 0; i < order_count; i++) {
        if (curr_orders[i] == customer_id) {
            return 1;
        }
    }
    return 0;
}

// 按店面分组客户订单
void group_orders_by_store(int *store_customer_count) {
    for (int i = 0; i < pending_count; i++) {
        int customer = pending_orders[i];
        int store = customer_store[customer];
        store_customer_count[store]++;
    }
}

// 寻找最近的节点
int find_nearest(int current_node, int max_pickups) {
    int nearest_node = -1;
    int min_distance = INF;
    
    // 首先检查是否需要送餐（已取的餐）
    for (int i = 0; i < order_count; i++) {
        int customer = curr_orders[i];
        if (!visited[customer] && graph[current_node][customer] < min_distance) {
            min_distance = graph[current_node][customer];
            nearest_node = customer;
        }
    }
    
    // 如果没有需要立即送的餐，且未达到最大取餐数量，考虑取餐
    if (nearest_node == -1 && order_count < max_pickups) {
        // 查找未访问的店面，且有客户等待该店面的餐
        for (int i = 1; i <= MAX_STORES; i++) {
            if (!visited[i]) {
                // 检查是否有未配送的订单来自这个店面
                int has_pending_orders = 0;
                for (int j = 0; j < pending_count; j++) {
                    int customer = pending_orders[j];
                    if (!visited[customer] && !is_customer_order_picked(customer) && customer_store[customer] == i) {
                        has_pending_orders = 1;
                        break;
                    }
                }
                
                if (has_pending_orders && graph[current_node][i] < min_distance) {
                    min_distance = graph[current_node][i];
                    nearest_node = i;
                }
            }
        }
    }
    
    // 如果还是没找到，考虑所有店面
    if (nearest_node == -1) {
        for (int i = 1; i <= MAX_STORES; i++) {
            // 检查是否有未配送的订单来自这个店面
            int has_pending_orders = 0;
            for (int j = 0; j < pending_count; j++) {
                int customer = pending_orders[j];
                if (!visited[customer] && !is_customer_order_picked(customer) && customer_store[customer] == i) {
                    has_pending_orders = 1;
                    break;
                }
            }
            
            if (has_pending_orders && graph[current_node][i] < min_distance) {
                min_distance = graph[current_node][i];
                nearest_node = i;
            }
        }
    }
    
    return nearest_node;
}

// 检查是否完成所有订单
int all_completed() {
    for (int i = 0; i < pending_count; i++) {
        if (!visited[pending_orders[i]]) {
            return 0;
        }
    }
    return 1;
}

// 优化配送路径
void optimize_delivery(int start_node, int max_pickups) {
    // 重置状态
    path_length = 0;
    order_count = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        visited[i] = 0;
    }
    
    int current_node = start_node;
    optimal_path[path_length++] = current_node;
    
    while (!all_completed()) {
        // 查找最近的下一个节点
        int next_node = find_nearest(current_node, max_pickups);
        
        if (next_node == -1) {
            // 如果没有找到下一个节点，但有已取的餐需要送
            if (order_count > 0) {
                // 找出最近的客户送餐
                int nearest_customer = -1;
                int min_dist = INF;
                
                for (int i = 0; i < order_count; i++) {
                    int customer = curr_orders[i];
                    if (!visited[customer] && graph[current_node][customer] < min_dist) {
                        min_dist = graph[current_node][customer];
                        nearest_customer = customer;
                    }
                }
                
                if (nearest_customer != -1) {
                    next_node = nearest_customer;
                }
            }
            
            // 如果还是没找到，尝试找到任何未访问的客户的店面
            if (next_node == -1) {
                for (int i = 0; i < pending_count; i++) {
                    int customer = pending_orders[i];
                    if (!visited[customer]) {
                        int store = customer_store[customer];
                        // 去取餐
                        next_node = store;
                        break;
                    }
                }
            }
            
            // 如果还是找不到，则无法完成所有订单
            if (next_node == -1) {
                printf("无法找到合理的路径来完成所有订单\n");
                break;
            }
        }
        
        // 更新路径和当前节点
        optimal_path[path_length++] = next_node;
        current_node = next_node;
        
        // 如果是店面，取该店面的餐
        if (current_node >= 1 && current_node <= MAX_STORES) {
            visited[current_node] = 1;  // 标记店面为已访问
            // 查找所有从该店面点餐的客户
            for (int i = 0; i < pending_count; i++) {
                int customer = pending_orders[i];
                if (!visited[customer] && !is_customer_order_picked(customer) && customer_store[customer] == current_node) {
                    // 如果达到最大取餐数量，则不再取餐
                    if (order_count >= max_pickups) {
                        break;
                    }
                    curr_orders[order_count++] = customer;
                }
            }
        }
        // 如果是客户，送该客户的餐
        else if (current_node >= 101) {
            visited[current_node] = 1;  // 标记客户为已访问
            // 更新已取订单列表
            for (int i = 0; i < order_count; i++) {
                if (curr_orders[i] == current_node) {
                    // 移除已送达的订单
                    for (int j = i; j < order_count - 1; j++) {
                        curr_orders[j] = curr_orders[j + 1];
                    }
                    order_count--;
                    break;
                }
            }
        }
    }
}

// 计算路径总距离
int calculate_total_distance(int path[], int len) {
    int total = 0;
    for (int i = 0; i < len - 1; i++) {
        total += graph[path[i]][path[i+1]];
    }
    return total;
}

// 打印最优路径
void print_optimal_path() {
    printf("\n------------------------\n");
    printf("最优配送路径: ");
    for (int i = 0; i < path_length; i++) {
        printf("%d", optimal_path[i]);
        if (i < path_length - 1) {
            printf(" -> ");
        }
    }
    printf("\n总配送距离: %d\n", calculate_total_distance(optimal_path, path_length));
}

// 打印取餐送餐过程
void print_delivery_process(int start_node, int max_pickups) {
    // 重置状态
    path_length = 0;
    order_count = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        visited[i] = 0;
    }
    
    int current_node = start_node;
    optimal_path[path_length++] = current_node;
    
    printf("\n开始配送过程...\n");
    printf("------------------------\n");
    
    while (!all_completed()) {
        // 查找最近的下一个节点
        int next_node = find_nearest(current_node, max_pickups);
        
        if (next_node == -1) {
            // 如果没有找到下一个节点，但有已取的餐需要送
            if (order_count > 0) {
                // 找出最近的客户送餐
                int nearest_customer = -1;
                int min_dist = INF;
                
                for (int i = 0; i < order_count; i++) {
                    int customer = curr_orders[i];
                    if (!visited[customer] && graph[current_node][customer] < min_dist) {
                        min_dist = graph[current_node][customer];
                        nearest_customer = customer;
                    }
                }
                
                if (nearest_customer != -1) {
                    next_node = nearest_customer;
                }
            }
            
            // 如果还是没找到，尝试找到任何未访问的客户的店面
            if (next_node == -1) {
                for (int i = 0; i < pending_count; i++) {
                    int customer = pending_orders[i];
                    if (!visited[customer]) {
                        int store = customer_store[customer];
                        // 去取餐
                        next_node = store;
                        break;
                    }
                }
            }
            
            // 如果还是找不到，则无法完成所有订单
            if (next_node == -1) {
                printf("无法找到合理的路径来完成所有订单\n");
                break;
            }
        }
        
        // 更新路径和当前节点
        optimal_path[path_length++] = next_node;
        current_node = next_node;
        
        // 如果是店面，取该店面的餐
        if (current_node >= 1 && current_node <= MAX_STORES) {
            visited[current_node] = 1;  // 标记店面为已访问
            // 查找所有从该店面点餐的客户
            for (int i = 0; i < pending_count; i++) {
                int customer = pending_orders[i];
                if (!visited[customer] && !is_customer_order_picked(customer) && customer_store[customer] == current_node) {
                    // 如果达到最大取餐数量，则不再取餐
                    if (order_count >= max_pickups) {
                        break;
                    }
                    curr_orders[order_count++] = customer;
                    printf("取餐：从店面 %d 取餐，送往客户 %d\n", current_node, customer);
                }
            }
        }
        // 如果是客户，送该客户的餐
        else if (current_node >= 101) {
            visited[current_node] = 1;  // 标记客户为已访问
            // 更新已取订单列表
            for (int i = 0; i < order_count; i++) {
                if (curr_orders[i] == current_node) {
                    printf("送餐：将店面 %d 的餐送到客户 %d\n", customer_store[current_node], current_node);
                    // 移除已送达的订单
                    for (int j = i; j < order_count - 1; j++) {
                        curr_orders[j] = curr_orders[j + 1];
                    }
                    order_count--;
                    break;
                }
            }
        }
    }
}

// 寻找最优起始店面
int find_optimal_start_store(int max_pickups) {
    int best_start = 1;
    int min_total_distance = INF;
    int temp_path[MAX_NODES * 2];
    int temp_path_len = 0;
    
    printf("正在计算最优起始店面...\n");
    
    // 尝试每个店面作为起始点
    for (int start = 1; start <= MAX_STORES; start++) {
        // 检查该店面是否有订单
        int has_orders = 0;
        for (int i = 0; i < pending_count; i++) {
            if (customer_store[pending_orders[i]] == start) {
                has_orders = 1;
                break;
            }
        }
        
        if (!has_orders) {
            continue;  // 跳过没有订单的店面
        }
        
        // 计算以该店面为起点的路径
        optimize_delivery(start, max_pickups);
        
        // 计算总距离
        int total_distance = calculate_total_distance(optimal_path, path_length);
        
        printf("  从店面 %d 开始：总距离 = %d\n", start, total_distance);
        
        // 更新最优起点
        if (total_distance < min_total_distance) {
            min_total_distance = total_distance;
            best_start = start;
            
            // 保存临时路径
            temp_path_len = path_length;
            for (int i = 0; i < path_length; i++) {
                temp_path[i] = optimal_path[i];
            }
        }
    }
    
    // 恢复最优路径
    path_length = temp_path_len;
    for (int i = 0; i < path_length; i++) {
        optimal_path[i] = temp_path[i];
    }
    
    printf("最优起始店面是：店面 %d (总距离: %d)\n", best_start, min_total_distance);
    return best_start;
}

int main() {
    int max_pickups = 3;  // 默认骑手一次最多取3个餐
    int store_customer_count[MAX_STORES+1] = {0};  // 每个店面的客户数量

    printf("外卖配送时间优化系统\n");
    printf("========================\n");
    
    // 让用户选择最大取餐数量
    printf("请输入骑手一次最多能取的餐数量 (1-%d，默认为3): ", MAX_PICKUPS);
    char input[10];
    fgets(input, sizeof(input), stdin);
    if (sscanf(input, "%d", &max_pickups) != 1 || max_pickups < 1 || max_pickups > MAX_PICKUPS) {
        printf("输入无效或超出范围，使用默认值: 3\n");
        max_pickups = 3;
    }
    
    printf("------------------------\n");
    printf("初始化系统...\n");
    
    // 初始化
    init_graph();
    
    // 读取数据
    printf("读取配送网络数据...\n");
    read_delivery_data("delivery_data.txt");
    
    printf("读取客户点餐信息...\n");
    read_customer_store_data("客户拼好饭点餐店面信息.txt");
    
    // 按店面分组客户订单信息
    group_orders_by_store(store_customer_count);
    
    printf("------------------------\n");
    printf("共有 %d 个客户订单\n", pending_count);
    printf("各店面订单分布:\n");
    for (int i = 1; i <= MAX_STORES; i++) {
        if (store_customer_count[i] > 0) {
            printf("店面 %d: %d 个订单\n", i, store_customer_count[i]);
        }
    }
    
    printf("------------------------\n");
    printf("配送参数设置:\n");
    printf("骑手一次最多取 %d 个餐\n", max_pickups);
    
    // 自动寻找最优起始店面
    int optimal_start = find_optimal_start_store(max_pickups);
    
    // 使用最优起始店面进行配送路径规划，并打印详细过程
    print_delivery_process(optimal_start, max_pickups);
    
    // 显示结果
    print_optimal_path();
    
    // 保存到CSV
    save_to_csv("optimal_routes.csv");
    
    printf("配送任务完成！\n");
    
    return 0;
}