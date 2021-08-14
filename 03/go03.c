#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };

/// <summary>
/// 右、下、左、上
/// </summary>
int dir4[4] = { +1, +WIDTH, -1, -WIDTH };

/// <summary>
/// コウの座標
/// </summary>
int ko_z;

// 指し手の要素数
#define MAX_MOVES 1000

/// <summary>
/// 棋譜
/// </summary>
int record[MAX_MOVES];

/// <summary>
/// n手目。1手目が0
/// </summary>
int moves = 0;

/// <summary>
/// x, y を z（座標；配列のインデックス） に変換
/// </summary>
/// <param name="x">is (1 &lt;= x &lt;= 9)</param>
/// <param name="y">is (1 &lt;= y &lt;= 9)</param>
/// <returns></returns>
int get_z(int x, int y)
{
    return y * WIDTH + x;
}

/// <summary>
/// for display only
/// </summary>
/// <param name="z">座標</param>
/// <returns>人が読める形の座標</returns>
int get81(int z)
{
    int y = z / WIDTH;
    int x = z - y * WIDTH; // 106 = 9*11 + 7 = (x,y)=(7,9) -> 79
    if (z == 0)
        return 0;
    return x * 10 + y; // x*100+y for 19x19
}

/// <summary>
/// 石の色を反転
/// </summary>
/// <param name="col">石の色</param>
/// <returns>反転した石の色</returns>
int flip_color(int col)
{
    return 3 - col;
}

/// <summary>
/// 呼吸点を探索するアルゴリズムで使用
/// </summary>
int check_board[BOARD_MAX];

/// <summary>
/// count_liberty関数の中で呼び出されます。再帰
/// </summary>
/// <param name="tz">着手（開始）座標</param>
/// <param name="color">連の色</param>
/// <param name="p_liberty">呼吸点の数</param>
/// <param name="p_stone">連の石の数</param>
void count_liberty_sub(int tz, int color, int* p_liberty, int* p_stone)
{
    int z, i;

    check_board[tz] = 1; // search flag
    (*p_stone)++;        // number of stone
    for (i = 0; i < 4; i++)
    {
        // 隣の座標
        z = tz + dir4[i];

        // もし、チェック済みの交点なら、無視します
        if (check_board[z])
            continue;

        // もし、空点なら、チェック済みにし、呼吸点を１増やします
        if (board[z] == 0)
        {
            check_board[z] = 1;
            (*p_liberty)++; // number of liberty
        }

        // もし、着手した石と同じ色なら、再帰します
        if (board[z] == color)
            count_liberty_sub(z, color, p_liberty, p_stone);
    }
}

/// <summary>
/// 呼吸点の数
/// </summary>
/// <param name="tz">着手座標</param>
/// <param name="p_liberty">呼吸点の数</param>
/// <param name="p_stone">連の石の数</param>
void count_liberty(int tz, int* p_liberty, int* p_stone)
{
    int i;
    *p_liberty = *p_stone = 0;

    // チェックボードのクリア
    for (i = 0; i < BOARD_MAX; i++)
        check_board[i] = 0;

    count_liberty_sub(tz, board[tz], p_liberty, p_stone);
}

/// <summary>
/// 石を取り上げます
/// </summary>
/// <param name="tz">着手座標</param>
/// <param name="color">石の色</param>
void take_stone(int tz, int color)
{
    int z, i;

    board[tz] = 0;
    for (i = 0; i < 4; i++)
    {
        z = tz + dir4[i];
        if (board[z] == color)
            take_stone(z, color);
    }
}

/// <summary>
/// put stone.
/// </summary>
/// <param name="tz">着手座標。0ならパス</param>
/// <param name="color">石の色</param>
/// <returns>エラーコード。success returns 0</returns>
int put_stone(int tz, int color)
{
    // 検索情報を覚えておく配列
    int around[4][3];

    // 相手の石の色
    int un_col = flip_color(color);

    // 空白に石を置いたら1
    int space = 0;

    // 壁に石を置いたら1
    int wall = 0;

    // 自殺手になってしまうとき1
    int mycol_safe = 0;

    // 取り上げた石の数
    int capture_sum = 0;

    // コウかもしれないとき1
    int ko_maybe = 0;

    // 呼吸点の数
    int liberty;

    // 連の石の数
    int stone;

    // ループ・カウンタ
    int i;

    // pass
    if (tz == 0)
    {
        ko_z = 0;
        return 0;
    }

    // count 4 neighbor's liberty and stones.
    for (i = 0; i < 4; i++)
    {
        int z, c, liberty, stone;
        around[i][0] = around[i][1] = around[i][2] = 0;

        // 隣の座標
        z = tz + dir4[i];

        c = board[z]; // color

        // もし、隣が空点なら
        if (c == 0)
            space++;

        // もし、隣が壁なら
        if (c == 3)
            wall++;

        // もし、隣が空点または壁なら
        if (c == 0 || c == 3)
            continue;

        // 呼吸点の数と、連の石の数を数えます
        count_liberty(z, &liberty, &stone);
        around[i][0] = liberty;
        around[i][1] = stone;
        around[i][2] = c;

        // 隣の石が相手の色で、呼吸点が1なら、その石を取れます
        if (c == un_col && liberty == 1)
        {
            capture_sum += stone;
            ko_maybe = z;
        }

        // もし隣に自分の色の石があっても、その石の呼吸点が２以上あればセーフ
        if (c == color && liberty >= 2)
            mycol_safe++;
    }

    // 石を取っておらず、隣に空点がなく、隣に呼吸点が２つ以上空いている自分の石もないなら、自殺手
    if (capture_sum == 0 && space == 0 && mycol_safe == 0)
        return 1; // suicide

    // もし、コウの座標に石を置こうとしたら、コウ
    if (tz == ko_z)
        return 2; // ko

    // もし、目の座標に石を置こうとしたら、目潰し
    //if ( wall + mycol_safe == 4 ) return 3; // eye

    // もし、石の上に石を置こうとしたら、反則手
    if (board[tz] != 0)
        return 4;

    // 取れる相手の石を取ります
    for (i = 0; i < 4; i++)
    {
        int lib = around[i][0];
        int c = around[i][2];
        if (c == un_col && lib == 1 && board[tz + dir4[i]])
        {
            take_stone(tz + dir4[i], un_col);
        }
    }

    // 石を置きます
    board[tz] = color;

    // 着手点を含む連の呼吸点の数を数えます
    count_liberty(tz, &liberty, &stone);
    // 石を1個取ったらコウかも知れない
    if (capture_sum == 1 && stone == 1 && liberty == 1)
        ko_z = ko_maybe;
    else
        ko_z = 0;

    return 0;
}

/// <summary>
/// 盤の描画
/// </summary>
void print_board()
{
    int x, y;
    const char* str[4] = { ".", "X", "O", "#" };

    // 筋の符号の表示
    printf("   ");
    for (x = 0; x < B_SIZE; x++)
        printf("%d", x + 1);
    printf("\n");

    // 盤の各行の表示
    for (y = 0; y < B_SIZE; y++)
    {
        printf("%2d ", y + 1);
        for (x = 0; x < B_SIZE; x++)
        {
            printf("%s", str[board[get_z(x + 1, y + 1)]]);
        }
        if (y == 4)
            printf("  ko_z=%d", get81(ko_z));
        printf("\n");
    }
}

/// <summary>
/// 空点の座標をランダムに取得
/// </summary>
/// <returns>空点の座標</returns>
int get_empty_z()
{
    int x = 0, y = 0, z = 0;

    // 枠も気にせずランダムに探します。
    // 正確な自然数の乱数は、範囲を指定して　続けて或る程度の回数トライすると　範囲の中のすべての要素を巡ります
    for (;;)
    {
        x = (rand() % B_SIZE) + 1;
        y = (rand() % B_SIZE) + 1;
        z = get_z(x, y);
        if (board[z] == 0)
            break;
    }
    return z;
}

/// <summary>
/// ランダム・ムーブで100手打ちます
/// </summary>
/// <param name="color">石の色</param>
/// <returns>座標</returns>
int play_one_move(int color)
{
    int i, z, err;
    for (i = 0; i < 100; i++)
    {
        z = get_empty_z();
        err = put_stone(z, color);
        if (err == 0)
            return z;
    }

    // パス
    z = 0;
    put_stone(0, color);
    return z;
}

/// <summary>
/// プログラムはここから始まります
/// </summary>
/// <returns>エラーコード。正常時は0</returns>
int main()
{
    int color = 1;
    int z;

    //srand( (unsigned)time( NULL ) );
    for (;;)
    {
        z = play_one_move(color);
        printf("moves=%4d, color=%d, z=%d\n", moves, color, get81(z));
        print_board();

        record[moves] = z;
        moves++;
        if (moves == MAX_MOVES)
        {
            printf("max moves!\n");
            break;
        }
        if (z == 0 && moves >= 2 && record[moves - 2] == 0)
        {
            printf("two pass\n");
            break;
        }

        color = flip_color(color);
    }

    return 0;
}
