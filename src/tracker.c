#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <conio.h>
#include <sys/stat.h>

#ifdef _WIN32
    #define stat _stat
#endif

#include "tracker.h"

static Branch *create_branch(const char *name, Commit *head) {
    Branch *branch = (Branch *) malloc(sizeof(Branch));
    if (branch == NULL) {
        return NULL;
    }

    strncpy(branch->name, name, BRANCH_NAME_LEN - 1);
    branch->name[BRANCH_NAME_LEN - 1] = '\0';
    branch->head = head;
    branch->next = NULL;
    return branch;
}

void tracker_init(Tracker *tracker) {
    tracker->tracked_file[0] = '\0';
    tracker->working_dir[0] = '\0';
    tracker->repo_path[0] = '\0';
    tracker->repository_loaded = 0;
    tracker->next_commit_id = 1;
    tracker->total_commit_head = NULL;
    tracker->commit_tail = NULL;
    tracker->blob_refs = NULL;
    tracker->branches = create_branch("main", NULL);
    tracker->tags = NULL;
    tracker->current_branch = tracker->branches;
    tracker->checked_out = NULL;
}

void tracker_free(Tracker *tracker) {
    //释放所有内存
    Commit *commit = tracker->commit_tail;
    BlobRef *blob_ref = tracker->blob_refs;
    Branch *branch = tracker->branches;
    Tag *tag = tracker->tags;

    while (commit != NULL) {
        Commit *next = commit->next;
        free(commit);
        commit = next;
    }

    while (blob_ref != NULL) {
        BlobRef *next = blob_ref->next;
        free(blob_ref);
        blob_ref = next;
    }

    while (branch != NULL) {
        Branch *next = branch->next;
        free(branch);
        branch = next;
    }

    while (tag != NULL) {
        Tag *next = tag->next;
        free(tag);
        tag = next;
    }
}

void save_exit(Tracker *tracker) {
    int i=0;
    system("cls");
    printf("状态保存和程序退出模块\n");
    printf("---------------------------------\n");
    if (tracker->repository_loaded == 1) {

        printf("正在保存数据，请稍后...\n");
        storage_save_tracker(tracker);
        system("cls");
        printf("状态保存和程序退出模块\n");
        printf("---------------------------------\n");
        printf("数据已保存！\n");
    }
    printf("是否退出程序？\n");
    printf("1-是\n");
    i = choice_check();
    if (i != 1) return;
    exit(0);
}

void open_or_create_repository(Tracker *tracker) {
    //初始化tracker
    int i;
    char path[MAX_PATH_LEN];
    char tracked_file[MAX_PATH_LEN];
    char working_dir[MAX_PATH_LEN];
    FILE *fp;
    int repo_path_len;
    size_t len;
    int c;

    system("cls");
    printf("仓库打开和程序初始化模块\n");
    printf("---------------------------------\n");
    if (tracker->repository_loaded == 1) {
        printf("当前已选中文件“%s”，是否选择其他文件？\n", tracker->tracked_file);
        printf("1--是\n");
        i = choice_check();
        if (i != 1) {
            printf("已取消切换文件\n");
            system("pause");
            return;
        }
        tracker_free(tracker);
        tracker_init(tracker);
        system("cls");
        printf("仓库打开和程序初始化模块\n");
        printf("---------------------------------\n");
    }
    printf("请输入要打开的文件路径（含空格请用双引号括起来）：");
    if (fgets(path, MAX_PATH_LEN, stdin) == NULL) {
        printf("读取输入失败！\n");
        system("pause");
        return;
    }
    len = strlen(path);
    if (len > 0 && path[len - 1] == '\n') {
        path[len - 1] = '\0';
    } else {
        while ((c = getchar()) != '\n' && c != EOF);
    }
    printf("---------------------------------\n");
    if (path_analysis(path, tracked_file, working_dir) != 1) {
        printf("错误：文件路径解析失败！请检查是否使用绝对路径！文件未被打开！\n");
        system("pause");
        return;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        printf("错误：文件打开失败！请检查文件路径和权限！文件未被打开！\n");
        system("pause");
        return;
    }
    fclose(fp);
    printf("文件所在目录：%s，文件名：%s\n", working_dir, tracked_file);
    printf("是否确认打开？");
    printf("1--是\n");
    i = choice_check();
    if (i != 1) {
        printf("已取消打开文件\n");
        system("pause");
        return;
    }
    strcpy(tracker->tracked_file, tracked_file);
    strcpy(tracker->working_dir, working_dir);
    repo_path_len = snprintf(tracker->repo_path, REPO_PATH_LEN, "%s/.ft/repos/%s",tracker->working_dir, tracked_file);
    if (repo_path_len < 0 || repo_path_len >= REPO_PATH_LEN) {
        printf("错误：仓库路径过长。请缩短文件路径后重试。\n");
        tracker->tracked_file[0] = '\0';
        tracker->working_dir[0] = '\0';
        tracker->repo_path[0] = '\0';
        system("pause");
        return;
    }

    if (storage_repository_exists(tracker)) {
        if (storage_load_tracker(tracker)) {
            storage_ft_write_index(tracker->working_dir, tracker->tracked_file, path);
            printf("已打开现有仓库\n");
            system("pause");
            return;
        }
        printf("错误：仓库数据损坏或不完整，已停止打开且未覆盖原始数据。\n");
        tracker_free(tracker);
        tracker_init(tracker);
        system("pause");
        return;
    }
    if (storage_init_repository(tracker)) {
        storage_ft_write_index(tracker->working_dir, tracker->tracked_file, path);
        printf("已新建工作仓库\n");
        system("pause");
        return;
    }
    printf("错误：新建工作目录失败！请检查目录权限！文件未被打开！\n");
    strcpy(tracker->tracked_file, "");
    strcpy(tracker->working_dir, "");
    strcpy(tracker->repo_path, "");
    system("pause");
}

static void do_commit(Tracker *tracker, const char *message,unsigned char *data, const long size, const char *hash) {
    char full_path[MAX_PATH_LEN];
    Commit *commit;
    BlobRef *blob_ref;

    snprintf(full_path, sizeof(full_path), "%s/%s",
             tracker->working_dir, tracker->tracked_file);

    blob_ref = tracker->blob_refs;
    while (blob_ref != NULL) {
        if (strcmp(blob_ref->hash, hash) == 0) break;
        blob_ref = blob_ref->next;
    }
    if (blob_ref != NULL) {
        blob_ref->ref_count++;
    } else {
        blob_ref = (BlobRef *)malloc(sizeof(BlobRef));
        if (blob_ref == NULL) { free(data); return; }
        strncpy(blob_ref->hash, hash, HASH_LEN - 1);
        blob_ref->hash[HASH_LEN - 1] = '\0';
        blob_ref->ref_count = 1;
        blob_ref->next = tracker->blob_refs;
        if (!storage_save_blob(tracker, hash, data, size)) {
            free(blob_ref);
            free(data);
            printf("错误：提交失败，已放弃提交！请检查写入权限！\n");
            return;
        }
        tracker->blob_refs = blob_ref;
    }

    commit = (Commit *)malloc(sizeof(Commit));
    if (commit == NULL) { free(data); return; }

    commit->id = tracker->next_commit_id;
    tracker->next_commit_id++;
    strncpy(commit->file_path, full_path, MAX_PATH_LEN - 1);
    commit->file_path[MAX_PATH_LEN - 1] = '\0';
    strncpy(commit->message, message, MAX_MESSAGE_LEN - 1);
    commit->message[MAX_MESSAGE_LEN - 1] = '\0';
    strncpy(commit->hash, hash, HASH_LEN - 1);
    commit->hash[HASH_LEN - 1] = '\0';
    strncpy(commit->branch, tracker->current_branch ? tracker->current_branch->name : "main", BRANCH_NAME_LEN - 1);
    commit->branch[BRANCH_NAME_LEN - 1] = '\0';

    commit->prev = tracker->current_branch->head;
    commit->next = NULL;
    commit->timestamp = time(NULL);
    tracker->current_branch->head = commit;
    tracker->checked_out = commit;

    if (tracker->total_commit_head != NULL)
        tracker->total_commit_head->next = commit;
    tracker->total_commit_head = commit;
    if (tracker->commit_tail == NULL)
        tracker->commit_tail = commit;

    storage_save_tracker(tracker);
    printf("提交成功，版本 ID：%d\n", commit->id);
    free(data);
}

void commit_version(Tracker *tracker) {
    unsigned char *data = NULL;
    long size = 0;
    char hash[HASH_LEN];
    char full_path[MAX_PATH_LEN];
    char message[MAX_MESSAGE_LEN] = "";


    system("cls");
    printf("文件提交模块\n");
    printf("---------------------------------\n");
    if (tracker->repository_loaded == 0) {
        printf("请先打开/创建文件仓库（选择1）\n");
        system("pause");
        return;
    }
    if (tracker->checked_out!=tracker->current_branch->head) {
        printf("警告：当前未处于提交头，如需在此版本上提交，请先新建分支！\n");
        system("pause");
        return;
    }

    snprintf(full_path, sizeof(full_path), "%s/%s",
             tracker->working_dir, tracker->tracked_file);

    if (!storage_read_file(full_path, &data, &size)) {
        printf("无法读取文件：%s\n", full_path);
        system("pause");
        return;
    }

    hash_data(data, size, hash);

    printf("当前文件完整哈希值：%s\n", hash);

    printf("---------------------------------\n");
    printf("是否确认提交？\n");
    printf("1-是  0-取消\n");
    if (choice_check() != 1) {
        printf("已取消提交\n");
        free(data);
        system("pause");
        return;
    }

    /* 重复提交检查 */
    if (tracker->current_branch->head != NULL &&
        strcmp(tracker->current_branch->head->hash, hash) == 0) {
        printf("文件内容与上次提交未变化\n");
        }

    printf("请输入提交说明：");
    if (fgets(message, MAX_MESSAGE_LEN, stdin) != NULL) {
        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';
        }
    }
    if (!storage_text_is_safe(message)) {
        printf("错误：提交说明不能包含字符 |。\n");
        free(data);
        system("pause");
        return;
    }

    printf("---------------------------------\n");
    printf("拟提交ID:%d\n",tracker->next_commit_id);
    printf("提交文件哈希：%s\n",static_short_hash(hash));
    printf("提交说明：%s\n",message);
    printf("---------------------------------\n");
    printf("是否确认提交？\n");
    printf("1-是  0-取消\n");
    if (choice_check() != 1) {
        printf("已取消提交\n");
        free(data);
        system("pause");
        return;
    }
    do_commit(tracker, message, data, size, hash);
    system("pause");
}

void show_history(const Tracker *tracker) {
    system("cls");
    const Commit *commit;
    const Commit *arr[MAX_COMMITS];
    int count = 0, i, choice;
    char mark[15]="";

    printf("提交记录查看模块\n");
    printf("---------------------------------\n");
    if (tracker->repository_loaded == 0) {
        printf("请先打开/创建文件仓库（选择1）\n");
        system("pause");
        return;
    }
    printf("1. 当前分支（%s）\n", tracker->current_branch->name);
    printf("2. 全部提交\n");
    printf("0. 返回\n");
    choice = choice_check();

    if (choice == 0) return;
    system("cls");

    if (choice == 1) {
        commit = tracker->current_branch->head;
        while (commit != NULL && count < MAX_COMMITS) {
            arr[count++] = commit;
            commit = commit->prev;
        }
        printf("提交记录查看模块\n");
        printf("---------------------------------\n");
        printf("当前分支：%s\n", tracker->current_branch->name);
    } else {
        commit = tracker->commit_tail;
        while (commit != NULL && count < MAX_COMMITS) {
            arr[count++] = commit;
            commit = commit->next;
        }
        printf("提交记录\n");
        printf("---------------------------------\n");
        printf("所有分支\n");
    }

    if (count == 0) {
        printf("暂无历史版本\n");
        system("pause");
        return;
    }

    printf("---------------------------------\n");
    for (i = count - 1; i >= 0; i--) {
        commit = arr[i];
        char time_buf[20];
        struct tm *tm_info = localtime(&commit->timestamp);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M", tm_info);
        if (commit==tracker->checked_out) strcpy(mark,"（当前） ");
        else strcpy(mark,"");
        printf("版本 %d [%s] %s %s%s\n  %s\n",commit->id, commit->branch,time_buf, static_short_hash(commit->hash),mark, commit->message);
        if (choice == 1 && i > 0 && strcmp(arr[i]->branch, arr[i-1]->branch) != 0) printf("  ---- 分支 [%s] 从此处分出 ----\n", arr[i-1]->branch);
    }
    printf("---------------------------------\n");
    system("pause");
}

void delete_repository(Tracker *tracker) {
    system("cls");
    printf("仓库清空模块\n");
    printf("---------------------------------\n");
    if (tracker->repository_loaded == 0) {
        printf("请先打开/创建文件仓库（选择1）\n");
        system("pause");
        return;
    }

    printf("确定要删除当前仓库吗？\n");
    printf("（将删除 .ft 目录及其所有数据）\n");
    printf("1-确认删除  0-取消\n");
    if (choice_check() != 1) {
        printf("已取消\n");
        system("pause");
        return;
    }

    char dir[MAX_PATH_LEN];
    strcpy(dir, tracker->working_dir);
    tracker_free(tracker);
    tracker_init(tracker);

    char cmd[MAX_PATH_LEN + 32];
    snprintf(cmd, sizeof(cmd), "rmdir /s /q \"%s/.ft\"", dir);
    system(cmd);

    printf("仓库已删除\n");
    system("pause");
}

void restore_version(Tracker *tracker) {
    int target_id;
    Commit *c;
    char full_path[MAX_PATH_LEN];
    char time_buf[20];
    struct tm *tm_info;
    unsigned char *cur_data = NULL;
    long cur_size = 0;
    char cur_hash[HASH_LEN];

    system("cls");
    printf("提交版本恢复模块\n");
    printf("---------------------------------\n");
    if (tracker->repository_loaded == 0) {
        printf("请先打开/创建文件仓库（选择1）\n");
        system("pause");
        return;
    }
    if (tracker->current_branch == NULL||tracker->current_branch->head == NULL) {
        printf("当前分支下没有可恢复的历史版本\n");
        system("pause");
        return;
    }

    printf("在下一个输入框中输入要恢复的版本 ID\n");
    target_id = choice_check();
    if (target_id == 0) {
        printf("已取消恢复版本！\n");
        system("pause");
        return;
    }

    c = tracker->current_branch->head;
    while (c) {
        if (c->id == target_id) break;
        c = c->prev;
    }

    if (c == NULL) {
        printf("未找到版本 ID：%d\n", target_id);
        system("pause");
        return;
    }

    tm_info = localtime(&c->timestamp);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M", tm_info);
    printf("版本信息\n");
    printf("---------------------------------\n");
    printf("ID：%d\n", c->id);
    printf("分支：%s\n", c->branch);
    printf("时间：%s\n", time_buf);
    printf("哈希：%s\n", c->hash);
    printf("说明：%s\n", c->message);
    printf("---------------------------------\n");
    printf("是否确认恢复到该版本？\n");
    printf("1-是  0-取消\n");
    if (choice_check() != 1) {
        printf("已取消恢复\n");
        system("pause");
        return;
    }

    snprintf(full_path, sizeof(full_path), "%s/%s",tracker->working_dir, tracker->tracked_file);

    if (tracker->checked_out != NULL) {
        if (storage_read_file(full_path, &cur_data, &cur_size)) {
            hash_data(cur_data, cur_size, cur_hash);
            free(cur_data);

            if (strcmp(cur_hash, tracker->checked_out->hash) != 0) {
                printf("警告：当前文件有未提交的修改！\n");
                printf("恢复版本 %d 将丢失这些修改。\n", target_id);
                printf("是否仍然继续？\n");
                printf("1-是  0-取消\n");
                if (choice_check() != 1) {
                    printf("已取消恢复\n");
                    system("pause");
                    return;
                }
            }
        }
    }

    if (!storage_restore_blob(tracker, c->hash, full_path)) {
        printf("恢复文件失败\n");
        system("pause");
        return;
    }
    tracker->checked_out = (Commit *)c;
    storage_save_head(tracker);

    printf("版本 %d 已恢复到 %s\n", target_id, full_path);
    system("pause");
}

void watch_files(Tracker *tracker) {
    int elapsed;
    int changed = 0;
    struct stat st;
    char full_path[MAX_PATH_LEN];
    char last_hash[HASH_LEN] = "";
    char new_hash[HASH_LEN];
    unsigned char *data = NULL;
    long size = 0;
    time_t last_mtime = 0;

    system("cls");
    printf("文件监控模块\n");
    printf("---------------------------------\n");
    if (tracker->repository_loaded == 0) {
        printf("请先打开/创建文件仓库（选择1）\n");
        system("pause");
        return;
    }
    if (tracker->checked_out!=tracker->current_branch->head) {
        printf("警告：当前未处于提交头，如需在此版本上提交，请先新建分支！\n");
        system("pause");
        return;
    }

    snprintf(full_path, sizeof(full_path), "%s/%s",
             tracker->working_dir, tracker->tracked_file);

    if (storage_read_file(full_path, &data, &size)) {
        hash_data(data, size, last_hash);
        stat(full_path, &st);
        last_mtime = st.st_mtime;
        free(data);
        data = NULL;
    }

    printf("监控文件：%s\n", full_path);
    printf("每 %d 秒检查一次修改时间，变化时自动提交\n",MONITOR_TIMES/1000);
    printf("按 0 退出监控\n");
    printf("---------------------------------\n");

    while (1) {
        for (elapsed = 0; elapsed < MONITOR_TIMES; elapsed += 100) {
            Sleep(100);
            if (_kbhit() && _getch() == '0') goto exit_monitor;
        }

        if (stat(full_path, &st) != 0) continue;
        if (st.st_mtime == last_mtime) continue;
        last_mtime = st.st_mtime;

        if (!storage_read_file(full_path, &data, &size)) continue;
        hash_data(data, size, new_hash);

        if (strcmp(new_hash, last_hash) != 0) {
            changed = 1;
            printf("\n检测到文件变化，正在自动提交...\n");
            strcpy(last_hash, new_hash);
            do_commit(tracker, "自动监控提交", data, size, new_hash);
            data = NULL;
        } else {
            free(data);
            data = NULL;
        }
    }
    exit_monitor:
    if (changed == 1) printf("---------------------------------\n");
    printf("已退出监控进程\n");
    system("pause");
}

void manage_branches(Tracker *tracker) {
    int choice;
    char name[BRANCH_NAME_LEN];
    Branch *b, *prev;
    int idx, i, found;

    system("cls");
    printf("分支管理模块\n");
    printf("---------------------------------\n");
    if (tracker->repository_loaded == 0) {
        printf("请先打开/创建文件仓库（选择1）\n");
        system("pause");
        return;
    }
    while (1) {
        system("cls");
        printf("分支管理模块\n");
        printf("---------------------------------\n");
        printf("当前分支：%s\n", tracker->current_branch->name);
        printf("所有分支：\n");
        b = tracker->branches;
        i = 1;
        while (b) {
            printf("  %d. %s\n", i++, b->name);
            b = b->next;
        }
        printf("---------------------------------\n");
        printf("1. 创建分支\n");
        printf("2. 切换分支\n");
        printf("3. 删除分支\n");
        printf("0. 返回\n");
        choice = choice_check();

        if (choice == 1) {
            printf("请输入新分支名称：");
            if (fgets(name, BRANCH_NAME_LEN, stdin) == NULL) continue;
            {
                size_t len = strlen(name);
                if (len > 0 && name[len - 1] == '\n') name[len - 1] = '\0';
            }
            if (name[0] == '\0') continue;
            if (!storage_text_is_safe(name)) {
                printf("错误：分支名称不能包含字符 |。\n");
                system("pause");
                continue;
            }

            b = tracker->branches;
            found = 0;
            while (b) {
                if (strcmp(b->name, name) == 0) {
                    found = 1;
                    break;
                }
                b = b->next;
            }
            if (found) {
                printf("分支 '%s' 已存在\n", name);
                system("pause");
                continue;
            }

            b = (Branch *) malloc(sizeof(Branch));
            if (!b) {
                printf("错误：内存申请失败！\n");
                system("pause");
                continue;
            }
            strncpy(b->name, name, BRANCH_NAME_LEN - 1);
            b->name[BRANCH_NAME_LEN - 1] = '\0';
            b->head = tracker->checked_out;
            b->next = NULL;
            Branch *last = tracker->branches;
            while (last->next != NULL) last = last->next;
            last->next = b;
            storage_save_branches(tracker);
            printf("分支 '%s' 创建成功\n", name);
            system("pause");
        } else if (choice == 2) {
            int total = 0;
            b = tracker->branches;
            while (b) {
                total++;
                b = b->next;
            }
            if (total == 0) {
                printf("没有分支可切换\n");
                system("pause");
                continue;
            }

            printf("请选择要切换到的分支（1-%d）：", total);
            idx = 0;
            if (scanf("%d", &idx) != 1 || idx < 1 || idx > total) {
                while (getchar() != '\n') {
                }
                printf("选择无效\n");
                system("pause");
                continue;
            }
            while (getchar() != '\n') {
            }
            b = tracker->branches;
            i = 1;
            found = 0;
            while (b) {
                if (i == idx) {
                    found = 1;
                    break;
                }
                i++;
                b = b->next;
            }
            if (found) {
                if (b == tracker->current_branch) {
                    printf("已在分支 '%s'\n", b->name);
                } else if (b->head != NULL) {
                    int do_switch = 1;

                    if (tracker->checked_out != NULL) {
                        char full_path[MAX_PATH_LEN];
                        unsigned char *data = NULL;
                        long size = 0;
                        char hash[HASH_LEN];

                        snprintf(full_path, sizeof(full_path), "%s/%s",
                                 tracker->working_dir, tracker->tracked_file);
                        if (storage_read_file(full_path, &data, &size)) {
                            hash_data(data, size, hash);
                            free(data);
                            if (strcmp(hash, tracker->checked_out->hash) != 0) {
                                printf("警告：当前文件有未提交的修改\n");
                                printf("切换到分支 '%s' 将丢弃这些修改\n", b->name);
                                printf("是否继续？ 1-是  0-否\n");
                                do_switch = (choice_check() == 1);
                            }
                        }
                    }

                    if (do_switch) {
                        char full_path[MAX_PATH_LEN];
                        snprintf(full_path, sizeof(full_path), "%s/%s",
                                 tracker->working_dir, tracker->tracked_file);
                        if (storage_restore_blob(tracker, b->head->hash, full_path)) {
                            tracker->current_branch = b;
                            tracker->checked_out = b->head;
                            printf("已切换到分支 '%s'（文件已恢复到 ID%d）\n",
                                   b->name, b->head->id);
                        } else {
                            printf("未能切换到分支 '%s'（警告：文件恢复失败）\n", b->name);
                        }
                    } else {
                        printf("已取消切换\n");
                    }
                } else {
                    tracker->current_branch = b;
                    tracker->checked_out = NULL;
                    printf("已切换到分支 '%s'（该分支尚无提交）\n", b->name);
                }
            } else {
                printf("选择无效\n");
            }
            storage_save_head(tracker);
            system("pause");
        } else if (choice == 3) {
            int total = 0;
            b = tracker->branches;
            while (b) {
                total++;
                b = b->next;
            }
            if (total <= 1) {
                printf("至少保留一个分支\n");
                system("pause");
                continue;
            }

            printf("请输入要删除的分支编号：");
            scanf("%d", &idx);
            while (getchar() != '\n') {
            }
            b = tracker->branches;
            prev = NULL;
            i = 1;
            found = 0;
            while (b) {
                if (i == idx) {
                    if (strcmp(b->name, "main") == 0) printf("不能删除 main 分支\n");
                    else if (b == tracker->current_branch) printf("不能删除当前分支\n");
                    else {
                        if (prev) prev->next = b->next;
                        else tracker->branches = b->next;
                        free(b);
                        storage_save_branches(tracker);
                        printf("分支已删除\n");
                    }
                    found = 1;
                    break;
                }
                prev = b;
                b = b->next;
                i++;
            }
            if (!found) printf("选择无效\n");
            system("pause");
        } else if (choice == 0) {
            return;
        }
    }
}

void manage_tags(Tracker *tracker) {
    int choice;
    char name[TAG_NAME_LEN];
    int commit_id, i, idx, found, total;
    Tag *t, *prev, *tail;
    Commit *c;

    system("cls");
    printf("标签管理模块\n");
    printf("---------------------------------\n");
    if (tracker->repository_loaded == 0) {
        printf("请先打开/创建文件仓库（选择1）\n");
        system("pause");
        return;
    }
    while (1) {
        system("cls");
        printf("标签管理模块\n");
        printf("---------------------------------\n");
        printf("所有标签：\n");
        t = tracker->tags;
        i = 1;
        while (t) {
            if (t->commit==NULL) printf("  %d. %s -> 对应版本丢失，将在下一次SL时移除！\n", i++, t->name);
            else printf("  %d. %s -> 版本 %d [%s] %s\n", i++, t->name, t->commit->id,static_short_hash(t->commit->hash), t->commit->message);
            t = t->next;
        }
        printf("---------------------------------\n");
        printf("1. 创建标签\n");
        printf("2. 删除标签\n");
        printf("0. 返回\n");
        choice = choice_check();

        if (choice == 1) {
            printf("请输入标签名称：");
            if (fgets(name, TAG_NAME_LEN, stdin) == NULL) continue;
            {
                size_t len = strlen(name);
                if (len > 0 && name[len - 1] == '\n') name[len - 1] = '\0';
            }
            if (name[0] == '\0') continue;
            if (!storage_text_is_safe(name)) {
                printf("错误：标签名称不能包含字符 |。\n");
                system("pause");
                continue;
            }
            printf("---------------------------------\n");
            t = tracker->tags;
            found = 0;
            while (t) {
                if (strcmp(t->name, name) == 0) {
                    found = 1;
                    break;
                }
                t = t->next;
            }
            if (found) {
                printf("标签 '%s' 已存在\n", name);
                system("pause");
                continue;
            }

            printf("请在下一个输入框中输入要标记的版本 ID\n");
            commit_id = choice_check();
            c = tracker->commit_tail;
            found = 0;
            while (c) {
                if (c->id == commit_id) {
                    found = 1;
                    break;
                }
                c = c->next;
            }
            if (!found) {
                printf("未找到版本 ID：%d\n", commit_id);
                system("pause");
                continue;
            }

            t = (Tag *)malloc(sizeof(Tag));
            if (!t) {
                printf("内存分配失败\n");
                system("pause");
                continue;
            }
            strncpy(t->name, name, TAG_NAME_LEN - 1);
            t->name[TAG_NAME_LEN - 1] = '\0';
            t->commit = c;
            t->next = NULL;

            if (tracker->tags == NULL) {
                tracker->tags = t;
            } else {
                tail = tracker->tags;
                while (tail->next != NULL) tail = tail->next;
                tail->next = t;
            }
            storage_save_tags(tracker);
            printf("标签 '%s' 创建成功\n", name);
            system("pause");

        } else if (choice == 2) {
            total = 0;
            t = tracker->tags;
            while (t) {
                total++;
                t = t->next;
            }
            if (total == 0) {
                printf("没有标签可删除\n");
                system("pause");
                continue;
            }

            printf("请输入要删除的标签编号（1-%d）：", total);
            scanf("%d", &idx);
            while (getchar() != '\n') {
            }
            if (idx < 1 || idx > total) {
                printf("编号无效\n");
                system("pause");
                continue;
            }

            t = tracker->tags;
            prev = NULL;
            i = 1;
            while (t) {
                if (i == idx) {
                    if (prev) prev->next = t->next;
                    else tracker->tags = t->next;
                    free(t);
                    printf("标签已删除\n");
                    storage_save_tags(tracker);
                    break;
                }
                prev = t;
                t = t->next;
                i++;
            }
            system("pause");

        } else if (choice == 0) {
            return;
        }
    }
}

void show_graph(const Tracker *tracker) {
    const Commit *arr[MAX_COMMITS];
    int count = 0;
    const Branch *b;
    int lane_count = 0;
    char mark[15]="";

    system("cls");
    printf("提交图展示模块\n");
    printf("---------------------------------\n");
    if (tracker->repository_loaded == 0) {
        printf("请先打开/创建文件仓库（选择1）\n");
        system("pause");
        return;
    }
    if (tracker->commit_tail == NULL) {
        printf("无提交图\n");
        system("pause");
        return;
    }

    b = tracker->branches;
    while (b) { lane_count++; b = b->next; }
    if (lane_count > 8) lane_count = 8;

    /* 收集全部提交（旧→新） */
    const Commit *c = tracker->commit_tail;
    while (c && count < MAX_COMMITS) {
        arr[count++] = c;
        c = c->next;
    }

    printf("---------------------------------\n");

    /* 分支表头 */
    b = tracker->branches;
    int i = 0;
    while (b && i < lane_count) {
        printf(" %s", b->name);
        int pad = 8 - (int)strlen(b->name);
        if (pad < 1) pad = 1;
        for (int j = 0; j < pad; j++) printf(" ");
        b = b->next;
        i++;
    }
    printf("\n");

    for (i = 0; i < lane_count; i++) printf("----");
    printf("\n");

    /* 新→旧遍历 */
    for (i = count - 1; i >= 0; i--) {
        c = arr[i];

        /* lane 定位 */
        int col = 0;
        b = tracker->branches;
        int found = 0;
        while (b && col < lane_count) {
            if (strcmp(c->branch, b->name) == 0) {
                found = 1;
                printf(" *  ");
                for (int j = col + 1; j < lane_count; j++)
                    printf(" |  ");
                break;
            }
            printf(" |  ");
            col++;
            b = b->next;
        }
        if (!found) {
            for (int j = 0; j < lane_count; j++) printf(" |  ");
        }

        /* 时间戳 */
        char time_buf[20];
        struct tm *tm_info = localtime(&c->timestamp);
        strftime(time_buf, sizeof(time_buf), "%m-%d %H:%M", tm_info);
        printf(" [%s]", time_buf);

        /* 分叉标记：和全局下一笔（更旧）不同分支 */
        if (i > 0 && strcmp(c->branch, arr[i-1]->branch) != 0)
            printf(" <");

        if (c==tracker->checked_out) strcpy(mark," <-当前");
        else strcpy(mark,"");
        printf(" 版本 %d: %s%s", c->id, c->message, mark);

        /* 标签 */
        const Tag *t = tracker->tags;
        while (t) {
            if (t->commit == c) {
                printf("  (%s)", t->name);
            }
            t = t->next;
        }

        printf("\n");
    }
    printf("---------------------------------\n");
    system("pause");
}
