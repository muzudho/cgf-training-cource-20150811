#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/// <summary>
/// コミ
/// </summary>
double komi = 6.5;

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
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};

/// <summary>
/// 右、下、左、上
/// </summary>
int dir4[4] = {+1, +WIDTH, -1, -WIDTH};

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
void count_liberty_sub(int tz, int color, int *p_liberty, int *p_stone)
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
void count_liberty(int tz, int *p_liberty, int *p_stone)
{
    int i;
    *p_liberty = *p_stone = 0;
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
/// 目潰しをエラーとするなら
/// </summary>
const int FILL_EYE_ERR = 1;

/// <summary>
/// 目潰しを合法手とするなら（囲碁のルールでは合法手）
/// </summary>
const int FILL_EYE_OK = 0;

/// <summary>
/// put stone.
/// </summary>
/// <param name="tz">着手座標。0ならパス</param>
/// <param name="color">石の色</param>
/// <param name="fill_eye_err">目潰しをエラーとするなら1、そうでないなら0</param>
/// <returns>エラーコード。success returns 0. in playout, fill_eye_err = 1</returns>
int put_stone(int tz, int color, int fill_eye_err)
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

        // 隣の石が相手の色で、呼吸点が1なら、その石を取れます
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
    if (wall + mycol_safe == 4 && fill_eye_err)
        return 3; // eye

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
    const char *str[4] = {".", "X", "O", "#"};

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
/// 地の簡易計算（これが厳密に計算できるようなら囲碁は完全解明されている）を表示し、勝敗を返します。
/// スコアにはコミは含みませんが、勝敗にはコミを含んでいます。
/// </summary>
/// <param name="turn_color">手番の色</param>
/// <returns>黒の勝ちなら1、負けなら0</returns>
int count_score(int turn_color)
{
    int x, y, i;
    // 黒のスコア
    int score = 0;
    // 黒の勝ちなら1、負けなら0
    int win;
    // 黒石の数、白石の数
    int black_area = 0, white_area = 0;
    // 石の数＋地の数
    int black_sum, white_sum;
    // 4方向にある石について、[ゴミ値, 盤上の黒石の数, 盤上の白石の数]
    int mk[4];
    // [空点の数, 黒石の数, 白石の数]
    int kind[3];

    kind[0] = kind[1] = kind[2] = 0;
    for (y = 0; y < B_SIZE; y++)
        for (x = 0; x < B_SIZE; x++)
        {
            int z = get_z(x + 1, y + 1);
            int c = board[z];
            kind[c]++;

            // 石が置いてある座標なら以降は無視
            if (c != 0)
                continue;

            // 大雑把に さくっと地計算。
            // 4方向にある黒石、白石の数を数えます
            mk[1] = mk[2] = 0;
            for (i = 0; i < 4; i++)
                mk[board[z + dir4[i]]]++;
            // 黒石だけがあるなら黒の地
            if (mk[1] && mk[2] == 0)
                black_area++;
            // 白石だけがあるなら白の地
            if (mk[2] && mk[1] == 0)
                white_area++;
        }

    // スコア計算（コミ含まず）
    black_sum = kind[1] + black_area;
    white_sum = kind[2] + white_area;
    score = black_sum - white_sum;

    // 黒の勝敗判定
    win = 0;
    if (score - komi > 0)
        win = 1;

    //printf("black_sum=%2d, (stones=%2d, area=%2d)\n",black_sum, kind[1], black_area);
    //printf("white_sum=%2d, (stones=%2d, area=%2d)\n",white_sum, kind[2], white_area);
    //printf("score=%d, win=%d\n",score, win);
    return win;
}

/// <summary>
/// プレイアウトします
/// </summary>
/// <param name="turn_color">手番の石の色</param>
/// <returns>黒の勝ちなら1、負けなら0</returns>
int playout(int turn_color)
{
    int color = turn_color;

    // １つ前の着手の座標
    int previous_z = 0;

    // ループ・カウンタ
    int loop;
    int loop_max = B_SIZE * B_SIZE + 200; // for triple ko

    for (loop = 0; loop < loop_max; loop++)
    {
        // all empty points are candidates.
        int empty[BOARD_MAX];
        // 配列のインデックス
        int empty_num = 0;
        int x, y, z, r, err;
        // 壁を除く盤上の全ての空点の座標を empty配列にセットします
        for (y = 0; y < B_SIZE; y++)
            for (x = 0; x < B_SIZE; x++)
            {
                int z = get_z(x + 1, y + 1);
                // 空点でなければ無視
                if (board[z] != 0)
                    continue;
                empty[empty_num] = z;
                empty_num++;
            }
        // 配列のインデックス
        r = 0;
        for (;;)
        {
            // もし空点がなければ、パス
            if (empty_num == 0)
            {
                z = 0;
            }
            else
            {
                // ランダムに空点を選びます
                r = rand() % empty_num;
                z = empty[r];
            }
            err = put_stone(z, color, FILL_EYE_ERR);
            if (err == 0)
                break;
            // もし空点に石を置くと正常終了しなかったなら、残りの座標で続行します
            empty[r] = empty[empty_num - 1]; // err, delete
            empty_num--;
        }
        // もしパスが連続したら対局終了
        if (z == 0 && previous_z == 0)
            break; // continuous pass
        // そうでなければ盤を表示して手番を変えて続行
        previous_z = z;
        //  print_board();
        //  printf("loop=%d,z=%d,c=%d,empty_num=%d,ko_z=%d\n",
        //         loop, get81(z), color, empty_num, get81(ko_z) );
        color = flip_color(color);
    }

    return count_score(turn_color);
}

/// <summary>
/// 原始モンテカルロ
/// </summary>
/// <param name="color">手番の色</param>
/// <returns>最善手の座標</returns>
int primitive_monte_calro(int color)
{
    int try_num = 30; // number of playout

    // 最善手の座標
    int best_z = 0;

    // 最善の価値
    double best_value;

    // 勝率
    double win_rate;
    int x, y, err, i;
    // プレイアウトして勝った回数
    int win_sum;
    // 黒番が勝ったら1、負けたら0
    int win;

    // コウの座標のコピー
    int ko_z_copy;

    // 盤のコピー
    int board_copy[BOARD_MAX]; // keep current board
    ko_z_copy = ko_z;
    memcpy(board_copy, board, sizeof(board));

    // 根ノードでは黒番は-100点、白番は+100点から
    if (color == 1)
    {
        best_value = -100;
    }
    else
    {
        best_value = +100;
    }

    // try all empty point
    for (y = 0; y < B_SIZE; y++)
        for (x = 0; x < B_SIZE; x++)
        {
            // 石を置く座標
            int z = get_z(x + 1, y + 1);

            // 空点でなければ無視
            if (board[z] != 0)
                continue;

            // 目潰ししないように石を置く
            err = put_stone(z, color, FILL_EYE_ERR);

            // もし石を置けなかったら、次の交点へ
            if (err != 0)
                continue;

            win_sum = 0;
            for (i = 0; i < try_num; i++)
            {
                // 現局面を退避
                int board_copy2[BOARD_MAX];
                int ko_z_copy2 = ko_z;
                memcpy(board_copy2, board, sizeof(board));

                // プレイアウト
                win = playout(flip_color(color));
                win_sum += win;

                // 現局面に復元
                ko_z = ko_z_copy2;
                memcpy(board, board_copy2, sizeof(board));
            }

            // 勝率
            win_rate = (double)win_sum / try_num;
            //  print_board();
            //  printf("z=%d,win=%5.3f\n",get81(z),win_rate);

            // 最善手の更新
            // select max value, black
            // select min value, white
            if ((color == 1 && win_rate > best_value) ||
                (color == 2 && win_rate < best_value))
            {
                best_value = win_rate;
                best_z = z;
                printf("best_z=%d,color=%d,v=%5.3f,try_num=%d\n", get81(best_z), color, best_value, try_num);
            }

            // コウの復元
            ko_z = ko_z_copy;
            // 盤の復元
            memcpy(board, board_copy, sizeof(board)); // resume board
        }

    return best_z;
}

/// <summary>
/// プログラムはここから始まります
/// </summary>
/// <returns>エラーコード。正常時は0</returns>
int main()
{
    // 黒番から
    int color = 1;
    int z, err, i;

    // 乱数の種を設定
    srand((unsigned)time(NULL));

    // 黒番、白番が１回ずつ原始モンテカルロでプレイアウトします
    for (i = 0; i < 2; i++)
    {
        z = primitive_monte_calro(color);
        err = put_stone(z, color, FILL_EYE_OK);
        print_board();
        color = flip_color(color);
    }

    return 0;
}
