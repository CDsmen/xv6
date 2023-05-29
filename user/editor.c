#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

// 全局定义区

#define MAX_LINE_NUM 1024 // 最大行数
#define MAX_LINE_LEN 512  // 每行最大长度
#define BUF_SIZE 4096	  // buf数组大小

// 函数声明区

// 确保用到的行已分配
void alloc_line();
// 显示文本
void show();
// 显示帮助
void help();
// 插入命令
void ins(int n);
// 修改命令
void mod(int n);
// 删除命令
void del(int n);
// 保存命令
void save();
// 退出命令
void quit();
// 字符串拼接
void strncat(char *p, const char *q, int n);

// 全局变量区

int alloc_line_num = 0;	   // 已经分配的行数
int now_line_num = 0;	   // 当前文件行数
int changed = 0;		   // 相对上次保存是否有更改
int need_show = 1;		   // 是否需要在命令后显示文档
char *lines[MAX_LINE_NUM]; // 行指针数组
char *file_name;		   // 文件名
char buf[BUF_SIZE];		   // 缓存数组，用于读入文件

int main(int argc, char *argv[])
{
	// 命令参数不对
	if (argc != 2)
	{
		fprintf(2, "editor command is [editor file_name]\n");
		exit(1);
	}

	file_name = argv[1];
	int fd; // 文件描述符
	// 文件打开失败
	if ((fd = open(argv[1], O_RDONLY)) < 0)
	{
		fprintf(2, "cannot open %s\n", argv[1]);
		exit(1);
	}

	int len = 0;
	while ((len = read(fd, buf, BUF_SIZE)) > 0)
	{
		int i = 0;
		int last = 0;
		alloc_line();
		while (i < len)
		{
			for (i = last; i < len && buf[i] != '\n'; i++)
				;
			strncat(lines[now_line_num], buf + last, i - last);

			// 如果是换行就另起一行
			if (i < len && buf[i] == '\n')
			{
				// 文件过大，当前行数不足以存下
				if (now_line_num == MAX_LINE_NUM - 1)
				{
					fprintf(2, "file is to large\n");
					exit(1);
				}
				now_line_num++;
				alloc_line();
			}
			last = i + 1;
		}
	}
	close(fd);

	show();
	help();

	while (1)
	{
		printf("\nPlease enter a command:\n");
		memset(buf, 0, MAX_LINE_LEN);
		gets(buf, MAX_LINE_LEN - 1);

		// ins
		if (buf[0] == 'i' && buf[1] == 'n' && buf[2] == 's')
		{
			if (buf[3] == '-' && atoi(&buf[4]) >= 0)
			{
				ins(atoi(&buf[4]) - 1);
			}
			else if (buf[3] == '\0' || buf[3] == '\n' || buf[3] == ' ')
			{
				ins(now_line_num);
			}
			else
			{
				printf("error use of ins\n");
				help();
			}
		}
		// mod
		else if (buf[0] == 'm' && buf[1] == 'o' && buf[2] == 'd')
		{
			if (buf[3] == '-' && atoi(&buf[4]) >= 0)
			{
				mod(atoi(&buf[4]) - 1);
			}
			else if (buf[3] == '\0' || buf[3] == '\n' || buf[3] == ' ')
			{
				mod(now_line_num);
			}
			else
			{
				printf("error use of mod\n");
				help();
			}
		}
		// del
		else if (buf[0] == 'd' && buf[1] == 'e' && buf[2] == 'l')
		{
			if (buf[3] == '-' && atoi(&buf[4]) >= 0)
			{
				del(atoi(&buf[4]) - 1);
			}
			else if (buf[3] == '\0' || buf[3] == '\n' || buf[3] == ' ')
			{
				del(now_line_num);
			}
			else
			{
				printf("error use of del\n");
				help();
			}
		}
		// show
		else if (buf[0] == 's' && buf[1] == 'h' && buf[2] == 'o' && buf[3] == 'w')
		{
			need_show = 1;
			printf("now enable show current contents after executing a command.\n");
			show();
		}
		// hide
		else if (buf[0] == 'h' && buf[1] == 'i' && buf[2] == 'd' && buf[3] == 'e')
		{
			need_show = 0;
			printf("now disable show current contents after executing a command.\n");
		}
		// help
		else if (buf[0] == 'h' && buf[1] == 'e' && buf[2] == 'l' && buf[3] == 'p')
		{
			help();
		}
		// save
		else if (buf[0] == 's' && buf[1] == 'a' && buf[2] == 'v' && buf[3] == 'e')
		{
			save();
		}
		// exit
		else if (buf[0] == 'e' && buf[1] == 'x' && buf[2] == 'i' && buf[3] == 't')
		{
			quit();
		}
		// unknow
		else
		{
			printf("unknow command.\n");
			help();
		}
	}
	exit(0);
}

void alloc_line()
{
	while (now_line_num >= alloc_line_num)
	{
		lines[alloc_line_num] = malloc(MAX_LINE_LEN);
		lines[alloc_line_num++][0] = 0;
	}
}

void show()
{
	printf("*****************************************************\n");
	for (int i = 0; i <= now_line_num; i++)
		printf("%d%d%d: %s\n", (i + 1) / 100, (i + 1) / 10, (i + 1), lines[i]);
	printf("*****************************************************\n");
}

void help()
{
	printf("*****************************************************\n");
	printf("Input your command:\n");
	printf("help, show help\n");
	printf("ins-n, insert a line after line n\n");
	printf("mod-n, modify line n\n");
	printf("del-n, delete line n\n");
	printf("ins, insert a line after the last line\n");
	printf("mod, modify the last line\n");
	printf("del, delete the last line\n");
	printf("show, enable show current contents after executing a command.\n");
	printf("hide, disable show current contents after executing a command.\n");
	printf("save, save the file\n");
	printf("exit, exit editor\n");
	printf("*****************************************************\n");
}

void ins(int n)
{
	if (n < 0 || n > now_line_num)
	{
		printf("input line_num error\n");
		return;
	}
	// 文件过大，当前行数不足以存下
	if (now_line_num == MAX_LINE_NUM - 1)
	{
		printf("file is to large that can't ins\n");
		return;
	}
	printf("Please input the ins_text:\n");
	gets(buf, MAX_LINE_LEN - 1);
	buf[strlen(buf) - 1] = '\0';

	// 新增一行
	now_line_num++;
	alloc_line();

	// 将后面的行往后腾出一行
	for (int i = now_line_num; i > n; i--)
	{
		strcpy(lines[i], lines[i - 1]);
	}
	// 写入内容
	lines[n + 1][0] = 0;
	strcpy(lines[n + 1], buf);

	changed = 1;
	if (need_show == 1)
		show();
}

void mod(int n)
{
	if (n < 0 || n > now_line_num)
	{
		printf("input line_num error\n");
		return;
	}
	printf("Please input the mod_text:\n");
	gets(buf, MAX_LINE_LEN - 1);
	buf[strlen(buf) - 1] = '\0';

	lines[n][0] = 0;
	strcpy(lines[n], buf);
	changed = 1;
	if (need_show == 1)
		show();
}

void del(int n)
{
	if (n < 0 || n > now_line_num)
	{
		printf("input line_num error\n");
		return;
	}

	// 将后面的行往前一行
	for (int i = n; i < now_line_num; i++)
	{
		strcpy(lines[i], lines[i + 1]);
	}
	now_line_num--;

	changed = 1;
	if (need_show == 1)
		show();
}

void save()
{
	unlink(file_name);

	int fd = open(file_name, O_WRONLY | O_CREATE);
	if (fd == -1)
	{
		fprintf(2, "cannot create %s\n", file_name);
		exit(1);
	}

	for (int i = 0; i <= now_line_num; i++)
	{
		write(fd, lines[i], strlen(lines[i]));
		if (i != now_line_num)
			write(fd, "\n", 1);
	}
	close(fd);

	changed = 0;
	return;
}

void quit()
{
	while (changed == 1)
	{
		printf("do you want to save the file? y/n\n");
		char buf[MAX_LINE_LEN];
		gets(buf, MAX_LINE_LEN - 1);
		if (buf[0] == 'y' || buf[0] == 'Y')
			save();
		else if (buf[0] == 'n' || buf[0] == 'N')
			break;
		else
			printf("unknow ans\n");
	}

	for (int i = 0; i < alloc_line_num; i++)
	{
		free(lines[i]);
	}
	exit(0);
}
void strncat(char *p, const char *q, int n)
{
	while (*p != '\0')
	{
		p++;
	}
	for (int i = 0; i < n; i++)
	{
		*p = *q;
		p++;
		q++;
	}
	*p = '\0';
}