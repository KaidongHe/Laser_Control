# GitHub 推送流程说明

本文档用于说明如何把当前 `serialhelper` 工程作为独立 Git 仓库推送到 GitHub，并解释每一步命令的作用。

## 1. 进入工程目录

```powershell
cd C:\Users\Antis\Desktop\DAT\QT_laser\laser-control\serialhelper
```

作用：

- 确保后续 `git init`、`git add`、`git commit`、`git push` 都作用在当前 Qt 工程目录。
- 由于上级目录 `QT_laser` 也可能是 Git 仓库，必须进入 `serialhelper` 后再操作，避免提交到上级仓库。

## 2. 初始化本地 Git 仓库

```powershell
git init
```

作用：

- 在当前目录创建 `.git` 文件夹。
- 当前 `serialhelper` 工程会成为一个独立 Git 仓库。

检查当前状态：

```powershell
git status
```

作用：

- 查看哪些文件还没有加入 Git。
- 查看当前是否已经有提交、是否有未保存的改动。

## 3. 准备 .gitignore

建议在 `serialhelper` 目录下创建 `.gitignore`，避免把编译产物、本机配置、临时文件上传到 GitHub。

推荐内容：

```gitignore
debug/
release/
build*/
*.user
*.pro.user
*.exe
*.obj
*.o
*.dll
*.pdb
*.ilk
Makefile
.qmake.stash
```

如果 `laser_config.ini` 是本机运行参数，不希望上传到仓库，可以加入：

```gitignore
laser_config.ini
```

如果需要给别人参考配置，可以保留一个示例文件：

```powershell
copy laser_config.ini laser_config.example.ini
```

## 4. 添加文件到暂存区

```powershell
git add .
```

作用：

- 把当前目录下所有未跟踪文件和修改文件加入 Git 暂存区。
- 暂存区里的内容会进入下一次 commit。

如果只想添加指定文件，可以使用：

```powershell
git add README.md
git add operatorform.cpp operatorform.h operatorform.ui
```

## 5. 创建本地提交

```powershell
git commit -m "initial commit"
```

作用：

- 把暂存区内容保存为一次本地提交。
- `-m` 后面是提交说明。

后续修改时，提交说明建议写清楚改了什么，例如：

```powershell
git commit -m "add operator serial connection ui"
```

## 6. 连接 GitHub 远程仓库

先在 GitHub 网页中新建仓库，例如：

```text
https://github.com/KaidongHe/Laser_Control.git
```

然后在本地执行：

```powershell
git remote add origin https://github.com/KaidongHe/Laser_Control.git
```

作用：

- 给当前本地仓库添加一个远程地址。
- `origin` 是远程仓库的默认名字。

检查远程地址：

```powershell
git remote -v
```

## 7. 设置主分支名

```powershell
git branch -M main
```

作用：

- 把当前分支改名为 `main`。
- GitHub 新仓库默认主分支一般也是 `main`，这样本地和远程保持一致。

查看当前分支：

```powershell
git branch --show-current
```

## 8. 首次推送到 GitHub

```powershell
git push -u origin main
```

作用：

- 把本地 `main` 分支推送到 GitHub 的 `origin/main`。
- `-u` 会建立本地分支和远程分支的跟踪关系。
- 建立后，后续可以直接执行 `git push`。

## 9. 远程仓库已有 README 时的处理

如果推送时出现：

```text
! [rejected] main -> main (non-fast-forward)
error: failed to push some refs
```

说明：

- GitHub 远程仓库里已经有提交。
- 常见原因是在 GitHub 创建仓库时勾选了 `README`、`.gitignore` 或 `LICENSE`。
- 本地提交历史和远程提交历史不是同一条线，所以 Git 拒绝直接覆盖。

推荐处理方式：

```powershell
git pull origin main --allow-unrelated-histories
```

作用：

- 把远程已有提交拉到本地。
- `--allow-unrelated-histories` 允许合并两个原本互不相关的仓库历史。

如果没有冲突，继续推送：

```powershell
git push -u origin main
```

如果出现冲突：

1. 打开冲突文件，保留正确内容。
2. 删除冲突标记：

```text
<<<<<<< HEAD
=======
>>>>>>> origin/main
```

3. 重新添加并提交：

```powershell
git add .
git commit -m "merge remote initial files"
git push -u origin main
```

## 10. 如果确定远程内容不要了

如果远程仓库只是自动生成的 README，而你想用本地工程完全覆盖远程，可以使用：

```powershell
git push -u origin main --force-with-lease
```

作用：

- 用本地 `main` 覆盖远程 `main`。
- `--force-with-lease` 比 `--force` 稍安全，它会在远程分支没有被别人继续更新时才覆盖。

注意：

- 这会让远程已有提交从分支历史中消失。
- 如果远程内容有价值，优先使用 `git pull origin main --allow-unrelated-histories`。

## 11. 后续日常更新流程

每次修改代码后，按下面流程提交并推送：

```powershell
git status
git add .
git commit -m "说明本次修改内容"
git push
```

各命令作用：

| 命令 | 作用 |
|---|---|
| `git status` | 查看当前有哪些文件被修改、未跟踪或已暂存 |
| `git add .` | 把当前目录所有修改加入暂存区 |
| `git commit -m "..."` | 创建一次本地提交 |
| `git push` | 推送到已经绑定的 GitHub 远程分支 |

## 12. 常见问题

### 12.1 remote origin already exists

报错：

```text
error: remote origin already exists.
```

说明：

- 当前仓库已经配置过 `origin`。

查看已有远程：

```powershell
git remote -v
```

如果地址不对，可以修改：

```powershell
git remote set-url origin https://github.com/KaidongHe/Laser_Control.git
```

### 12.2 Authentication failed

说明：

- GitHub HTTPS 推送不再支持直接用账号密码。
- 需要使用 Personal Access Token，或者配置 SSH Key。

HTTPS 推送时：

- 用户名填 GitHub 用户名。
- 密码位置填 GitHub Token。

### 12.3 dubious ownership

报错：

```text
fatal: detected dubious ownership in repository
```

说明：

- 当前执行 Git 的 Windows 用户和仓库目录所有者不同。
- 常见于沙箱、管理员权限、不同账号运行终端。

如果确认目录可信，可以执行：

```powershell
git config --global --add safe.directory C:/Users/Antis/Desktop/DAT/QT_laser/laser-control/serialhelper
```

### 12.4 上级仓库和当前工程仓库嵌套

当前工程路径：

```text
C:\Users\Antis\Desktop\DAT\QT_laser\laser-control\serialhelper
```

如果上级 `QT_laser` 也是 Git 仓库，那么 `serialhelper` 会成为嵌套仓库。

建议：

- 日常提交 `serialhelper` 时，只在 `serialhelper` 目录下执行 Git 命令。
- 上级仓库不要再把 `laser-control/serialhelper` 当普通文件夹提交。
- 可以在上级仓库的 `.gitignore` 中加入：

```gitignore
laser-control/serialhelper/
```

## 13. 推荐完整命令示例

适用于本地已有代码、GitHub 新建仓库为空的情况：

```powershell
cd C:\Users\Antis\Desktop\DAT\QT_laser\laser-control\serialhelper
git init
git add .
git commit -m "initial commit"
git branch -M main
git remote add origin https://github.com/KaidongHe/Laser_Control.git
git push -u origin main
```

适用于远程已有 README 导致推送被拒绝的情况：

```powershell
cd C:\Users\Antis\Desktop\DAT\QT_laser\laser-control\serialhelper
git pull origin main --allow-unrelated-histories
git push -u origin main
```
