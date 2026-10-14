#ifndef TRACKER_H
#include <time.h>
#define TRACKER_H

#define MAX_PATH_LEN 260    //文件路径最大长度
#define MAX_MESSAGE_LEN 128 //提交说明最大长度
#define HASH_LEN 65         //SHA-256 哈希字符串长度
#define BRANCH_NAME_LEN 64  //分支名最大长度
#define BUF_SIZE "63"       //！！需要BRANCH_LEN同加减！！
#define TAG_NAME_LEN 64     //标签名最大长度
#define MAX_COMMITS 2048
#define MONITOR_TIMES 5000
#define HASH_DISPLAY_LEN 7
#define REPO_PATH_LEN  (MAX_PATH_LEN + 64)
#define STORAGE_PATH_LEN (MAX_PATH_LEN + 192)

#define STR(x)  #x
#define XSTR(x) STR(x)

typedef struct Commit {
    int id;                         //提交ID
    char file_path[MAX_PATH_LEN];   //文件路径
    char message[MAX_MESSAGE_LEN];  //提交信息
    char hash[HASH_LEN];            //提交哈希
    char branch[BRANCH_NAME_LEN];//所属分支
    time_t timestamp;
    struct Commit *prev;            //上一次提交节点地址
    struct Commit *next;            //下一个提交节点地址
} Commit;       //直接用Commit代替，下同

typedef struct BlobRef {
    char hash[HASH_LEN];    //文件哈希
    int ref_count;          //被引用计数
    struct BlobRef *next;   //下一个文件存储节点地址
} BlobRef;

typedef struct Branch {
    char name[BRANCH_NAME_LEN]; //分支名
    Commit *head;               //提交头
    struct Branch *next;        //下一个分支地址
} Branch;

typedef struct Tag {
    char name[TAG_NAME_LEN];    //标签头
    Commit *commit;              //指向的提交
    struct Tag *next;           //下一个标签
} Tag;

typedef struct Tracker {
    char tracked_file[MAX_PATH_LEN];//文件名
    char working_dir[MAX_PATH_LEN]; //工作目录
    char repo_path[REPO_PATH_LEN];   //仓库地址
    int repository_loaded;          //是否已加载文件
    Commit *commit_tail;            //提交链表尾指针
    BlobRef *blob_refs;             //文件记录链表头指针
    Branch *branches;               //分支链表头指针
    Tag *tags;                      //标签链表头指针
    Branch *current_branch;         //当前分支指针
    int next_commit_id;
    Commit *total_commit_head;
    Commit *checked_out;            // 当前检出的版本（工作区对应的commit）
} Tracker;

/*lib.c*/
int path_analysis(char* path, char* tracked_file,char* working_dir);
void hash_data(const unsigned char *data, long size, char hash[HASH_LEN]);
int choice_check();
const char* static_short_hash(const char *hash);
int storage_text_is_safe(const char *text);

/*tracker.c*/
void tracker_init(Tracker *tracker);
void tracker_free(Tracker *tracker);
void save_exit(Tracker *tracker);
void open_or_create_repository(Tracker *tracker);
void commit_version(Tracker *tracker);
void show_history(const Tracker *tracker);
void restore_version(Tracker *tracker);
void watch_files(Tracker *tracker);
void manage_branches(Tracker *tracker);
void manage_tags(Tracker *tracker);
void show_graph(const Tracker *tracker);
void delete_repository(Tracker *tracker);

/*storage.c*/
int storage_init_repository(Tracker *tracker);
int storage_repository_exists(const Tracker *tracker);
int storage_read_file(const char *path, unsigned char **data, long *size);
int storage_write_file(const char *path, const unsigned char *data, long size);
int storage_save_blob(const Tracker *tracker, const char *hash, const unsigned char *data, long size);
int storage_restore_blob(const Tracker *tracker, const char *hash, const char *target_path);
int storage_save_head(const Tracker *tracker);
int storage_load_head(Tracker *tracker);
int storage_save_commits(const Tracker *tracker);
int storage_load_commits(Tracker *tracker);
int storage_save_branches(const Tracker *tracker);
int storage_load_branches(Tracker *tracker);
int storage_save_tags(const Tracker *tracker);
int storage_load_tags(Tracker *tracker);
int storage_save_blobrefs(const Tracker *tracker);
int storage_load_blobrefs(Tracker *tracker);
int storage_save_tracker(const Tracker *tracker);
int storage_load_tracker(Tracker *tracker);
int storage_ft_open_index(const char *dir, const char *file_name, char *full_path_out);
int storage_ft_write_index(const char *dir, const char *file_name, const char *repo_name);


#endif
