import pandas as pd
import networkx as nx
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
import os
import matplotlib.font_manager as fm

# 配置字体和样式
def configure_plot_style():
    # 查找系统中文字体
    fonts = [f for f in fm.findSystemFonts() if 'hei' in f.lower() or 'yuan' in f.lower() or 'kaiti' in f.lower() or 'songti' in f.lower() or 'yahei' in f.lower()]
    
    # 在Mac系统上常见的中文字体
    mac_fonts = ['STHeiti', 'PingFang SC', 'Hiragino Sans GB', 'Apple LiSung Light', 'Apple LiGothic Medium', 'STFangsong']
    
    # 添加合适的中文字体
    font_families = ['Arial Unicode MS', 'SimHei', 'Microsoft YaHei', 'STSong'] + mac_fonts
    if fonts:
        print(f"找到系统中文字体: {fonts[:3]}...")
        font_families = fonts[:3] + font_families
    
    plt.rcParams['font.sans-serif'] = font_families
    plt.rcParams['axes.unicode_minus'] = False    # 解决负号显示问题
    plt.rcParams['font.size'] = 11  # 默认字体大小
    plt.rcParams['figure.figsize'] = [12, 10]  # 默认图形大小
    # 使用默认纯白色背景，不再使用ggplot样式
    print("配置了绘图样式和中文字体")

# 读取客户点餐信息
def read_customer_store_data(filename="客户拼好饭点餐店面信息.txt"):
    customer_store = {}
    try:
        with open(filename, 'r', encoding='utf-8') as file:
            for line in file:
                if line.startswith('#') or line.strip() == '':
                    continue
                parts = line.strip().split()
                if len(parts) >= 2:
                    customer_id = int(parts[0])
                    store_id = int(parts[1])
                    customer_store[customer_id] = store_id
        print(f"成功读取客户点餐信息，共 {len(customer_store)} 条记录")
        return customer_store
    except Exception as e:
        print(f"读取客户点餐信息出错: {e}")
        return {}

# 读取配送网络数据
def read_delivery_data(filename="delivery_data.txt"):
    graph = {}
    try:
        with open(filename, 'r', encoding='utf-8') as file:
            for line in file:
                if line.startswith('#') or line.strip() == '':
                    continue
                parts = line.strip().split()
                if len(parts) >= 3:
                    node1 = int(parts[0])
                    node2 = int(parts[1])
                    distance = int(parts[2])
                    
                    if node1 not in graph:
                        graph[node1] = {}
                    if node2 not in graph:
                        graph[node2] = {}
                    
                    graph[node1][node2] = distance
                    graph[node2][node1] = distance  # 无向图
        
        print(f"成功读取配送网络数据，共 {len(graph)} 个节点")
        return graph
    except Exception as e:
        print(f"读取配送网络数据出错: {e}")
        return {}

# 读取最优路径数据
def read_optimal_routes(filename="optimal_routes.csv"):
    try:
        routes_df = pd.read_csv(filename)
        print(f"成功读取最优路径数据，共 {len(routes_df)} 条记录")
        return routes_df
    except Exception as e:
        print(f"读取最优路径数据出错: {e}")
        return None

# 可视化最优配送路径
def visualize_optimal_path():
    # 配置绘图样式
    configure_plot_style()
    
    # 读取数据
    customer_store = read_customer_store_data()
    delivery_graph = read_delivery_data()
    optimal_routes = read_optimal_routes()
    
    if not customer_store or not delivery_graph or optimal_routes is None:
        print("无法继续可视化，数据读取失败")
        return
    
    # 创建NetworkX图
    G = nx.Graph()
    
    # 添加边和权重
    for source, destinations in delivery_graph.items():
        for target, weight in destinations.items():
            G.add_edge(source, target, weight=weight)
    
    # 提取最优路径
    path_nodes = []
    
    for _, row in optimal_routes.iterrows():
        source = row['Source']
        destination = row['Destination']
        
        if source not in path_nodes:
            path_nodes.append(source)
        if destination not in path_nodes and destination != source:
            path_nodes.append(destination)
    
    print(f"最优路径包含 {len(path_nodes)} 个节点")
    
    # 创建路径边列表
    path_edges = []
    for i in range(len(optimal_routes)):
        source = optimal_routes.iloc[i]['Source']
        destination = optimal_routes.iloc[i]['Destination']
        if source != destination:  # 排除自环
            path_edges.append((source, destination))
    
    # 创建子图，只包含路径中的节点
    path_graph = G.subgraph(path_nodes)
    
    # 计算总距离
    total_distance = optimal_routes['Distance'].sum()
    
    # 设置图的大小
    plt.figure(figsize=(14, 12))
    
    # 设置节点位置 - 尝试不同的布局算法
    #pos = nx.spring_layout(path_graph, seed=42)  # 弹簧布局
    pos = nx.kamada_kawai_layout(path_graph)  # Kamada-Kawai布局算法，通常效果更好
    
    # 绘制节点
    store_nodes = [n for n in path_graph.nodes() if n < 100]
    customer_nodes = [n for n in path_graph.nodes() if n >= 100]
    
    # 绘制店面节点（方形）
    nx.draw_networkx_nodes(path_graph, pos, 
                          nodelist=store_nodes, 
                          node_color='gold', 
                          node_size=1000,
                          node_shape='s')
    
    # 绘制客户节点（圆形）
    nx.draw_networkx_nodes(path_graph, pos, 
                          nodelist=customer_nodes, 
                          node_color='skyblue', 
                          node_size=800,
                          node_shape='o')
    
    # 绘制边：路径上的边用红色加粗显示，其他边用灰色细线
    path_edge_list = []
    other_edge_list = []
    
    for u, v in path_graph.edges():
        if (u, v) in path_edges or (v, u) in path_edges:
            path_edge_list.append((u, v))
        else:
            other_edge_list.append((u, v))
    
    # 绘制非路径边
    nx.draw_networkx_edges(path_graph, pos, 
                          edgelist=other_edge_list,
                          width=1.0,
                          edge_color='lightgray',
                          style='dashed',
                          alpha=0.5)
    
    # 绘制路径边
    nx.draw_networkx_edges(path_graph, pos, 
                          edgelist=path_edge_list,
                          width=3.0,
                          edge_color='red',
                          arrows=True,
                          arrowsize=20,
                          arrowstyle='->')
    
    # 绘制边的权重标签
    edge_labels = {}
    for u, v, d in path_graph.edges(data=True):
        if 'weight' in d:
            edge_labels[(u, v)] = d['weight']
    
    nx.draw_networkx_edge_labels(path_graph, pos, edge_labels=edge_labels, font_size=10)
    
    # 绘制节点标签 - 使用中文
    store_labels = {node: f"店面 {node}" for node in store_nodes}
    customer_labels = {node: f"客户 {node}\n(从店面{customer_store.get(node, '?')}点餐)" for node in customer_nodes}
    
    # 合并标签字典
    node_labels = {**store_labels, **customer_labels}
    
    # 直接使用中文标签
    nx.draw_networkx_labels(path_graph, pos, labels=node_labels, font_size=10, font_weight='bold')
    
    # 添加路径序号标签
    for i in range(len(path_edges)):
        u, v = path_edges[i]
        # 计算标签位置（在边的中间位置）
        x = (pos[u][0] + pos[v][0]) / 2
        y = (pos[u][1] + pos[v][1]) / 2
        # 添加些偏移量避免与边重叠
        offset_x = (pos[v][0] - pos[u][0]) * 0.1
        offset_y = (pos[v][1] - pos[u][1]) * 0.1
        plt.text(x + offset_x, y + offset_y, f"{i+1}", fontsize=12, 
                 bbox=dict(facecolor='white', alpha=0.7, edgecolor='black', boxstyle='round'),
                 ha='center', va='center')
    
    # 创建图例 - 使用中文
    store_patch = mpatches.Patch(color='gold', label='店面')
    customer_patch = mpatches.Patch(color='skyblue', label='客户')
    path_line = mpatches.Patch(color='red', label='最优配送路径')
    
    plt.legend(handles=[store_patch, customer_patch, path_line], 
              loc='lower right', fontsize=12)
    
    # 添加标题 - 使用中文
    plt.title(f"最优配送路径 (总距离: {total_distance})", fontsize=16)
    
    # 关闭坐标轴
    plt.axis('off')
    plt.tight_layout()
    
    # 保存图像
    plt.savefig("optimal_route_visualization.png", dpi=300, bbox_inches='tight')
    print("最优路径可视化已保存为 'optimal_route_visualization.png'")
    
    # 显示图像
    plt.show()

# 可视化路径顺序
def visualize_path_sequence():
    # 配置绘图样式
    configure_plot_style()
    
    # 读取数据
    customer_store = read_customer_store_data()
    optimal_routes = read_optimal_routes()
    
    if not customer_store or optimal_routes is None:
        print("无法继续可视化，数据读取失败")
        return
    
    # 创建图表
    plt.figure(figsize=(14, 8))
    
    # 提取路径节点
    path_nodes = []
    for _, row in optimal_routes.iterrows():
        source = row['Source']
        destination = row['Destination']
        
        if source not in path_nodes:
            path_nodes.append(source)
        if destination not in path_nodes and destination != source:
            path_nodes.append(destination)
    
    # 计算总距离
    total_distance = optimal_routes['Distance'].sum()
    
    # 节点Y坐标 - 创建更均匀的分布
    y_pos = np.linspace(0, 10, len(path_nodes))
    
    # 绘制节点
    colors = []
    shapes = []
    sizes = []
    for node in path_nodes:
        if node < 100:  # 店面
            colors.append('gold')
            shapes.append('s')  # 方形
            sizes.append(200)
        else:  # 客户
            colors.append('skyblue')
            shapes.append('o')  # 圆形
            sizes.append(180)
    
    # 分别绘制不同形状的节点
    for shape, marker in [('s', 's'), ('o', 'o')]:
        indices = [i for i, s in enumerate(shapes) if s == shape]
        if indices:
            x_coords = [i for i in range(len(path_nodes)) if shapes[i] == shape]
            y_coords = [y_pos[i] for i in range(len(path_nodes)) if shapes[i] == shape]
            node_colors = [colors[i] for i in range(len(path_nodes)) if shapes[i] == shape]
            node_sizes = [sizes[i] for i in range(len(path_nodes)) if shapes[i] == shape]
            plt.scatter(x_coords, y_coords, c=node_colors, s=node_sizes, marker=marker, edgecolors='black', linewidths=1)
    
    # 添加节点标签 - 使用中文
    for i, node in enumerate(path_nodes):
        label = f"店面 {node}" if node < 100 else f"客户 {node} (从店面{customer_store.get(node, '?')}点餐)"
        plt.annotate(label, (i, y_pos[i]), fontsize=10, ha='center', va='bottom')
    
    # 连接路径 - 使用曲线
    for i in range(len(path_nodes) - 1):
        plt.plot([i, i+1], [y_pos[i], y_pos[i+1]], 'r-', linewidth=2)
        # 添加方向箭头
        plt.arrow(i + 0.8, y_pos[i] + 0.8*(y_pos[i+1]-y_pos[i])/2, 0.01, 0.01,
                 head_width=0.3, head_length=0.3, fc='red', ec='red')
        
        # 添加步骤标签 - 使用中文
        plt.text(i + 0.5, (y_pos[i] + y_pos[i+1])/2 + 0.3, f"步骤 {i+1}",
                fontsize=9, color='darkred', 
                bbox=dict(facecolor='white', alpha=0.7, edgecolor='red', boxstyle='round'))
    
    # 添加距离标签 - 使用中文
    for i in range(len(optimal_routes)):
        if i < len(path_nodes) - 1:  # 确保不超出索引范围
            source = optimal_routes.iloc[i]['Source']
            destination = optimal_routes.iloc[i]['Destination']
            distance = optimal_routes.iloc[i]['Distance']
            
            src_idx = path_nodes.index(source)
            dst_idx = path_nodes.index(destination)
            
            if src_idx < len(path_nodes) - 1 and dst_idx == src_idx + 1:
                plt.text(src_idx + 0.5, (y_pos[src_idx] + y_pos[dst_idx])/2 - 0.3,
                        f"距离: {distance}",
                        fontsize=8, color='blue',
                        bbox=dict(facecolor='white', alpha=0.7, edgecolor='blue'))
    
    # 添加标题 - 使用中文
    plt.title(f"最优配送路径顺序 (总距离: {total_distance})", fontsize=14)
    
    # 设置坐标轴 - 使用中文
    plt.xlabel("配送顺序", fontsize=12)
    plt.xticks(range(len(path_nodes)), [f"{i+1}" for i in range(len(path_nodes))])
    plt.yticks([])
    
    # 添加图例 - 使用中文
    store_patch = mpatches.Patch(color='gold', label='店面')
    customer_patch = mpatches.Patch(color='skyblue', label='客户')
    path_line = mpatches.Patch(color='red', label='配送路径')
    
    plt.legend(handles=[store_patch, customer_patch, path_line], 
              loc='lower right', fontsize=12)
    
    plt.tight_layout()
    
    # 保存图像
    plt.savefig("path_sequence_visualization.png", dpi=300, bbox_inches='tight')
    print("路径顺序可视化已保存为 'path_sequence_visualization.png'")
    
    # 显示图像
    plt.show()

if __name__ == "__main__":
    # 可视化最优配送路径
    visualize_optimal_path()
    
    # 可视化路径顺序
    visualize_path_sequence() 