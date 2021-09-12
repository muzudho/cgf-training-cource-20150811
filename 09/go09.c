#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

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
/// プレイアウト回数
/// </summary>
int all_playouts = 0;

/// <summary>
/// テストでプレイアウトするなら1
/// </summary>
int flag_test_playout = 0;

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
        z = tz + dir4[i];
        if (check_board[z])
            continue;
        if (board[z] == 0)
        {
            check_board[z] = 1;
            (*p_liberty)++; // number of liberty
        }
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
            printf("  ko_z=%d,moves=%d", get81(ko_z), moves);
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
    // 手番の勝ちなら1、負けなら0
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

    // 白番なら勝敗を反転
    if (turn_color == 2)
        win = -win;

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

    all_playouts++;

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

        // テストでプレイアウトするのなら、棋譜に手を記録します
        if (flag_test_playout)
            record[moves++] = z;

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
/// 原始モンテカルロ（ネガマックス）
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
    // 手番が勝ったら1、負けたら0
    int win;

    // コウの座標のコピー
    int ko_z_copy;

    // 盤のコピー
    int board_copy[BOARD_MAX]; // keep current board
    ko_z_copy = ko_z;
    memcpy(board_copy, board, sizeof(board));

    // 根ノードでは最低-100点から
    best_value = -100;

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
                win = -playout(flip_color(color));
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
            if (win_rate > best_value)
            {
                best_value = win_rate;
                best_z = z;
                //    printf("best_z=%d,color=%d,v=%5.3f,try_num=%d\n",get81(best_z),color,best_value,try_num);
            }

            // コウの復元
            ko_z = ko_z_copy;
            // 盤の復元
            memcpy(board, board_copy, sizeof(board)); // resume board
        }

    return best_z;
}

// following are for UCT(Upper Confidence Tree)
// `UCT` - 探索と知識利用のバランスを取る手法

/// <summary>
/// 手を保存するための構造体
/// </summary>
typedef struct
{
    /// <summary>
    /// 手の場所（move position）
    /// </summary>
    int z;

    /// <summary>
    /// 試した回数（number of games）
    /// </summary>
    int games;

    /// <summary>
    /// 勝率（winrate）
    /// </summary>
    double rate;

    /// <summary>
    /// ノードのリストのインデックス。次のノード（next node）を指す
    /// </summary>
    int next;
} CHILD;

// 最大の子数。9路なら82個。+1 for PASS
#define CHILD_SIZE (B_SIZE * B_SIZE + 1)

/// <summary>
/// 局面を保存する構造体
/// </summary>
typedef struct
{
    /// <summary>
    /// 実際の子どもの数
    /// </summary>
    int child_num;
    CHILD child[CHILD_SIZE];
    /// <summary>
    /// 何回このノードに来たか（子の合計）
    /// </summary>
    int child_games_sum;
} NODE;

// 以下、探索木全体を保存

// 最大10000局面まで
#define NODE_MAX 10000

/// <summary>
/// ノードのリスト
/// </summary>
NODE node[NODE_MAX];

/// <summary>
/// ノードのリストのサイズ。登録局面数
/// </summary>
int node_num = 0;

/// <summary>
/// no next node
/// </summary>
const int NODE_EMPTY = -1;

/// <summary>
/// illegal move
/// </summary>
const int ILLEGAL_Z = -1;

/// <summary>
/// リストの末尾に要素を追加。手を追加。
/// この手を打った後のノードは、なし
/// </summary>
/// <param name="pN">局面</param>
/// <param name="z">手の座標</param>
void add_child(NODE* pN, int z)
{
    // 新しい要素のインデックス
    int n = pN->child_num;
    pN->child[n].z = z;
    pN->child[n].games = 0;
    pN->child[n].rate = 0;
    pN->child[n].next = NODE_EMPTY;
    // ノードのリストのサイズ更新
    pN->child_num++;
}

/// <summary>
/// create new node.
/// 空点を全部追加。
/// PASSも追加。
/// </summary>
/// <returns>ノードのリストのインデックス。作られたノードを指す。最初は0から</returns>
int create_node()
{
    int x, y, z;
    NODE* pN;

    // これ以上増やせません
    if (node_num == NODE_MAX)
    {
        printf("node over Err\n");
        exit(0);
    }

    // 末尾の未使用の要素
    pN = &node[node_num];
    pN->child_num = 0;
    pN->child_games_sum = 0;

    // 空点をリストの末尾に追加
    for (y = 0; y < B_SIZE; y++)
        for (x = 0; x < B_SIZE; x++)
        {
            z = get_z(x + 1, y + 1);
            if (board[z] != 0)
                continue;
            add_child(pN, z);
        }
    add_child(pN, 0); // add PASS

    // 末尾に１つ追加した分、リストのサイズ１つ追加
    node_num++;

    // 最後の要素を指すインデックスを返します
    return node_num - 1;
}

/// <summary>
/// UCBが最大の手を返します。
/// 一度も試していない手は優先的に選びます。
/// 定数 Ｃ は実験で決めてください。
/// PASS があるので、すべての手がエラーはありえません。
/// </summary>
/// <param name="node_n">ノードのリストのインデックス</param>
/// <returns>ノードのリストのインデックス。選択した子ノードを指します</returns>
int select_best_ucb(int node_n)
{
    NODE* pN = &node[node_n];
    int select = -1;
    double max_ucb = -999;
    double ucb = 0;
    int i;

    // 子要素の数だけ繰り返します
    for (i = 0; i < pN->child_num; i++)
    {
        CHILD* c = &pN->child[i];

        // 非合法手の座標なら無視
        if (c->z == ILLEGAL_Z)
            continue;

        if (c->games == 0)
        {
            ucb = 10000 + (rand() & 0x7fff); // try once
        }
        else
        {
            const double C = 1; // depends on program
            ucb = c->rate + C * sqrt(log((double)pN->child_games_sum) / c->games);
        }

        // UCB値の最大を更新
        if (ucb > max_ucb)
        {
            max_ucb = ucb;
            select = i;
        }
    }
    if (select == -1)
    {
        printf("Err! select\n");
        exit(0);
    }
    return select;
}

/// <summary>
/// ゲームをプレイします（再帰関数）
/// UCTという探索の手法で行います
/// </summary>
/// <param name="color">手番の色。最初は考えているプレイヤーの色</param>
/// <param name="node_n">ノードのリストのインデックス。最初は0</param>
/// <returns>手番の勝率</returns>
int search_uct(int color, int node_n)
{
    // この局面
    NODE* pN = &node[node_n];

    // 最善の一手（子ノード）
    CHILD* c = NULL;
    int select, z, err, win;

    // とりあえず打ってみる
    for (;;)
    {
        // 最善の一手（子ノード）のインデックス
        select = select_best_ucb(node_n);
        // 最善の一手（子ノード）
        c = &pN->child[select];
        // 最善の一手（子ノード）の座標
        z = c->z;
        // 石を置く
        err = put_stone(z, color, FILL_EYE_ERR);
        // 合法手ならループを抜けます
        if (err == 0)
            break;
        // 非合法手なら、 ILLEGAL_Z をセットして ループをやり直し
        c->z = ILLEGAL_Z; // select other move
    }

    // この一手が１度も試行されていなければ、プレイアウトします
    // c->games <= 10 とかにすればメモリを節約できます。
    // c->games <= 0 より強くなる場合もあります。
    // playout in first time. <= 10 can reduce node.
    if (c->games <= 0)
    {
        // 手番をひっくり返してプレイアウト
        win = -playout(flip_color(color));
    }
    // この一手が既に試行されていれば、（プレイアウトではなく）search_uct します。
    else
    {
        // 子ノードが葉なら、さらに延長
        if (c->next == NODE_EMPTY)
            c->next = create_node();

        // 手番をひっくり返して UCT探索（ネガマックス形式）。勝率はひっくり返して格納
        win = -search_uct(flip_color(color), c->next);
    }

    // 勝率の更新（update winrate）
    c->rate = (c->rate * c->games + win) / (c->games + 1);

    // 対局数カウントアップ
    c->games++;
    pN->child_games_sum++;

    return win;
}

/// <summary>
/// number of uct loop
/// </summary>
int uct_loop = 1000;

/// <summary>
/// 一番良く打たれた一手の座標を返します
/// </summary>
/// <param name="color">手番の色</param>
/// <returns>一番良く打たれた一手の座標</returns>
int get_best_uct(int color)
{
    int next, i, best_z, best_i = -1;
    int max = -999;
    NODE* pN;

    // ノードリストの要素数
    node_num = 0;
    // 次のノードのインデックス。ここでは0。現図を作成しています
    next = create_node();

    // とりあえず UCT探索（search_uct）を、uct_loop回繰り返します
    for (i = 0; i < uct_loop; i++)
    {
        // 現図を退避
        int board_copy[BOARD_MAX];
        int ko_z_copy = ko_z;
        memcpy(board_copy, board, sizeof(board));

        // UCT探索
        search_uct(color, next);

        // 現図を復元
        ko_z = ko_z_copy;
        memcpy(board, board_copy, sizeof(board));
    }
    // 次のノード
    pN = &node[next];
    // 子ノード全部確認
    for (i = 0; i < pN->child_num; i++)
    {
        // 子ノード
        CHILD* c = &pN->child[i];
        // 最大対局数（一番打たれた手ということ）の更新
        if (c->games > max)
        {
            best_i = i;
            max = c->games;
        }
        //  printf("%2d:z=%2d,rate=%.4f,games=%3d\n",i, get81(c->z), c->rate, c->games);
    }
    // ベストなノードの座標
    best_z = pN->child[best_i].z;
    printf("best_z=%d,rate=%.4f,games=%d,playouts=%d,nodes=%d\n",
        get81(best_z), pN->child[best_i].rate, max, all_playouts, node_num);

    return best_z;
}

/// <summary>
/// 指し手を棋譜に記入
/// </summary>
/// <param name="z">座標</param>
/// <param name="color">手番の色</param>
void add_moves(int z, int color)
{
    // 石を置きます
    int err = put_stone(z, color, FILL_EYE_OK);
    // 非合法手なら強制終了
    if (err != 0)
    {
        printf("Err!\n");
        exit(0);
    }
    // 棋譜の末尾に記入
    record[moves] = z;
    // 棋譜のサイズを伸ばします
    moves++;
    // 盤表示
    print_board();
}

/// <summary>
/// コンピューターの次の一手の座標を取得
/// </summary>
/// <param name="color">手番の色</param>
/// <param name="fUCT">UCTの手法を使ってゲームプレイするか</param>
/// <returns>座標</returns>
int get_computer_move(int color, int fUCT)
{
    // 現在時刻
    clock_t st = clock();
    // 思考時間（秒）
    double t;
    // 座標
    int z;

    // プレイアウト回数
    all_playouts = 0;
    if (fUCT)
    {
        // UCTを使ったゲームプレイ
        z = get_best_uct(color);
    }
    else
    {
        // 原始モンテカルロでゲームプレイ
        z = primitive_monte_calro(color);
    }
    // 消費時間（秒）
    t = (double)(clock() + 1 - st) / CLOCKS_PER_SEC;
    // 情報表示
    printf("%.1f sec, %.0f playout/sec, play_z=%2d,moves=%d,color=%d,playouts=%d\n",
        t, all_playouts / t, get81(z), moves, color, all_playouts);

    return z;
}

/// <summary>
/// print SGF game record
/// </summary>
void print_sgf()
{
    int i;

    // ヘッダー出力
    printf("(;GM[1]SZ[%d]KM[%.1f]PB[]PW[]\n", B_SIZE, komi);

    // 指し手出力
    for (i = 0; i < moves; i++)
    {
        int z = record[i];
        // 段
        int y = z / WIDTH;
        // 筋
        int x = z - y * WIDTH;
        // 色
        const char* sStone[2] = { "B", "W" };
        printf(";%s", sStone[i & 1]);
        // パス
        if (z == 0)
        {
            printf("[]");
        }
        else
        {
            printf("[%c%c]", x + 'a' - 1, y + 'a' - 1);
        }
        // 改行
        if (((i + 1) % 10) == 0)
            printf("\n");
    }

    // 終端
    printf(")\n");
}

/// <summary>
/// 黒番は原始モンテカルロ、白番はUCTで自己対戦
/// </summary>
void selfplay()
{
    // 黒の手番
    int color = 1;
    // 座標
    int z;

    for (;;)
    {
        // 白の手番では UCT、黒の手番では 原始モンテカルロでゲームプレイします
        int fUCT = 1;
        if (color == 1)
            fUCT = 0;

        // 次の一手
        z = get_computer_move(color, fUCT);
        // 棋譜に書込み
        add_moves(z, color);
        // パスパスなら終局
        if (z == 0 && moves > 1 && record[moves - 2] == 0)
            break;
        // 300手を超えても終局
        if (moves > 300)
            break; // too long
        // 手番の色反転
        color = flip_color(color);
    }

    // SGF形式の棋譜を出力
    print_sgf();
}

/// <summary>
/// 黒手番でプレイアウトのテスト
/// </summary>
void test_playout()
{
    flag_test_playout = 1;
    // 黒手番でプレイアウト
    playout(1);
    // 盤表示
    print_board();
    // SGF形式の棋譜を出力
    print_sgf();
}

/// <summary>
/// プログラムはここから始まります
/// </summary>
/// <returns>エラーコード。正常時は0</returns>
int main()
{
    // 乱数の種を設定
    srand((unsigned)time(NULL));

    //test_playout(); return 0;

    // 自己対戦
    selfplay();

    return 0;
}
