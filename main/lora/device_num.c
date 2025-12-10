#include "device_num.h"
#include <stdio.h>
#include <string.h>


// 传感器名称的固定部分常量
#define SENSOR_NAME_DATE    "20250401"
#define NAME_LENGTH         16

// 土壤传感器的深度编码
#define SS_DEPTH_5CM        "05"
#define SS_DEPTH_20CM       "20"

// 三合一传感器的通用位置编码
#define PCPS_POS_CODE       "00"

/**
 * @brief 根据索引生成土壤传感器 (ss) 编号
 * * @param index 收发器模块的传感器顺序 (1 到 18)
 * @param buffer 用于存储生成编号的缓冲区 (至少需要 NAME_LENGTH 字节)
 * @return int 成功写入的字符数
 */
int generate_ss_name(int index, char *buffer) {
    if (index < 1 || index > 18) {
        return -1; // 索引越界
    }

    // 2. 确定深度编码 (Position Code)
    const char *depth_code;
    if (index % 2 != 0) {
        // 奇数索引: 5cm 深度
        depth_code = SS_DEPTH_5CM;
    } else {
        // 偶数索引: 20cm 深度
        depth_code = SS_DEPTH_20CM;
    }

    // 3. 使用 snprintf 拼接字符串 (snprintf 更安全，可防止缓冲区溢出)
    // 格式: ss + YYYYMMDD + 深度编码 + 地块序列号 + 传感器ID
    // 假设最后三位 ID 和地块序列号 plot_id 是相同的 (001, 002, ...)
    return snprintf(buffer, NAME_LENGTH,
                    "ss%s%s%03d",
                    SENSOR_NAME_DATE, // 20250401
                    depth_code,       // 0500 或 2000
                    index);         // 传感器ID (001 - 009)
}


/**
 * @brief 根据索引生成三合一传感器 (ps/pc) 编号
 * * @param index 收发器模块的传感器顺序 (1 到 18)
 * @param buffer 用于存储生成编号的缓冲区
 * @return int 成功写入的字符数
 */
int generate_pspc_name(int index, char *buffer) {
    if (index < 1 || index > 18) {
        return -1; // 索引越界
    }

    // 2. 确定传感器类型码 (ps 或 pc)
    const char *type_code;
    if (index % 2 != 0) {
        // 奇数索引: ps (棚边 - side)
        type_code = "ps";
    } else {
        // 偶数索引: pc (棚中 - center)
        type_code = "pc";
    }

    // 3. 使用 snprintf 拼接字符串
    // 格式: ps/pc + YYYYMMDD + 0000 + 地块序列号 + 传感器ID
    return snprintf(buffer, NAME_LENGTH,
                    "%s%s%s%03d",
                    type_code,        // ps 或 pc
                    SENSOR_NAME_DATE, // 20250401
                    PCPS_POS_CODE,    // 通用位置编码 0000
                    index);         // 传感器ID (001 - 009)
}


// --- 3. main 函数测试 ---
int main() {
    char ss_name[NAME_LENGTH];
    char pspc_name[NAME_LENGTH];
    int plot_id;
    int i;
    
    printf("==================== 传感器索引映射测试 (1-%d) ====================\n", NAME_LENGTH);
    printf("地块编号 | 索引 | 土壤传感器 (ss) - 深度 | 三合一传感器 (ps/pc) - 位置\n");
    printf("---------|------|------------------------|-------------------------------\n");

    for (i = 1; i <= 18; i++) {
        // 计算地块 ID
        plot_id = (i + 1) / 2;
        
        // 1. 生成 SS 编号
        generate_ss_name(i, ss_name);
        const char *ss_desc = (i % 2 != 0) ? "5cm (0500)" : "20cm (2000)";

        // 2. 生成 PS/PC 编号
        generate_pspc_name(i, pspc_name);
        const char *pspc_desc = (i % 2 != 0) ? "棚边 (ps)" : "棚中 (pc)";
        
        // 3. 打印结果
        printf("  D%02d    | %02d   | %s (%s) | %s (%s)\n", 
               plot_id, 
               i, 
               ss_name, ss_desc, 
               pspc_name, pspc_desc);
               
        // 每两个索引（即每完成一个地块）添加分隔线
        if (i % 2 == 0) {
            printf("---------|------|------------------------|-------------------------------\n");
        }
    }
    
    return 0;
}
