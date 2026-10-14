#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define stat _stat
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path) mkdir(path, 0755)
#endif
// 创建目录调用系统接口，Win和mac/linux两套接口（部分跨平台兼容，后期可全兼容）

#include "tracker.h"

int make_dir_if_needed(const char* path) {  // 有判断的创建目录
    struct stat st;
    if (stat(path, &st) == 0) {
        // 已存在，确认是不是文件夹
        return (st.st_mode & S_IFDIR) ? 0 : -1;  // 是文件则返回错误
    }
    // 不存在，创建
    return MKDIR(path);
}

int storage_repository_exists(const Tracker* tracker) {
    struct stat st;
    return stat(tracker->repo_path, &st) == 0 && (st.st_mode & S_IFDIR);
}

int storage_init_repository(Tracker* tracker) {  // 创建程序目录
    char temp[STORAGE_PATH_LEN];
    char temp2[STORAGE_PATH_LEN];
    const size_t len = sizeof(temp);
    snprintf(temp, len, "%s/.ft", tracker->working_dir);
    if (make_dir_if_needed(temp) < 0) {
        printf("storage_init_repository: Unable to create directory %s\n",
               temp);
        system("pause");
        return 0;
    }
    strcpy(temp2, temp);
    snprintf(temp, len, "%s/repos", temp2);
    if (make_dir_if_needed(temp) < 0) {
        printf("storage_init_repository: Unable to create directory %s\n",
               temp);
        system("pause");
        return 0;
    }
    strcpy(temp2, temp);
    snprintf(temp, len, "%s/%s", temp2, tracker->tracked_file);
    if (make_dir_if_needed(temp) < 0) {
        printf("storage_init_repository: Unable to create directory %s\n",
               temp);
        system("pause");
        return 0;
    }
    snprintf(temp, len, "%s/blobs", tracker->repo_path);
    if (make_dir_if_needed(temp) < 0) {
        printf("storage_init_repository: Unable to create directory %s\n",
               temp);
        system("pause");
        return 0;
    }

    if (!storage_save_tracker(tracker)) {
        printf("storage_init_repository: Unable to create files\n");
        system("pause");
        return 0;
    }
    tracker->repository_loaded = 1;
    return 1;
}

int storage_read_file(const char* path, unsigned char** data, long* size) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }

    *size = ftell(file);
    if (*size < 0) {
        fclose(file);
        return 0;
    }

    rewind(file);
    *data = (unsigned char*)malloc((size_t)*size + 1);
    if (*data == NULL) {
        fclose(file);
        return 0;
    }

    if (fread(*data, 1, (size_t)*size, file) != (size_t)*size) {
        free(*data);
        *data = NULL;
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

int storage_write_file(const char* path, const unsigned char* data, long size) {
    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        return 0;
    }

    if (fwrite(data, 1, (size_t)size, file) != (size_t)size) {
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

int storage_save_blob(const Tracker* tracker, const char* hash,
                      const unsigned char* data, long size) {
    char path[STORAGE_PATH_LEN];

    snprintf(path, sizeof(path), "%s/blobs/%s", tracker->repo_path, hash);
    return storage_write_file(path, data, size);
}

int storage_restore_blob(const Tracker* tracker, const char* hash,
                         const char* target_path) {
    char blob_path[STORAGE_PATH_LEN];
    unsigned char* data = NULL;
    long size = 0;
    int ok;

    snprintf(blob_path, sizeof(blob_path), "%s/blobs/%s", tracker->repo_path,
             hash);
    if (!storage_read_file(blob_path, &data, &size)) {
        return 0;
    }

    ok = storage_write_file(target_path, data, size);
    free(data);
    return ok;
}

static void repo_file_path(const Tracker* t, const char* filename, char* out,
                           size_t out_size) {  // 组装文件路径
    snprintf(out, out_size, "%s/%s", t->repo_path, filename);
}

static void strip_newline(char* str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

static int split_line(char* line, char* fields[],
                      int max_fields) {  // 文件字段解析
    int count = 0;
    char* p = line;
    if (!p || *p == '\0') return 0;
    fields[count++] = p;
    while (*p) {
        if (*p == '|') {
            if (count >= max_fields) return 0;
            *p = '\0';
            fields[count++] = p + 1;
        }
        p++;
    }
    return count == max_fields ? count : 0;
}

static Commit* find_commit_by_id(const Tracker* t, const int id) {
    Commit* c = t->commit_tail;
    while (c) {
        if (c->id == id) return c;
        c = c->next;
    }
    return NULL;
}

/*HEAD.io*/

int storage_save_head(const Tracker* tracker) {
    char path[STORAGE_PATH_LEN];
    FILE* fp;
    int checked_out_id = tracker->checked_out ? tracker->checked_out->id : 0;

    repo_file_path(tracker, "HEAD", path, sizeof(path));
    fp = fopen(path, "w");
    if (fp == NULL) return 0;
    fprintf(fp, "%s|%d\n",
            tracker->current_branch ? tracker->current_branch->name : "main",
            checked_out_id);
    fclose(fp);
    return 1;
}

int storage_load_head(Tracker* tracker) {
    char path[STORAGE_PATH_LEN];
    FILE* fp;
    char line[BRANCH_NAME_LEN + 20];
    char* fields[2];
    int checked_out_id = 0;
    Branch* b;
    int n;

    repo_file_path(tracker, "HEAD", path, sizeof(path));
    fp = fopen(path, "r");
    if (fp == NULL) goto fallback;

    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        goto fallback;
    }
    fclose(fp);

    strip_newline(line);
    n = split_line(line, fields, 2);
    if (n == 0) goto fallback;

    // 设置 current_branch
    b = tracker->branches;
    while (b) {
        if (strcmp(fields[0], b->name) == 0) {
            tracker->current_branch = b;
            break;
        }
        b = b->next;
    }
    if (b == NULL) goto fallback;

    // 解析 checked_out_id
    if (n == 2) {
        checked_out_id = atoi(fields[1]);
    }

    // 设置 checked_out
    if (checked_out_id == 0 && tracker->current_branch->head != NULL) {
        tracker->checked_out = tracker->current_branch->head;
    } else if (checked_out_id != 0) {
        tracker->checked_out = find_commit_by_id(tracker, checked_out_id);
        if (tracker->checked_out == NULL) {
            tracker->checked_out = tracker->current_branch->head;
        }
    } else {
        tracker->checked_out = NULL;
    }

    return 1;

fallback:
    b = tracker->branches;
    while (b) {
        if (strcmp("main", b->name) == 0) {
            tracker->current_branch = b;
            tracker->checked_out = b->head;
            return 2;
        }
        b = b->next;
    }
    return 0;
}

/*commits.io*/

int storage_save_commits(const Tracker* tracker) {
    char path[STORAGE_PATH_LEN];
    FILE* fp;
    Commit* c;
    Commit* arr[MAX_COMMITS];
    int i = 0;

    repo_file_path(tracker, "commits", path, sizeof(path));
    fp = fopen(path, "w");
    if (fp == NULL) return 0;
    c = tracker->commit_tail;
    while (c != NULL) {
        arr[i++] = c;
        c = c->next;
    }
    for (i--; i >= 0; i--) {
        fprintf(fp, "%d|%s|%s|%s|%s|%d|%ld\n", arr[i]->id, arr[i]->file_path,
                arr[i]->message, arr[i]->hash, arr[i]->branch,
                arr[i]->prev == NULL ? 0 : arr[i]->prev->id,
                (long)arr[i]->timestamp);
    }
    fclose(fp);
    return 1;
}

int storage_load_commits(Tracker* tracker) {
    char path[STORAGE_PATH_LEN];
    FILE* fp;
    char line[MAX_PATH_LEN + MAX_MESSAGE_LEN + HASH_LEN + BRANCH_NAME_LEN + 40];
    Commit* temp = NULL;
    Commit* c = NULL;
    char* fields[7];
    Commit* id_map[MAX_COMMITS] = {NULL};
    int id_prev[MAX_COMMITS] = {0};

    repo_file_path(tracker, "commits", path, sizeof(path));
    fp = fopen(path, "r");
    if (fp == NULL) return 0;

    tracker->commit_tail = NULL;

    while (fgets(line, sizeof(line), fp)) {
        c = NULL;

        strip_newline(line);
        if (split_line(line, fields, 7) != 7) continue;

        c = (Commit*)malloc(sizeof(Commit));
        if (c == NULL) {
            fclose(fp);
            return 0;
        }

        c->id = atoi(fields[0]);
        strncpy(c->file_path, fields[1], MAX_PATH_LEN - 1);
        c->file_path[MAX_PATH_LEN - 1] =
            '\0';  // 预防过长超载缺失结束符，后期完善处理，下同
        strncpy(c->message, fields[2], MAX_MESSAGE_LEN - 1);
        c->message[MAX_MESSAGE_LEN - 1] = '\0';
        strncpy(c->hash, fields[3], HASH_LEN - 1);
        c->hash[HASH_LEN - 1] = '\0';
        strncpy(c->branch, fields[4], BRANCH_NAME_LEN - 1);
        c->branch[BRANCH_NAME_LEN - 1] = '\0';
        c->timestamp = (time_t)atol(fields[6]);
        if (c->id <= 0 || c->id >= MAX_COMMITS) {
            printf("Storage_Load_Commits: Illegal commit ID %d, skipped\n",
                   c->id);
            free(c);
            continue;
        }
        id_map[c->id] = c;
        id_prev[c->id] = atoi(fields[5]);

        c->next = temp;
        if (temp == NULL) {
            tracker->total_commit_head = c;
            tracker->next_commit_id = c->id + 1;
        }
        temp = c;
    }
    tracker->commit_tail = temp;
    fclose(fp);
    while (temp != NULL) {
        if (temp->id <= 0 || temp->id >= MAX_COMMITS) {
            temp->prev = NULL;
        } else {
            int prev_id = id_prev[temp->id];
            if (prev_id <= 0 || prev_id >= MAX_COMMITS ||
                id_map[prev_id] == NULL)
                temp->prev = NULL;
            else
                temp->prev = id_map[prev_id];
        }
        temp = temp->next;
    }

    return 1;
}

/*branches.io*/

int storage_save_branches(const Tracker* tracker) {
    char path[STORAGE_PATH_LEN];
    FILE* fp;
    Branch* b;
    repo_file_path(tracker, "branches", path, sizeof(path));
    fp = fopen(path, "w");
    if (!fp) return 0;
    b = tracker->branches;
    while (b) {
        fprintf(fp, "%s|%d\n", b->name, b->head ? b->head->id : 0);
        b = b->next;
    }
    fclose(fp);
    return 1;
}

int storage_load_branches(Tracker* tracker) {
    char path[STORAGE_PATH_LEN];
    FILE* fp;
    char line[BRANCH_NAME_LEN + 20];
    Branch* tail = tracker->branches;
    char* fields[2];
    Branch* b;
    int head_id;

    repo_file_path(tracker, "branches", path, sizeof(path));
    fp = fopen(path, "r");
    if (fp == NULL) return 0;

    while (fgets(line, sizeof(line), fp)) {
        strip_newline(line);
        if (split_line(line, fields, 2) != 2) continue;
        if (strcmp(fields[0], "main") == 0) {
            head_id = atoi(fields[1]);
            if (head_id == 0)
                tracker->branches->head = NULL;
            else {
                tracker->branches->head = find_commit_by_id(tracker, head_id);
                if (tracker->branches->head == NULL)
                    printf(
                        "Storage_Load_Branches: commit of branch \"main\" not "
                        "found\n");
            }
            continue;
        }
        b = (Branch*)malloc(sizeof(Branch));
        if (b == NULL) {
            fclose(fp);
            return 0;
        }

        strncpy(b->name, fields[0], BRANCH_NAME_LEN - 1);
        b->name[BRANCH_NAME_LEN - 1] = '\0';
        head_id = atoi(fields[1]);
        b->next = NULL;
        if (head_id == 0)
            b->head = NULL;
        else {
            b->head = find_commit_by_id(tracker, head_id);
            if (b->head == NULL) {
                printf(
                    "Storage_Load_Branches: commit of branch \"%s\" not "
                    "found\n",
                    b->name);
                free(b);
                continue;
            }
        }
        if (tail)
            tail->next = b;
        else
            tracker->branches = b;
        tail = b;
    }

    fclose(fp);
    return 1;
}

/*tags.io*/

int storage_save_tags(const Tracker* tracker) {
    char path[STORAGE_PATH_LEN];
    FILE* fp;
    Tag* t;
    repo_file_path(tracker, "tags", path, sizeof(path));
    fp = fopen(path, "w");
    if (!fp) return 0;
    t = tracker->tags;
    while (t) {
        if (t->commit == NULL)
            printf(
                "Storage_Save_Tags: commit of tag \"%s\" lost, give up the "
                "tag\n",
                t->name);
        else
            fprintf(fp, "%s|%d\n", t->name, t->commit->id);
        t = t->next;
    }
    fclose(fp);
    return 1;
}

int storage_load_tags(Tracker* tracker) {
    char path[STORAGE_PATH_LEN];
    FILE* fp;
    char line[TAG_NAME_LEN + 20];
    Tag* tail_t = NULL;
    char* fields[2];
    Tag* t;
    int commit_id;

    repo_file_path(tracker, "tags", path, sizeof(path));
    fp = fopen(path, "r");
    if (!fp) return 0;

    tracker->tags = NULL;

    while (fgets(line, sizeof(line), fp)) {
        strip_newline(line);
        if (split_line(line, fields, 2) != 2) continue;

        t = (Tag*)malloc(sizeof(Tag));
        if (!t) {
            fclose(fp);
            return 0;
        }

        strncpy(t->name, fields[0], TAG_NAME_LEN - 1);
        t->name[TAG_NAME_LEN - 1] = '\0';
        commit_id = atoi(fields[1]);
        t->next = NULL;
        t->commit = find_commit_by_id(tracker, commit_id);
        if (t->commit == NULL) {
            printf("Storage_Load_Tags: commit of tag \"%s\" not found\n",
                   t->name);
            free(t);
            continue;
        }
        if (tail_t)
            tail_t->next = t;
        else
            tracker->tags = t;
        tail_t = t;
    }

    fclose(fp);
    return 1;
}

/*blobrefs.io*/

int storage_save_blobrefs(const Tracker* tracker) {
    char path[STORAGE_PATH_LEN];
    FILE* fp;
    BlobRef* b;
    repo_file_path(tracker, "blobrefs", path, sizeof(path));
    fp = fopen(path, "w");
    if (fp == NULL) return 0;
    b = tracker->blob_refs;
    while (b != NULL) {
        fprintf(fp, "%s|%d\n", b->hash, b->ref_count);
        b = b->next;
    }
    fclose(fp);
    return 1;
}

int storage_load_blobrefs(Tracker* tracker) {
    char path[STORAGE_PATH_LEN];
    FILE* fp;
    char line[HASH_LEN + 20];
    BlobRef* tail_b = NULL;
    char* fields[2];
    BlobRef* b;

    repo_file_path(tracker, "blobrefs", path, sizeof(path));
    fp = fopen(path, "r");
    if (fp == NULL) return 0;

    tracker->blob_refs = NULL;

    while (fgets(line, sizeof(line), fp)) {
        strip_newline(line);
        if (split_line(line, fields, 2) != 2) continue;

        b = (BlobRef*)malloc(sizeof(BlobRef));
        if (b == NULL) {
            fclose(fp);
            return 0;
        }

        strncpy(b->hash, fields[0], HASH_LEN - 1);
        b->hash[HASH_LEN - 1] = '\0';
        b->ref_count = atoi(fields[1]);
        b->next = NULL;

        if (tail_b)
            tail_b->next = b;
        else
            tracker->blob_refs = b;
        tail_b = b;
    }

    fclose(fp);
    return 1;
}

/*tracker.io*/

int storage_save_tracker(const Tracker* tracker) {
    if (!storage_save_commits(tracker)) return 0;
    if (!storage_save_blobrefs(tracker)) return 0;
    if (!storage_save_branches(tracker)) return 0;
    if (!storage_save_tags(tracker)) return 0;
    if (!storage_save_head(tracker)) return 0;
    return 1;
}

int storage_load_tracker(Tracker* tracker) {
    int status;
    if (!storage_load_commits(tracker)) return 0;
    if (!storage_load_blobrefs(tracker)) return 0;
    if (!storage_load_branches(tracker)) return 0;
    if (!storage_load_tags(tracker)) return 0;
    status = storage_load_head(tracker);
    if (status == 0) return 0;
    if (status == 2)
        printf("Storage_Load_Tracker:加载分支头失败！已回退到main分支！\n");
    tracker->repository_loaded = 1;
    return 1;
}

int storage_ft_write_index(const char* dir, const char* file_name,
                           const char* full_path) {
    FILE* fp;
    char idx_path[STORAGE_PATH_LEN];
    char line[MAX_PATH_LEN * 2];
    char* fields[2];
    int found = 0;

    snprintf(idx_path, sizeof(idx_path), "%s/.ft/index", dir);

    /* 检查是否已存在 */
    fp = fopen(idx_path, "r");
    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp)) {
            strip_newline(line);
            if (split_line(line, fields, 2) == 2) {
                if (strcmp(fields[0], file_name) == 0) {
                    found = 1;
                    break;
                }
            }
        }
        fclose(fp);
    }

    if (found) return 1;

    fp = fopen(idx_path, "a");
    if (fp == NULL) return 0;
    fprintf(fp, "%s|%s\n", file_name, full_path);
    fclose(fp);
    return 1;
}

int storage_ft_open_index(const char* dir, const char* file_name,
                          char* full_path_out) {
    FILE* fp;
    char idx_path[STORAGE_PATH_LEN];
    char line[MAX_PATH_LEN * 2];
    char* fields[2];

    snprintf(idx_path, sizeof(idx_path), "%s/.ft/index", dir);
    fp = fopen(idx_path, "r");
    if (fp == NULL) return 0;

    while (fgets(line, sizeof(line), fp)) {
        strip_newline(line);
        if (split_line(line, fields, 2) == 2) {
            if (strcmp(fields[0], file_name) == 0) {
                strncpy(full_path_out, fields[1], MAX_PATH_LEN - 1);
                full_path_out[MAX_PATH_LEN - 1] = '\0';
                fclose(fp);
                return 1;
            }
        }
    }
    fclose(fp);
    return 0;
}
