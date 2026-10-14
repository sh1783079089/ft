#include <stdio.h>
#include <stdlib.h>

#include "tracker.h"

void show_menu(const Tracker* tracker) {
    system("cls");
    printf("====================================\n");
    printf(" 文件追踪与历史版本恢复系统\n");
    printf("====================================\n");
    if (tracker->repository_loaded) {
        printf(" 仓库：%s\\%s\n", tracker->working_dir, tracker->tracked_file);
        if (tracker->current_branch != NULL) {
            printf(" 分支：%s", tracker->current_branch->name);
            if (tracker->checked_out && tracker->current_branch->head &&
                tracker->checked_out != tracker->current_branch->head) {
                printf("  [当前: v%d, 最新: v%d]", tracker->checked_out->id,
                       tracker->current_branch->head->id);
            } else if (tracker->checked_out) {
                printf("  [v%d]", tracker->checked_out->id);
            }
            printf("\n");
        }
    } else
        printf(" 仓库：未打开\n");
    printf("====================================\n");
    printf("1. 打开/创建文件仓库\n");
    printf("2. 提交当前版本\n");
    printf("3. 查看历史版本\n");
    printf("4. 恢复指定版本\n");
    printf("5. 监控文件变化\n");
    printf("6. 分支管理\n");
    printf("7. 标签管理\n");
    printf("8. 查看提交图\n");
    printf("9. 删除/重置仓库\n");
    printf("0. 退出程序\n");
}

void menu(Tracker* tracker) {
    int choice = -1;
    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        choice = -1;
        show_menu(tracker);
        choice = choice_check();

        switch (choice) {
            case 1:
                open_or_create_repository(tracker);  // 打开文件，初始化tracker
                break;
            case 2:
                commit_version(tracker);  // 手动提交
                break;
            case 3:
                show_history(tracker);  // 展示提交历史
                break;
            case 4:
                restore_version(tracker);  // 恢复到某一版本
                break;
            case 5:
                watch_files(tracker);  // 监控文件变更
                break;
            case 6:
                manage_branches(tracker);  // 管理分支
                break;
            case 7:
                manage_tags(tracker);  // 管理标签
                break;
            case 8:
                show_graph(tracker);  // 展示提交树
                break;
            case 9:
                delete_repository(tracker);
                break;
            case 0:
                save_exit(tracker);
                break;
            default:
                printf("没有这个选项，请重新输入。\n");
                system("pause");
                break;
        }
    }
}

int main(void) {  // 程序主进程，创建tracker对象存储追踪文件信息
    Tracker tracker;
    tracker_init(&tracker);
    system("chcp 65001");
    menu(&tracker);
    tracker_free(&tracker);
    return 0;
}
