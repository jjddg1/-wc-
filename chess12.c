#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define SIZE 15                    // 棋盘大小 15x15
#define CHARSIZE 3                 // UTF-8中文字符占用3字节
#define MAX_DEPTH 3                // 默认搜索深度
#define INFINITY 1000000           // 极大值
#define TIME_LIMIT_MS 10000        // 搜索时间限制(10秒)
#define ABSOLUTE_MAX_DEPTH 10      // 最大搜索深度
#define MAX_CANDIDATES 40          // 候选点最大数量

// 全局变量定义
clock_t searchStart;               // 搜索开始时间
volatile int stopSearch = 0;       // 停止搜索标志

// 棋盘显示相关数组
char arrayForEmptyBoard[SIZE][SIZE*CHARSIZE+1] = {
    "┏┯┯┯┯┯┯┯┯┯┯┯┯┯┓",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┠┼┼┼┼┼┼┼┼┼┼┼┼┼┨",
    "┗┷┷┷┷┷┷┷┷┷┷┷┷┷┛"
};

char arrayForDisplayBoard[SIZE][SIZE*CHARSIZE+1];  // 显示棋盘
int arrayForInnerBoardLayout[SIZE][SIZE];          // 内部棋盘状态(0:空,1:黑,2:白)

// 棋子显示字符
char play1Pic[] = "●";      // 黑棋子
char play2Pic[] = "◎";      // 白棋子

// 游戏状态变量
int currentPlayer = 1;        // 当前玩家(1:黑棋,2:白棋)
int gameover = 0;             // 游戏结束标志
int humanPlaysBlack = 1;      // 玩家执黑标志
int humanIsFirst = 1;         // 玩家先手标志
int gameMode = 2;             // 游戏模式(1:人人对战,2:人机对战)

// 位置结构体
typedef struct {
    int row;
    int col;
} Position;

// 候选点结构体
typedef struct {
    int row;
    int col;
    int score;
} Candidate;

// 威胁等级枚举
typedef enum {
    THREAT_NONE = 0,
    THREAT_LIVE_THREE = 1,
    THREAT_FOUR = 2,
    THREAT_LIVE_FOUR = 3,
    THREAT_WIN = 4
} ThreatLevel;

// 全局变量：AI最后一步位置
Position lastAIMove;

// 函数声明
void initRecordBorard(void);
void innerLayoutToDisplayArray(void);
void displayBoard(void);
int checkGameWin();
int getPlayerMove();
void makeAIMove(void);
Position findBestMove(void);
int minimax(int depth, int alpha, int beta, int maximizingPlayer);
int evaluateBoard(void);
int evaluatePoint(int r, int c, int player);
int analyzeDirection(int r, int c, int player, int dr, int dc);
int hasNeighbor(int r, int c);
int hasNeighborRange(int r, int c, int range);
int isWinningMove(int row, int col, int player);
int scorePattern(const char *s);
void buildLineString(int r, int c, int dr, int dc, int player, char *out);
int isForbiddenMove(int r, int c);
int canBeRealFive(int r, int c, int p);
int checkExactFive(int r, int c, int p);
ThreatLevel findMaxThreat(int player);
int getThreatLevel(int r, int c, int player);
void sortCandidates(Candidate *cand, int n);
int generateBestDefenseMoves(int defender, int attackR, int attackC, int attacker, Candidate *cand);
int quickVCFCheck(int attacker);
int VCT(int attacker, int depth);
int VCF(int attacker, int depth);
int VCT_search(int attacker, int depth, int lastR, int lastC);
int canStartRealVCT(int r, int c, int attacker, int depth);
int canStartRealVCF(int r, int c, int attacker, int minLength);

// 主函数：程序入口
int main() {
    // Windows 环境下初始化 UTF-8 编码支持
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    char choice;
    int humanPlayer, aiPlayer;

    srand(time(NULL));
    initRecordBorard();
    innerLayoutToDisplayArray();
    displayBoard();

    printf("author:冯禹超\n");
    printf("请选择对战模式：\n");
    printf("1.人人对战\n");
    printf("2.人机对战\n");
    printf("请选择(1或2):");
    scanf("%d", &gameMode);
    while(getchar() != '\n');  // 清空输入缓冲区

    // 设置游戏模式参数
    if (gameMode == 2) {
        printf("请选择谁先手：\n");
        printf("1.玩家先手\n");
        printf("2.AI先手\n");
        printf("请选择(1或2):");
        scanf(" %c", &choice);
        while(getchar() != '\n');

        if (choice == '2') {
            humanIsFirst = 0;
            humanPlaysBlack = 0;
            humanPlayer = 2;
            aiPlayer = 1;
            currentPlayer = aiPlayer;
            printf("\n你选择了：AI先手\n");
            printf("玩家执白棋(◎)，AI执黑棋(●)\n");
        } else {
            humanIsFirst = 1;
            humanPlaysBlack = 1;
            humanPlayer = 1;
            aiPlayer = 2;
            currentPlayer = humanPlayer;
            printf("\n你选择了：玩家先手\n");
            printf("玩家执黑棋(●)，AI执白棋(◎)\n");
        }
    } else {
        // 人人对战模式
        humanPlaysBlack = 1;
        humanIsFirst = 1;
        humanPlayer = 1;
        aiPlayer = 2;
        currentPlayer = humanPlayer;
        printf("\n人人对战模式\n");
        printf("玩家1:● 玩家2:◎\n");
    }

    displayBoard();
    printf("\n按回车开始游戏...");
    getchar();

    // 主游戏循环
    while (!gameover) {
        if (gameMode == 1) {
            // 人人对战模式
            printf("当前玩家:%d\n", currentPlayer);
            if (getPlayerMove() != 0) continue;
        } else {
            // 人机对战模式
            if (currentPlayer == humanPlayer) {
                while (getPlayerMove() != 0) {}  // 等待玩家有效输入
            } else {
                makeAIMove();  // AI思考并落子
            }
        }

        // 更新显示
        innerLayoutToDisplayArray();
        displayBoard();

        // 显示AI落子位置
        if (gameMode == 2 && currentPlayer != humanPlayer) {
            printf("AI落子于%c%d\n",
                   'A' + lastAIMove.col,
                   15 - lastAIMove.row);
        }

        // 检查游戏状态
        int gameState = checkGameWin();
        if (gameState == 1) {
            gameover = 1;
            printf("\n游戏结束\n");
            break;
        } else if (gameState == -1) {
            gameover = 1;
            printf("\n平局\n");
            break;
        }

        // 切换玩家
        currentPlayer = 3 - currentPlayer;
    }

    printf("\n游戏结束！按回车退出...\n");
    getchar();
    return 0;
}
// 初始化空棋盘
void initRecordBorard(void) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            arrayForInnerBoardLayout[i][j] = 0;
        }
    }
}
// 将内部棋盘状态转换为显示数组
void innerLayoutToDisplayArray(void) {
    // 复制空棋盘模板
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j <= SIZE * CHARSIZE + 1; j++) {
            arrayForDisplayBoard[i][j] = arrayForEmptyBoard[i][j];
        }
    }
    
    // 添加棋子到显示棋盘
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (arrayForInnerBoardLayout[i][j] == 1) {
                // 黑棋
                arrayForDisplayBoard[i][CHARSIZE * j] = play1Pic[0];
                arrayForDisplayBoard[i][CHARSIZE * j + 1] = play1Pic[1];
                if (CHARSIZE == 3)
                    arrayForDisplayBoard[i][CHARSIZE * j + 2] = play1Pic[2];
            } else if (arrayForInnerBoardLayout[i][j] == 2) {
                // 白棋
                arrayForDisplayBoard[i][CHARSIZE * j] = play2Pic[0];
                arrayForDisplayBoard[i][CHARSIZE * j + 1] = play2Pic[1];
                if (CHARSIZE == 3)
                    arrayForDisplayBoard[i][CHARSIZE * j + 2] = play2Pic[2];
            }
        }
    }
}
// 显示棋盘
void displayBoard(void) {
#ifdef _WIN32
    system("cls");  // Windows 清屏
#else
    system("clear");  // Linux/macOS 清屏
#endif
    
    // 显示棋盘行和数字编号
    for (int i = 0; i < SIZE; i++) {
        printf("%2d ", SIZE - i);
        
        // 显示棋盘内容，每3个字符加一个空格
        for (int j = 0; j < SIZE * CHARSIZE; j++) {
            putchar(arrayForDisplayBoard[i][j]);
            
            if ((j + 1) % 3 == 0 && j < SIZE * CHARSIZE - 1) {
                putchar(' ');
            }
        }
        printf("\n");
    }   
    // 显示列字母标记
    printf("  ");
    for (int i = 0; i < SIZE; i++) {
        printf("%2c", 'A' + i);
    }
    printf("\n");
}
// 获取玩家落子位置
int getPlayerMove() {
    char letter;
    int number;
    int row, col;
    int player = currentPlayer;

    printf("请输入(例如:A 1):");
    scanf(" %c %d", &letter, &number);
    while(getchar() != '\n');  // 清空输入缓冲区

    // 处理输入字母大小写
    if (letter >= 'a' && letter <= 'z') {
        letter = letter - 'a' + 'A';
    }
    
    // 输入有效性检查
    if (letter < 'A' || letter > 'O') {
        printf("错误：列必须是A到O之间的字母\n");
        return -1;
    }
    if (number < 1 || number > 15) {
        printf("错误：行必须是1到15之间的数字\n");
        return -1;
    }

    // 坐标转换
    col = letter - 'A';
    row = 15 - number;

    // 检查位置是否为空
    if (arrayForInnerBoardLayout[row][col] != 0) {
        printf("错误：该位置已有棋子\n");
        return -1;
    }

    // 黑棋禁手检查
    if (player == 1) {
        if (isForbiddenMove(row, col)) {
            innerLayoutToDisplayArray();
            displayBoard();
            printf("\n您下在了禁手，失败！\n");
            gameover = 1;
            exit(0);
        }
    }

    // 落子
    arrayForInnerBoardLayout[row][col] = player;
    lastAIMove = (Position){row, col};
    return 0;
}
// 检查游戏是否结束(获胜或平局)
int checkGameWin() {
    if (lastAIMove.row < 0 || lastAIMove.col < 0) return 0;
    
    int r0 = lastAIMove.row, c0 = lastAIMove.col;
    
    // 只检查最后落子周围的9x9区域
    int startR = r0 - 4 < 0 ? 0 : r0 - 4;
    int endR = r0 + 4 >= SIZE ? SIZE - 1 : r0 + 4;
    int startC = c0 - 4 < 0 ? 0 : c0 - 4;
    int endC = c0 + 4 >= SIZE ? SIZE - 1 : c0 + 4;
    
    for (int i = startR; i <= endR; i++) {
        for (int j = startC; j <= endC; j++) {
            int player = arrayForInnerBoardLayout[i][j];
            if (player == 0) continue;
            
            // 水平方向检查
            if (j <= SIZE - 5) {
                if (player == arrayForInnerBoardLayout[i][j + 1] &&
                    player == arrayForInnerBoardLayout[i][j + 2] &&
                    player == arrayForInnerBoardLayout[i][j + 3] &&
                    player == arrayForInnerBoardLayout[i][j + 4])
                    return 1;
            }
            
            // 垂直方向检查
            if (i <= SIZE - 5) {
                if (player == arrayForInnerBoardLayout[i + 1][j] &&
                    player == arrayForInnerBoardLayout[i + 2][j] &&
                    player == arrayForInnerBoardLayout[i + 3][j] &&
                    player == arrayForInnerBoardLayout[i + 4][j])
                    return 1;
            }
            
            // 右下对角线检查
            if (i <= SIZE - 5 && j <= SIZE - 5) {
                if (player == arrayForInnerBoardLayout[i + 1][j + 1] &&
                    player == arrayForInnerBoardLayout[i + 2][j + 2] &&
                    player == arrayForInnerBoardLayout[i + 3][j + 3] &&
                    player == arrayForInnerBoardLayout[i + 4][j + 4])
                    return 1;
            }
            
            // 左下对角线检查
            if (i <= SIZE - 5 && j >= 4) {
                if (player == arrayForInnerBoardLayout[i + 1][j - 1] &&
                    player == arrayForInnerBoardLayout[i + 2][j - 2] &&
                    player == arrayForInnerBoardLayout[i + 3][j - 3] &&
                    player == arrayForInnerBoardLayout[i + 4][j - 4])
                    return 1;
            }
        }
    }
    
    // 平局检查(棋盘已满)
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (arrayForInnerBoardLayout[i][j] == 0) return 0;
        }
    }
    return -1;  // 平局
}

// AI落子函数
void makeAIMove(void) {
    Position bestMove = findBestMove();
    int ai = humanPlaysBlack ? 2 : 1;
    arrayForInnerBoardLayout[bestMove.row][bestMove.col] = ai;
    lastAIMove = bestMove;
}

// 候选点排序(按分数降序)
void sortCandidates(Candidate *cand, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (cand[j].score > cand[i].score) {
                Candidate t = cand[i];
                cand[i] = cand[j];
                cand[j] = t;
            }
        }
    }
}

// 寻找最佳落子位置(核心AI函数)
Position findBestMove(void) {
    Position bestMove = {-1, -1};
    int ai = humanPlaysBlack ? 2 : 1;
    int hu = humanPlaysBlack ? 1 : 2;
    double defenseWeight = (ai == 1) ? 1.2: 1.5;  // 黑棋1.2,白棋1.5

    // 统计棋盘上已有棋子数
    int stoneCount = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (arrayForInnerBoardLayout[i][j] != 0) stoneCount++;
        }
    }
    
    // LEVEL 1: 开局天元(第一步下中心)
    if (stoneCount == 0) return (Position){7, 7};
    
    // LEVEL 2: 杀招扫描
    // 先检查AI自己是否有必胜点
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (arrayForInnerBoardLayout[i][j] != 0) continue;
            
            // AI自己能否直接成五
            if (checkExactFive(i, j, ai)) {
                return (Position){i, j};
            }
        }
    }
    
    // 再检查玩家是否有必胜点(必须防守)
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (arrayForInnerBoardLayout[i][j] != 0) continue;
            
            // 玩家能否直接成五
            if (checkExactFive(i, j, hu)) {
                return (Position){i, j};
            }
        }
    }
    
    // 更宽松的胜利检查
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (arrayForInnerBoardLayout[i][j] != 0) continue;
            if (ai == 1 && isForbiddenMove(i, j)) continue;
            
            if (isWinningMove(i, j, ai)) return (Position){i, j};
            if (isWinningMove(i, j, hu)) bestMove = (Position){i, j};
        }
    }
    if (bestMove.row != -1) return bestMove;
    
    // VCF(连续冲四)攻击检查
    if (stoneCount >= 4) {
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (arrayForInnerBoardLayout[i][j] != 0) continue;
                if (!hasNeighborRange(i, j, 2)) continue;
                if (ai == 1 && isForbiddenMove(i, j)) continue;
                
                if (canStartRealVCF(i, j, ai, 12)) {
                    return (Position){i, j};
                }
            }
        }
    }
    
    // VCT(连续活三)攻击检查
    if (stoneCount >= 8) {
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (arrayForInnerBoardLayout[i][j] != 0) continue;
                if (!hasNeighborRange(i, j, 2)) continue;
                if (ai == 1 && isForbiddenMove(i, j)) continue;
                
                if (canStartRealVCT(i, j, ai, 8)) {
                    return (Position){i, j};
                }
            }
        }
    }
    
    // 紧急防御检查(防止对手的VCT/VCF)
    if (stoneCount >= 6) {
        Candidate urgentDefense[5];
        int urgentCount = 0;
        
        for (int i = 0; i < SIZE && urgentCount < 5; i++) {
            for (int j = 0; j < SIZE && urgentCount < 5; j++) {
                if (arrayForInnerBoardLayout[i][j] != 0) continue;
                if (!hasNeighborRange(i, j, 2)) continue;
                if (hu == 1 && isForbiddenMove(i, j)) continue;
                
                // 检查玩家是否能开始VCF/VCT
                if (canStartRealVCF(i, j, hu, 12) || canStartRealVCT(i, j, hu, 8)) {
                    Candidate defenseCands[6];
                    int defCount = generateBestDefenseMoves(ai, i, j, hu, defenseCands);
                    if (defCount > 0) {
                        urgentDefense[urgentCount++] = defenseCands[0];
                    }
                }
            }
        }
        
        if (urgentCount > 0) {
            sortCandidates(urgentDefense, urgentCount);
            return (Position){urgentDefense[0].row, urgentDefense[0].col};
        }
    }
    
    // LEVEL 3: 候选点评估
    Candidate cand[MAX_CANDIDATES];
    int candCount = 0;
    
    // 收集所有可能的落子点并评分
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (arrayForInnerBoardLayout[i][j] != 0 || !hasNeighborRange(i, j, 2)) continue;
            
            // 黑棋禁手点过滤
            if (ai == 1 && isForbiddenMove(i, j)) continue;
            
            // 评估该点对AI和玩家的价值
            int s_ai = evaluatePoint(i, j, ai);
            int s_hu = evaluatePoint(i, j, hu);
            
            // 禁手博弈策略：如果玩家是黑棋且该点是禁手，AI应该抢占
            if (hu == 1 && isForbiddenMove(i, j)) {
                if (ai == 2) {
                    s_hu = 0;
                    s_ai += 5000;
                }
            }
            
            // 中心权重(开局阶段中心位置更有价值)
            int centerBonus = (stoneCount < 15) ? (7 - abs(i - 7) + 7 - abs(j - 7)) * 50 : 0;
            
            // 综合评分 = AI进攻分 + 玩家威胁分*防守权重 + 中心奖励
            int score = s_ai + (int)(s_hu * defenseWeight) + centerBonus;
            
            // 维护候选点列表
            if (candCount < MAX_CANDIDATES) {
                cand[candCount++] = (Candidate){i, j, score};
            } else {
                // 如果候选点已满，替换掉分数最低的点
                int minIdx = 0;
                for (int k = 1; k < MAX_CANDIDATES; k++) {
                    if (cand[k].score < cand[minIdx].score) minIdx = k;
                }
                if (score > cand[minIdx].score) {
                    cand[minIdx] = (Candidate){i, j, score};
                }
            }
        }
    }
    
    // 按分数排序候选点
    sortCandidates(cand, candCount);
    
    // LEVEL 4: 迭代加深搜索(Minimax)
    searchStart = clock();
    stopSearch = 0;
    bestMove = (Position){cand[0].row, cand[0].col};
    
    // 从浅到深进行搜索
    for (int depth = 2; depth <= ABSOLUTE_MAX_DEPTH; depth += 2) {
        int levelBest = -INFINITY - 1000;
        Position levelMove = bestMove;
        
        // 对每个候选点进行搜索
        for (int i = 0; i < candCount; i++) {
            // 时间检查
            if ((clock() - searchStart) * 1000 / CLOCKS_PER_SEC > TIME_LIMIT_MS) {
                stopSearch = 1;
                break;
            }
            
            int r = cand[i].row, c = cand[i].col;
            // 模拟落子
            arrayForInnerBoardLayout[r][c] = ai;
            int score = minimax(depth - 1, -INFINITY, INFINITY, 0);
            arrayForInnerBoardLayout[r][c] = 0;
            
            // 更新最佳选择
            if (score > levelBest) {
                levelBest = score;
                levelMove = (Position){r, c};
            }
            
            // 发现必胜点，直接返回
            if (score >= 900000) {
                lastAIMove = levelMove;
                return levelMove;
            }
        }
        
        // 如果搜索未被中断，更新最佳走法
        if (!stopSearch) bestMove = levelMove;
        else break;
    }
    
    lastAIMove = bestMove;
    return bestMove;
}
// 检查落子是否形成胜利
int isWinningMove(int row, int col, int player) {
    arrayForInnerBoardLayout[row][col] = player;
    int winner = checkGameWin();
    arrayForInnerBoardLayout[row][col] = 0;
    return (winner == 1);  // checkGameWin返回1表示有玩家获胜
}
// 分析一个方向上的棋形
int analyzeDirection(int r, int c, int player, int dr, int dc) {
    char line[16];
    buildLineString(r, c, dr, dc, player, line);
    return scorePattern(line);
}
// 评估一个空位对指定玩家的价值
int evaluatePoint(int r, int c, int player) {
    // 黑棋禁手检查
    if (player == 1 && isForbiddenMove(r, c)) {
        return -999999;
    }
    
    int totalScore = 0;
    int count5 = 0, countL4 = 0, countD4 = 0, countL3 = 0, countD3 = 0;
    int dr[] = {1, 0, 1, 1}, dc[] = {0, 1, 1, -1};
    
    // 扫描四个方向
    for (int i = 0; i < 4; i++) {
        char line[16];
        buildLineString(r, c, dr[i], dc[i], player, line);
        int s = scorePattern(line);
        
        // 统计各种棋形的数量
        if (s >= 1000000) count5++;
        else if (s >= 200000) countL4++;
        else if (s >= 60000) countD4++;
        else if (s >= 30000) countL3++;
        else if (s >= 3000) countD3++;
        
        totalScore += s;
    }
    
    // 杀招等级划分
    if (count5 > 0) return 1400000;                 // 连五
    if (countL4 > 0 || countD4 >= 2) return 1200000; // 活四或双冲四
    if (player == 2 && countL3 >= 2) return 1050000; // 白棋双活三绝杀
    if (countD4 >= 1 && countL3 >= 1) return 800000; // 四三杀
    
    // 空间压制修正：避免在对手密集区域落废子
    if (totalScore < 10000) {
        int enemy = 3 - player;
        int enemyCount = 0;
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                int nr = r + i, nc = c + j;
                if (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE) {
                    if (arrayForInnerBoardLayout[nr][nc] == enemy) enemyCount++;
                }
            }
        }
        if (enemyCount >= 3) totalScore -= 500;  // 减少被包围点的价值
    }
    
    return totalScore;
}

// 评估整个棋盘的局势
int evaluateBoard(void) {
    int ai = humanPlaysBlack ? 2 : 1;
    int hu = humanPlaysBlack ? 1 : 2;
    int aiTotal = 0, huTotal = 0;
    
    // 统计棋盘上已有棋子数
    int stoneCount = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (arrayForInnerBoardLayout[i][j] != 0) stoneCount++;
        }
    }
    
    // 计算AI和玩家的总分数
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int p = arrayForInnerBoardLayout[i][j];
            if (p == ai) aiTotal += evaluatePoint(i, j, ai);
            else if (p == hu) huTotal += evaluatePoint(i, j, hu);
        }
    }
    
    // 动态调整防守权重
    double defenseWeight;
    if (ai == 1) {  // AI执黑
        if (stoneCount <= 4) defenseWeight = 1.6;
        else if (stoneCount < 15 && stoneCount > 4) defenseWeight = 1.4;
        else if (stoneCount < 40) defenseWeight = 1.2;
        else defenseWeight = 1.2;
    } else {  // AI执白
        defenseWeight = (stoneCount < 20) ? 1.6 : 1.4;
    }
    
    return aiTotal - (int)(huTotal * defenseWeight);
}
// 检查一个位置周围是否有邻居棋子(用于剪枝)
int hasNeighbor(int r, int c) {
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr, nc = c + dc;
            if (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE) {
                if (arrayForInnerBoardLayout[nr][nc] != 0) return 1;
            }
        }
    }
    return 0;
}
// Minimax算法实现(带Alpha-Beta剪枝)
int minimax(int depth, int alpha, int beta, int maximizingPlayer) {
    // 游戏结束检查
    int w = checkGameWin();
    if (w == 1) {
        if (maximizingPlayer) return INFINITY;
        else return -INFINITY;
    }
    
    // 搜索终止条件
    if (stopSearch) return evaluateBoard();
    if ((clock() - searchStart) * 1000 / CLOCKS_PER_SEC > TIME_LIMIT_MS) {
        stopSearch = 1;
        return evaluateBoard();
    }
    if (depth == 0) return evaluateBoard();
    
    // 确定当前玩家
    int player = maximizingPlayer ? (humanPlaysBlack ? 2 : 1) : (humanPlaysBlack ? 1 : 2);
    int best = maximizingPlayer ? -INFINITY : INFINITY;
    
    // 搜索所有可能位置
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (arrayForInnerBoardLayout[i][j] != 0) continue;
            if (!hasNeighborRange(i, j, 2)) continue;
            
            // 尝试落子
            arrayForInnerBoardLayout[i][j] = player;
            int val = minimax(depth - 1, alpha, beta, !maximizingPlayer);
            arrayForInnerBoardLayout[i][j] = 0;
            
            // 更新最佳值和剪枝
            if (maximizingPlayer) {
                if (val > best) best = val;
                if (best > alpha) alpha = best;
            } else {
                if (val < best) best = val;
                if (best < beta) beta = best;
            }
            
            if (beta <= alpha || stopSearch) return best;
        }
    }
    
    return best;
}
// 棋形评分函数
int scorePattern(const char* s) {
    // 1. 连五: 绝对胜利
    if (strstr(s, "XXXXX")) return 1000000;
    
    // 2. 活四: 先手绝杀
    if (strstr(s, "_XXXX_")) return 200000;
    
    // 3. 冲四: 必须应手的威胁
    if (strstr(s, "_XXXX") || strstr(s, "XXXX_") || 
        strstr(s, "X_XXX") || strstr(s, "XX_XX") || strstr(s, "XXX_X")) return 60000;
    
    // 4. 活三: 极强的进攻性
    if (strstr(s, "_XXX_") || strstr(s, "_X_XX_") || strstr(s, "_XX_X_")) return 30000;
    
    // 5. 眠三: 有威胁，但不需要立即封堵
    if (strstr(s, "__XXX") || strstr(s, "XXX__") || strstr(s, "X__XX") || 
        strstr(s, "XX__X") || strstr(s, "X_X_X")) return 3000;
    
    // 6. 活二: 阵地战的基础
    if (strstr(s, "__XX__") || strstr(s, "_X_X_")) return 500;
    
    // 7. 眠二与弱连接: 大幅压低分数
    if (strstr(s, "XX_") || strstr(s, "_XX") || strstr(s, "X__X")) return 50;
    
    return 0;
}
// 构建一个方向上的棋形字符串
void buildLineString(int r, int c, int dr, int dc, int player, char* out) {
    int idx = 0;
    // 从当前位置向两个方向各延伸4格
    for (int k = -4; k <= 4; k++) {
        int nr = r + k * dr;
        int nc = c + k * dc;
        // 棋盘边界外标记为'O'
        if (nr < 0 || nr >= SIZE || nc < 0 || nc >= SIZE) {
            out[idx++] = 'O';
        } else if (k == 0) {
            // 当前位置标记为'X'
            out[idx++] = 'X';
        } else {
            int v = arrayForInnerBoardLayout[nr][nc];
            if (v == 0) out[idx++] = '_';           // 空位
            else if (v == player) out[idx++] = 'X'; // 己方棋子
            else out[idx++] = 'O';                  // 对方棋子
        }
    }
    out[idx] = '\0';
}
// 检查一个位置周围指定范围内是否有邻居棋子
int hasNeighborRange(int r, int c, int range) {
    for (int dr = -range; dr <= range; dr++) {
        for (int dc = -range; dc <= range; dc++) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr, nc = c + dc;
            if (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE) {
                if (arrayForInnerBoardLayout[nr][nc] != 0) return 1;
            }
        }
    }
    return 0;
}
// 检查指定位置是否为禁手(仅对黑棋有效)
int isForbiddenMove(int r, int c) {
    int black = 1;
    if (arrayForInnerBoardLayout[r][c] != 0) return 0;
    
    // 临时落子
    arrayForInnerBoardLayout[r][c] = black;
    
    // 如果成五则不是禁手(五连优先)
    if (checkExactFive(r, c, black)) {
        arrayForInnerBoardLayout[r][c] = 0;
        return 0;
    }
    
    int dr[] = {1, 0, 1, 1}, dc[] = {0, 1, 1, -1};
    int overline = 0, fourCount = 0, liveThreeCount = 0;
    
    // 检查四个方向
    for (int i = 0; i < 4; i++) {
        char s[20];
        buildLineString(r, c, dr[i], dc[i], black, s);
        
        // 检查长连(超过五个子)
        if (strstr(s, "XXXXXX")) overline = 1;
        
        // 检查四四禁手
        int hasFour = 0;
        for (int k = -4; k <= 4; k++) {
            int nr = r + k * dr[i], nc = c + k * dc[i];
            if (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE && arrayForInnerBoardLayout[nr][nc] == 0) {
                if (canBeRealFive(nr, nc, black)) {
                    hasFour = 1;
                    break;
                }
            }
        }
        if (hasFour) fourCount++;
        
        // 检查三三禁手
        if (strstr(s, "_XXX_") || strstr(s, "_X_XX_") || strstr(s, "_XX_X_")) {
            int validDestinations = 0;
            for (int k = -4; k <= 4; k++) {
                int nr = r + k * dr[i], nc = c + k * dc[i];
                if (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE && arrayForInnerBoardLayout[nr][nc] == 0) {
                    arrayForInnerBoardLayout[nr][nc] = black;
                    char nextS[20];
                    buildLineString(nr, nc, dr[i], dc[i], black, nextS);
                    
                    if (strstr(nextS, "_XXXX_")) {
                        validDestinations++;
                    }
                    arrayForInnerBoardLayout[nr][nc] = 0;
                }
            }
            if (validDestinations >= 1) liveThreeCount++;
        }
    }
    
    arrayForInnerBoardLayout[r][c] = 0;  // 恢复棋盘
    
    // 判断禁手类型
    if (overline) return 1;           // 长连禁手
    if (fourCount >= 2) return 1;     // 四四禁手
    if (liveThreeCount >= 2) return 1; // 三三禁手
    
    return 0;
}
// 检查指定位置是否恰好形成五连
int checkExactFive(int r, int c, int p) {
    int dr[] = {1, 0, 1, 1}, dc[] = {0, 1, 1, -1};
    
    for (int i = 0; i < 4; i++) {
        int count = 1;  // 当前位置的棋子
        
        // 正向计数
        int nr = r + dr[i], nc = c + dc[i];
        while (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE && 
               arrayForInnerBoardLayout[nr][nc] == p) {
            count++;
            nr += dr[i];
            nc += dc[i];
        }
        
        // 反向计数
        nr = r - dr[i];
        nc = c - dc[i];
        while (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE && 
               arrayForInnerBoardLayout[nr][nc] == p) {
            count++;
            nr -= dr[i];
            nc -= dc[i];
        }
        
        // 恰好5个子
        if (count == 5) {
            return 1;
        }
    }
    return 0;
}
// 检查在一个位置落子是否能形成真正的五连
int canBeRealFive(int r, int c, int p) {
    if (r < 0 || r >= SIZE || c < 0 || c >= SIZE || arrayForInnerBoardLayout[r][c] != 0) return 0;
    arrayForInnerBoardLayout[r][c] = p;
    int isFive = checkExactFive(r, c, p);
    arrayForInnerBoardLayout[r][c] = 0;
    return isFive;
}
// 获取棋盘上的最大威胁等级
ThreatLevel findMaxThreat(int player) {
    ThreatLevel max = THREAT_NONE;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (arrayForInnerBoardLayout[i][j] != 0) continue;
            if (!hasNeighborRange(i, j, 2)) continue;
            ThreatLevel t = getThreatLevel(i, j, player);
            if (t > max) max = t;
        }
    }
    return max;
}
// 获取一个位置对指定玩家的威胁等级
int getThreatLevel(int r, int c, int player) {
    int maxS = 0;
    int dr[] = {1, 0, 1, 1}, dc[] = {0, 1, 1, -1};
    
    // 模拟落子进行判定
    arrayForInnerBoardLayout[r][c] = player;
    for (int i = 0; i < 4; i++) {
        char s[20];
        buildLineString(r, c, dr[i], dc[i], player, s);
        int score = scorePattern(s);
        if (score > maxS) maxS = score;
    }
    arrayForInnerBoardLayout[r][c] = 0;
    
    // 根据分数判断威胁等级
    if (maxS >= 1000000) return THREAT_WIN;       // 成五
    if (maxS >= 200000) return THREAT_LIVE_FOUR;  // 活四
    if (maxS >= 60000) return THREAT_FOUR;        // 冲四
    if (maxS >= 30000) return THREAT_LIVE_THREE;  // 活三
    return THREAT_NONE;
}
// 生成最佳防守点
int generateBestDefenseMoves(int defender, int attackR, int attackC, int attacker, Candidate *cand) {
    int count = 0;
    
    // 只考虑攻击点周围的5x5区域
    for (int dr = -2; dr <= 2; dr++) {
        for (int dc = -2; dc <= 2; dc++) {
            int r = attackR + dr, c = attackC + dc;
            if (r < 0 || r >= SIZE || c < 0 || c >= SIZE) continue;
            if (arrayForInnerBoardLayout[r][c] != 0) continue;
            
            // 防守方落子测试
            arrayForInnerBoardLayout[r][c] = defender;
            
            // 检查是否能降低攻击方的威胁等级
            ThreatLevel originalThreat = getThreatLevel(attackR, attackC, attacker);
            ThreatLevel newThreat = findMaxThreat(attacker);
            
            if (newThreat < originalThreat) {
                int score = (originalThreat - newThreat) * 10000;
                if (count < 6) {
                    cand[count++] = (Candidate){r, c, score};
                }
            }
            
            arrayForInnerBoardLayout[r][c] = 0;
        }
    }
    
    // 按防守效果排序
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (cand[j].score > cand[i].score) {
                Candidate temp = cand[i];
                cand[i] = cand[j];
                cand[j] = temp;
            }
        }
    }
    
    return count;
}
// 快速VCF(连续冲四)检查
int quickVCFCheck(int attacker) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (arrayForInnerBoardLayout[i][j] != 0) continue;
            
            arrayForInnerBoardLayout[i][j] = attacker;
            int fourCount = 0;
            
            // 统计冲四数量
            for (int dir = 0; dir < 4; dir++) {
                int dr[] = {1, 0, 1, 1}, dc[] = {0, 1, 1, -1};
                char line[20];
                buildLineString(i, j, dr[dir], dc[dir], attacker, line);
                
                if (strstr(line, "XXXX_") || strstr(line, "_XXXX") ||
                    strstr(line, "X_XXX") || strstr(line, "XX_XX") || 
                    strstr(line, "XXX_X")) {
                    fourCount++;
                }
            }
            
            arrayForInnerBoardLayout[i][j] = 0;
            
            if (fourCount >= 2) return 1;  // 发现双冲四
        }
    }
    return 0;
}
// VCF(连续冲四)搜索
int VCF(int attacker, int depth) {
    if (depth <= 0) return 0;
    
    // 时间检查
    if ((clock() - searchStart) * 1000 / CLOCKS_PER_SEC > TIME_LIMIT_MS) {
        stopSearch = 1;
        return 0;
    }
    
    // 检查是否已经赢棋
    if (findMaxThreat(attacker) == THREAT_WIN) return 1;
    
    // 收集VCF候选点
    Candidate vcfCands[10];
    int vcfCount = 0;
    
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (arrayForInnerBoardLayout[i][j] != 0) continue;
            if (!hasNeighborRange(i, j, 2)) continue;
            if (attacker == 1 && isForbiddenMove(i, j)) continue;
            
            // 只考虑能形成至少两个冲四威胁的点
            arrayForInnerBoardLayout[i][j] = attacker;
            int threatCount = 0;
            for (int dir = 0; dir < 4; dir++) {
                int dr[] = {1, 0, 1, 1}, dc[] = {0, 1, 1, -1};
                char line[20];
                buildLineString(i, j, dr[dir], dc[dir], attacker, line);
                
                if (strstr(line, "XXXX_") || strstr(line, "_XXXX") ||
                    strstr(line, "X_XXX") || strstr(line, "XX_XX") || 
                    strstr(line, "XXX_X")) {
                    threatCount++;
                }
            }
            arrayForInnerBoardLayout[i][j] = 0;
            
            if (threatCount >= 2) {
                vcfCands[vcfCount++] = (Candidate){i, j, threatCount * 1000};
                if (vcfCount >= 10) break;
            }
        }
    }
    
    // 按威胁数量排序
    sortCandidates(vcfCands, vcfCount);
    
    // 只尝试最有希望的几个点
    for (int idx = 0; idx < vcfCount && idx < 3; idx++) {
        int i = vcfCands[idx].row, j = vcfCands[idx].col;
        
        arrayForInnerBoardLayout[i][j] = attacker;
        int result = VCF(attacker, depth - 1);
        arrayForInnerBoardLayout[i][j] = 0;
        
        if (result) return 1;
    }
    
    return 0;
}
// VCT(连续活三)搜索
int VCT(int attacker, int depth) {
    if (depth <= 0 || stopSearch) return 0;
    
    // 时间检查
    if ((clock() - searchStart) * 1000 / CLOCKS_PER_SEC > TIME_LIMIT_MS) {
        stopSearch = 1;
        return 0;
    }
    
    // 先检查是否有直接的VCF
    if (quickVCFCheck(attacker)) return 1;
    
    // 收集高质量进攻点
    Candidate highQualityMoves[12];
    int hqCount = 0;
    
    for (int i = 0; i < SIZE && hqCount < 12; i++) {
        for (int j = 0; j < SIZE && hqCount < 12; j++) {
            if (arrayForInnerBoardLayout[i][j] != 0) continue;
            if (!hasNeighborRange(i, j, 2)) continue;
            if (attacker == 1 && isForbiddenMove(i, j)) continue;
            
            // 要求：必须能形成活三并且有后续
            arrayForInnerBoardLayout[i][j] = attacker;
            
            int liveThreeCount = 0;
            int hasFourThreat = 0;
            
            for (int dir = 0; dir < 4; dir++) {
                int dr[] = {1, 0, 1, 1}, dc[] = {0, 1, 1, -1};
                char line[20];
                buildLineString(i, j, dr[dir], dc[dir], attacker, line);
                
                if (strstr(line, "_XXX_") || strstr(line, "_X_XX_") || strstr(line, "_XX_X_")) {
                    liveThreeCount++;
                }
                if (strstr(line, "_XXXX_")) {
                    hasFourThreat = 1;
                }
            }
            
            arrayForInnerBoardLayout[i][j] = 0;
            
            // 高质量进攻点标准：活三+后续，或者活四
            if ((liveThreeCount >= 1 && hasFourThreat) || liveThreeCount >= 2) {
                highQualityMoves[hqCount++] = (Candidate){i, j, liveThreeCount * 1000 + (hasFourThreat ? 500 : 0)};
            }
        }
    }
    
    if (hqCount == 0) return 0;
    
    // 按质量排序
    sortCandidates(highQualityMoves, hqCount);
    
    // 只尝试最高质量的2-3个点
    for (int idx = 0; idx < hqCount && idx < 3; idx++) {
        int r = highQualityMoves[idx].row, c = highQualityMoves[idx].col;
        arrayForInnerBoardLayout[r][c] = attacker;
        
        int defender = 3 - attacker;
        int canWin = 1;
        
        // 防守方的最佳应对
        Candidate bestDefense[6];
        int defCount = generateBestDefenseMoves(defender, r, c, attacker, bestDefense);
        
        if (defCount == 0) {
            // 无有效防守，直接胜利
            canWin = 1;
        } else {
            // 检查所有防守方式
            for (int d = 0; d < defCount && d < 3; d++) {
                arrayForInnerBoardLayout[bestDefense[d].row][bestDefense[d].col] = defender;
                
                // 递归检查
                if (!VCT(attacker, depth - 1)) {
                    canWin = 0;
                }
                
                arrayForInnerBoardLayout[bestDefense[d].row][bestDefense[d].col] = 0;
                if (!canWin) break;
            }
        }
        
        arrayForInnerBoardLayout[r][c] = 0;
        if (canWin) return 1;
    }
    
    return 0;
}
// 检查是否能开始真正的VCT
int canStartRealVCT(int r, int c, int attacker, int depth) {
    if (r < 0 || r >= SIZE || c < 0 || c >= SIZE) return 0;
    if (arrayForInnerBoardLayout[r][c] != 0) return 0;
    if (attacker == 1 && isForbiddenMove(r, c)) return 0;
    
    // 落子测试
    arrayForInnerBoardLayout[r][c] = attacker;
    
    // 检查落子后是否形成至少一个活三且还有后续发展
    int liveThreeCount = 0;
    int hasFollowUp = 0;
    
    // 统计威胁类型
    for (int dir = 0; dir < 4; dir++) {
        int dr[] = {1, 0, 1, 1}, dc[] = {0, 1, 1, -1};
        char line[20];
        buildLineString(r, c, dr[dir], dc[dir], attacker, line);
        
        // 检查活三
        if (strstr(line, "_XXX_") || strstr(line, "_X_XX_") || strstr(line, "_XX_X_")) {
            liveThreeCount++;
            
            // 检查活三是否有后续杀招
            for (int k = -4; k <= 4; k++) {
                int nr = r + k * dr[dir], nc = c + k * dc[dir];
                if (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE && 
                    arrayForInnerBoardLayout[nr][nc] == 0) {
                    
                    // 模拟在活三两端落子
                    arrayForInnerBoardLayout[nr][nc] = attacker;
                    char nextLine[20];
                    buildLineString(nr, nc, dr[dir], dc[dir], attacker, nextLine);
                    
                    if (strstr(nextLine, "_XXXX_")) {
                        hasFollowUp = 1;
                    }
                    
                    arrayForInnerBoardLayout[nr][nc] = 0;
                    if (hasFollowUp) break;
                }
            }
        }
        
        // 检查冲四
        if (strstr(line, "XXXX_") || strstr(line, "_XXXX") ||
            strstr(line, "X_XXX") || strstr(line, "XX_XX") || 
            strstr(line, "XXX_X")) {
            hasFollowUp = 1;
        }
    }
    
    // 真正的VCT需要：至少一个活三 + 有后续发展
    if (liveThreeCount >= 1 && hasFollowUp) {
        // 深度搜索确认
        int result = VCT_search(attacker, depth - 1, r, c);
        arrayForInnerBoardLayout[r][c] = 0;
        return result;
    }
    
    arrayForInnerBoardLayout[r][c] = 0;
    return 0;
}
// VCT搜索辅助函数
int VCT_search(int attacker, int depth, int lastR, int lastC) {
    if (depth <= 0) return 0;
    if ((clock() - searchStart) * 1000 / CLOCKS_PER_SEC > TIME_LIMIT_MS) {
        stopSearch = 1;
        return 0;
    }
    
    // 检查是否已经获胜
    if (checkExactFive(lastR, lastC, attacker)) return 1;
    
    int defender = 3 - attacker;
    int bestResult = 0;
    
    // 收集攻击方可能的进攻点
    Candidate attackCands[15];
    int attackCount = 0;
    
    for (int i = 0; i < SIZE && attackCount < 15; i++) {
        for (int j = 0; j < SIZE && attackCount < 15; j++) {
            if (arrayForInnerBoardLayout[i][j] != 0) continue;
            if (!hasNeighborRange(i, j, 2)) continue;
            if (attacker == 1 && isForbiddenMove(i, j)) continue;
            
            // 评估进攻潜力
            arrayForInnerBoardLayout[i][j] = attacker;
            int threatLevel = getThreatLevel(i, j, attacker);
            arrayForInnerBoardLayout[i][j] = 0;
            
            // 主要关心活三和活四
            if (threatLevel >= THREAT_LIVE_THREE) {
                attackCands[attackCount++] = (Candidate){i, j, threatLevel * 1000};
            }
        }
    }
    
    // 按威胁等级排序
    sortCandidates(attackCands, attackCount);
    
    // 只尝试最有希望的3个进攻点
    for (int idx = 0; idx < attackCount && idx < 3; idx++) {
        int r = attackCands[idx].row, c = attackCands[idx].col;
        
        arrayForInnerBoardLayout[r][c] = attacker;
        
        // 收集防守方的应对点
        Candidate defenseCands[8];
        int defenseCount = 0;
        ThreatLevel currentThreat = getThreatLevel(r, c, attacker);
        
        // 防守方只能防守关键点
        for (int di = -2; di <= 2 && defenseCount < 8; di++) {
            for (int dj = -2; dj <= 2 && defenseCount < 8; dj++) {
                int nr = r + di, nc = c + dj;
                if (nr < 0 || nr >= SIZE || nc < 0 || nc >= SIZE) continue;
                if (arrayForInnerBoardLayout[nr][nc] != 0) continue;
                if (!hasNeighborRange(nr, nc, 1)) continue;
                
                // 检查是否能有效防守
                arrayForInnerBoardLayout[nr][nc] = defender;
                ThreatLevel afterDefense = getThreatLevel(r, c, attacker);
                arrayForInnerBoardLayout[nr][nc] = 0;
                
                if (afterDefense < currentThreat) {
                    defenseCands[defenseCount++] = (Candidate){nr, nc, 0};
                }
            }
        }
        
        // 检查是否能应对所有防守
        int canBreakAllDefenses = 1;
        if (defenseCount > 0) {
            for (int d = 0; d < defenseCount && d < 4; d++) {
                int dr = defenseCands[d].row, dc = defenseCands[d].col;
                arrayForInnerBoardLayout[dr][dc] = defender;
                
                // 递归检查防守后是否还能继续VCT
                if (!VCT_search(attacker, depth - 1, r, c)) {
                    canBreakAllDefenses = 0;
                }
                
                arrayForInnerBoardLayout[dr][dc] = 0;
                if (!canBreakAllDefenses) break;
            }
        }
        
        arrayForInnerBoardLayout[r][c] = 0;
        
        if (canBreakAllDefenses) {
            return 1;  // 找到了必胜的VCT路径
        }
    }
    
    return 0;
}
// 检查是否能开始真正的VCF
int canStartRealVCF(int r, int c, int attacker, int minLength) {
    arrayForInnerBoardLayout[r][c] = attacker;
    
    // 检查落子后是否形成至少2个冲四点
    int fourCount = 0;
    
    // 统计能形成冲四的方向数量
    for (int dir = 0; dir < 4; dir++) {
        int dr[] = {1, 0, 1, 1}, dc[] = {0, 1, 1, -1};
        char line[20];
        buildLineString(r, c, dr[dir], dc[dir], attacker, line);
        
        if (strstr(line, "XXXX_") || strstr(line, "_XXXX") ||
            strstr(line, "X_XXX") || strstr(line, "XX_XX") || 
            strstr(line, "XXX_X")) {
            fourCount++;
        }
    }
    
    // 真正的VCF起始需要至少能形成2个冲四威胁
    if (fourCount < 2) {
        arrayForInnerBoardLayout[r][c] = 0;
        return 0;
    }
    
    // 进一步检查这些冲四是否真的能形成链式攻击
    int realVCF = VCF(attacker, minLength);
    
    arrayForInnerBoardLayout[r][c] = 0;
    return realVCF;
}