#pragma once

// 每个段最大的文档数
#define SEGMENT_MAX_DOC 100000

// 查询返回的最大条数
#define LOOKUP_MAX_SIZE 3000

// 触发段合并 deleted:total 的比例
#define MERGE_DELETED_RATIO 0.2

// 引擎操作的队列大小
#define QUEUE_SIZE_INPUT 128

// 查询相关
#define LOOKUP_THREAD_NUM 8
#define LOOKUP_QUEUE_SIZE 16

