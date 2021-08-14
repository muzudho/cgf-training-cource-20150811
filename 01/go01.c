#include <stdio.h>
#include <stdlib.h>

// n路盤
#define B_SIZE 9

// 両端に番兵込みの幅
#define WIDTH (B_SIZE + 2)

// 番兵込みの盤の面積
#define BOARD_MAX (WIDTH * WIDTH)

/// <summary>
/// 盤
/// </summary>
int board[BOARD_MAX] = {
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 2, 1, 1, 0, 1, 0, 0, 0, 0, 3,
    3, 2, 2, 1, 1, 0, 1, 2, 0, 0, 3,
    3, 2, 0, 2, 1, 2, 2, 1, 1, 0, 3,
    3, 0, 2, 2, 2, 1, 1, 1, 0, 0, 3,
    3, 0, 0, 0, 2, 1, 2, 1, 0, 0, 3,
    3, 0, 0, 2, 0, 2, 2, 1, 0, 0, 3,
    3, 0, 0, 0, 0, 2, 1, 1, 0, 0, 3,
    3, 0, 0, 0, 0, 2, 2, 1, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 2, 1, 0, 0, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };

/// <summary>
/// 盤の描画
/// </summary>
void print_board()
{
    int x, y;
    const char* str[4] = { ".", "X", "O", "#" };

    // 筋の表示
    printf("   ");
    for (x = 0; x < B_SIZE; x++)
        printf("%d", x + 1);
    printf("\n");

    // 盤部の表示
    for (y = 0; y < B_SIZE; y++)
    {
        printf("%2d ", y + 1);
        for (x = 0; x <= B_SIZE; x++)
        {
            int c = board[(y + 1) * WIDTH + (x + 1)];
            printf("%s", str[c]);
        }
        printf("\n");
    }
}

/// <summary>
/// プログラムはここから始まります
/// </summary>
/// <returns>エラーコード。正常時は0</returns>
int main()
{
    print_board();
    return 0;
}
