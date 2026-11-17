#include <curses.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>
#include <string>

// ==========================================
// 2次元ベクトル(座標・移動量・サイズ)
// ==========================================

struct Size;
class PuyoGroup;

// 座標・移動量・盤面サイズは行と列で表す。行は下向き、列は右向きに増える。
struct Vec2 {
    protected:
        int r;
        int c;

        Vec2(int r, int c) : r(r), c(c) {}

    public:
        int getR() const { return r; }
        int getC() const { return c; }
};

// 座標同士の差や、移動・回転でずらす量を表す。
struct PosOffset : Vec2 {
    PosOffset(int r = 0, int c = 0) : Vec2(r, c) {}

    PosOffset operator+(const PosOffset& offset) const {
        return PosOffset(r + offset.getR(), c + offset.getC());
    }

    PosOffset operator-(const PosOffset& offset) const {
        return PosOffset(r - offset.getR(), c - offset.getC());
    }

    PosOffset operator*(int num) const {
        return PosOffset(num * r, num * c);
    }

    PosOffset operator/(int num) const {
        if (num == 0) throw std::invalid_argument("0では割れません");
        return PosOffset(r / num, c / num);
    }

    bool operator==(const PosOffset& other) const {
        return r == other.r && c == other.c;
    }

    bool operator<(const PosOffset& other) const {
        return r < other.r || (r == other.r && c < other.c);
    }
};

// 盤面や行列の中の位置を表す。
struct Position : Vec2 {
    Position(int r = 0, int c = 0) : Vec2(r, c) {}

    Position operator+(const PosOffset& offset) const {
        return Position(r + offset.getR(), c + offset.getC());
    }

    Position operator-(const PosOffset& offset) const {
        return Position(r - offset.getR(), c - offset.getC());
    }

    PosOffset operator-(const Position& other) const {
        return PosOffset(r - other.getR(), c - other.getC());
    }

    bool operator==(const Position& other) const {
        return r == other.r && c == other.c;
    }

    bool operator<(const Position& other) const {
        return r < other.r || (r == other.r && c < other.c);
    }

    Position center(const Position& other) const {
        return Position((r + other.getR()) / 2, (c + other.getC()) / 2);
    }

    bool isIn(const Size& size) const;
    bool isIn(const PuyoGroup& group) const;
};

// 行数と列数を表す。
struct Size : Vec2 {
    Size(int r = 0, int c = 0) : Vec2(r, c) {}

    PosOffset operator*(int num) const {
        return PosOffset(num * r, num * c);
    }

    PosOffset operator/(int num) const {
        if (num == 0) throw std::invalid_argument("0では割れません");
        return PosOffset(r / num, c / num);
    }

    bool operator==(const Size& other) const {
        return r == other.r && c == other.c;
    }

    bool operator!=(const Size& other) const {
        return !(*this == other);
    }

    Position center() const {
        return Position(r / 2, c / 2);
    }
};

bool Position::isIn(const Size& size) const {
    return r >= 0 && r < size.getR() && c >= 0 && c < size.getC();
}

// ==========================================
// 一次元配列・行列
// ==========================================

// 要素を一次元配列に保持し、添字でアクセスする。
template<class T>
class vector {
    protected:
        int num;
        T* x;

    public:
        vector() : num(0), x(nullptr) {}

        explicit vector(int n) : num(n), x(new T[n]()) {}

        vector(const vector<T>& other) : vector<T>(other.num) {
            for (int i = 0; i < num; i++) x[i] = other.x[i];
        }

        virtual ~vector() {
            delete[] x;
        }

        vector<T>& operator=(vector<T> other) {
            std::swap(num, other.num);
            std::swap(x, other.x);
            return *this;
        }

        vector<T> operator+(const vector<T>& v) const {
            return calculate(v, [](const T& a, const T& b) { return a + b; });
        }

        vector<T> operator-(const vector<T>& v) const {
            return calculate(v, [](const T& a, const T& b) { return a - b; });
        }

        const T& operator()(int i) const {
            if (i < 0 || i >= num) throw std::invalid_argument("存在しないindex");
            return x[i];
        }

        T& operator()(int i) {
            return const_cast<T&>(static_cast<const vector<T>&>(*this)(i));
        }

        int size() const { return num; }

    protected:
        template<class Func>
        vector<T> calculate(const vector<T>& v, Func func) const {
            if (num != v.num) throw std::invalid_argument("計算できないvector同士です");
            vector<T> result(num);
            for (int i = 0; i < num; i++) result.x[i] = func(x[i], v.x[i]);
            return result;
        }
};

// 行列を行優先で一次元配列に格納する。
template<class T>
class matrix : public vector<T> {
    protected:
        int R;
        int C;

        using vector<T>::num;
        using vector<T>::x;

    public:
        matrix() : vector<T>(), R(0), C(0) {}

        matrix(int rows, int cols) : vector<T>(rows * cols), R(rows), C(cols) {}

        explicit matrix(const Size& size) : matrix(size.getR(), size.getC()) {}

        matrix(const matrix<T>& other) : vector<T>(other), R(other.R), C(other.C) {}

        using vector<T>::operator();
        using vector<T>::size;

        const T& operator()(int r, int c) const {
            if (r < 0 || r >= R) throw std::invalid_argument("存在しないr");
            if (c < 0 || c >= C) throw std::invalid_argument("存在しないc");
            return x[r * C + c];
        }

        T& operator()(int r, int c) {
            return const_cast<T&>(static_cast<const matrix<T>&>(*this)(r, c));
        }

        const T& operator()(const Position& pos) const {
            return operator()(pos.getR(), pos.getC());
        }

        T& operator()(const Position& pos) {
            return const_cast<T&>(static_cast<const matrix<T>&>(*this)(pos));
        }

        matrix<T>& operator=(matrix<T> other) {
            std::swap(num, other.num);
            std::swap(x, other.x);
            std::swap(R, other.R);
            std::swap(C, other.C);
            return *this;
        }

        int rows() const { return R; }
        int cols() const { return C; }
        Size sizeObj() const { return Size(R, C); }

        std::map<Position, T> all() const {
            std::map<Position, T> result;
            for (int r = 0; r < R; r++) {
                for (int c = 0; c < C; c++) {
                    const Position pos(r, c);
                    result.emplace(pos, (*this)(pos));
                }
            }
            return result;
        }

        bool isIncluding(const Position& pos) const {
            return pos.isIn(sizeObj());
        }
    };

// ==========================================
// ぷよ集合・行列
// ==========================================

// DEFAULT は空きマスを表す。
enum puyocolor {
    DEFAULT = 0,
    RED,
    BLUE,
    GREEN,
    YELLOW
};

// 新しいぷよの色を赤・青・緑・黄から選ぶ。
puyocolor randColor() {
    return static_cast<puyocolor>(std::rand() % 4 + 1);
}

// 上下左右につながった同色のぷよを、盤面上の座標で保持する。
class PuyoGroup{
    public:
        puyocolor color;
        std::vector<Position> cells;

        PuyoGroup(const Position& firstCell, const puyocolor& color):
            color(color),
            cells(std::vector<Position>())
        {
            cells.push_back(firstCell);
        }

        int puyoNum()const { return cells.size(); }

};

bool Position::isIn(const PuyoGroup& group) const{
    return std::find(
        group.cells.begin(),
        group.cells.end(),
        *this
    )
       != group.cells.end();
}

// 着地して固定されたぷよを保持する盤面。
class PuyoStuck : public matrix<puyocolor> {
    public:
        using base = matrix<puyocolor>;
        using base::operator();

        PuyoStuck() : base() {}
        PuyoStuck(int rows, int cols) : base(rows, cols) {}
        explicit PuyoStuck(const Size& size) : base(size) {}
        PuyoStuck(const PuyoStuck& puyo) : base(puyo) {}

        int count() const {
            int result = 0;
            for (int i = 0; i < size(); i++) {
                if ((*this)(i) != DEFAULT) result++;
            }
            return result;
        }

        // 各列を下から調べ、浮いているぷよを1回の呼び出しにつき最大1マス落とす。
        bool breakDown() {
            bool moved = false;
            for (int c = 0; c < cols(); c++) {
                for (int r = rows() - 2; r >= 0; r--) {
                    if ((*this)(r, c) != DEFAULT && (*this)(r + 1, c) == DEFAULT) {
                        (*this)(r + 1, c) = (*this)(r, c);
                        (*this)(r, c) = DEFAULT;
                        moved = true;
                    }
                }
            }
            return moved;
        }

        // 同色の隣接マスを再帰的にたどり、登録済みのマスは再訪しない。
        void addNeighborPuyoToGroup(PuyoGroup& group, const Position& pos){
            std::vector<PosOffset> neighbors = {
                PosOffset(1, 0),
                PosOffset(0, 1),
                PosOffset(-1, 0),
                PosOffset(0, -1)
            };

            for(const PosOffset& neighbor : neighbors){
                Position neighborPos = pos + neighbor;
                if(neighborPos.isIn(sizeObj()) && !neighborPos.isIn(group) && (*this)(neighborPos) == group.color){
                    group.cells.push_back(neighborPos);
                    addNeighborPuyoToGroup(group, neighborPos);
                }
            }   
        }

        // 消去前の盤面でグループを確定し、4個以上のグループをまとめて消す。
        // 戻り値は消えたグループ数で、そのまま得点に加算する。
        int eracePuyo() {
            std::vector<PuyoGroup> groups = std::vector<PuyoGroup>();
            int eracedNum = 0;

            for(auto cell : all()){
                if(cell.second == DEFAULT) goto nextCell;

                for(const auto& group : groups){
                    if(cell.first.isIn(group)) goto nextCell;
                }

                {
                    PuyoGroup newGroup = PuyoGroup(cell.first, cell.second);
                    addNeighborPuyoToGroup(newGroup, cell.first);
                    groups.push_back(newGroup);
                }
                nextCell:
                ;
            }

            for(const auto& group : groups){
                if(group.puyoNum() >= 4){
                    for(const auto& pos : group.cells){
                        (*this)(pos) = DEFAULT;
                    }
                    eracedNum++;
                }
            }
            return eracedNum;
        }

        std::vector<int> surfaces() const {
            std::vector<int> result(cols(), rows());
            for (int c = 0; c < cols(); c++) {
                for (int r = 0; r < rows(); r++) {
                    if ((*this)(r, c) != DEFAULT) {
                        result[c] = r;
                        break;
                    }
                }
            }
            return result;
        }

        std::map<Position, puyocolor> allPuyo() const {
            std::map<Position, puyocolor> result;
            for (const auto& cell : all()) {
                if (cell.second != DEFAULT) result.insert(cell);
            }
            return result;
        }


    };

// 落下中の2個組を3×3の行列で表し、中央のぷよを回転軸にする。
class PuyoDrop : public matrix<puyocolor> {
    public:
        using base = matrix<puyocolor>;

    private:
        Position center_;

        public:
        PuyoDrop() : PuyoDrop(3) {
            (*this)(center_) = randColor();
            (*this)(center_ + PosOffset(0, 1)) = randColor();
        }

        explicit PuyoDrop(int size) : base(size, size), center_(size / 2, size / 2) {}
        explicit PuyoDrop(const Size& size) : base(size), center_(size.center()) {}
        PuyoDrop(const PuyoDrop& other) : base(other), center_(other.center_) {}

        PuyoDrop& operator=(const PuyoDrop& other) {
            if (this != &other) {
                base::operator=(other);
                center_ = other.center_;
            }
            return *this;
        }


        const Position& center() const { return center_; }

        // 元のオブジェクトを書き換えず、右回転後の候補を返す。
        PuyoDrop rotatedRightCandidate() const {
            PuyoDrop candidate(rows());
            for (int i = 0; i < candidate.size(); i++) candidate(i) = DEFAULT;

            for (const auto& cell : all()) {
                const Position destination(
                    cell.first.getC(),
                    rows() - 1 - cell.first.getR()
                );
                candidate(destination) = cell.second;
            }
            candidate.center_ = Position(center_.getC(), rows() - 1 - center_.getR());
            return candidate;
        }

        PuyoDrop rotatedLeftCandidate() const {
            PuyoDrop candidate(rows());
            for (int i = 0; i < candidate.size(); i++) candidate(i) = DEFAULT;

            for (const auto& cell : all()) {
                const Position destination(
                    cols() - 1 - cell.first.getC(),
                    cell.first.getR()
                );
                candidate(destination) = cell.second;
            }
            candidate.center_ = Position(cols() - 1 - center_.getC(), center_.getR());
            return candidate;
        }

        // 軸からの相対位置を返す。軸の盤面座標に足すと各ぷよの盤面座標になる。
        std::map<PosOffset, puyocolor> allPuyo() const {
            std::map<PosOffset, puyocolor> result;
            for (const auto& cell : all()) {
                if (cell.second != DEFAULT) result.emplace(cell.first - center_, cell.second);
            }
            return result;
        }
};

// ==========================================
// 端末の初期化と画面描画
// ==========================================

// ぷよの色を ncurses の色ペア番号・文字色・背景色に対応づける。
const std::vector<std::vector<short>> colorSets = {
    {static_cast<short>(DEFAULT), COLOR_WHITE, COLOR_BLACK},
    {static_cast<short>(RED), COLOR_RED, COLOR_BLACK},
    {static_cast<short>(BLUE), COLOR_BLUE, COLOR_BLACK},
    {static_cast<short>(YELLOW), COLOR_YELLOW, COLOR_BLACK},
    {static_cast<short>(GREEN), COLOR_GREEN, COLOR_BLACK}
};

void initColorPairs() {
    for (const std::vector<short>& colors : colorSets) {
        init_pair(colors[0], colors[1], colors[2]);
    }
}

void initScreen() {
    initscr();
    start_color();
    initColorPairs();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);  // 矢印キーを KEY_LEFT などのキーコードとして受け取る。
    // キー入力を待たず、入力がなくてもゲームを進める。
    timeout(0);
}

// 指定した端末座標に枠線と四隅を描く。
void drawRectangle(int r1, int c1, int r2, int c2, int color) {
    attrset(color);
    mvhline(r1, c1, 0, c2 - c1);
    mvhline(r2, c1, 0, c2 - c1);
    mvvline(r1, c1, 0, r2 - r1);
    mvvline(r1, c2, 0, r2 - r1);
    mvaddch(r1, c1, ACS_ULCORNER);
    mvaddch(r2, c1, ACS_LLCORNER);
    mvaddch(r1, c2, ACS_URCORNER);
    mvaddch(r2, c2, ACS_LRCORNER);
}

// 各画面は erase() で描画バッファを空にし、描き終えてから refresh() で反映する。
void displayStart(const Size& displaySize){
    erase();

    drawRectangle(1, 0, displaySize.getR() + 2, displaySize.getC() * 2 + 2, COLOR_PAIR(0));
    std::string title = "Puyo!";
    std::string message = "Press space";

    int centx = 2 * displaySize.center().getC() + 1;
    int centy = displaySize.center().getR() + 1;

    mvprintw(centy, centx - title.length() / 2, "%s", title.c_str());
    mvprintw(centy+1 , centx - message.length() / 2, "%s", message.c_str());
    refresh();
}

void displayGame(const matrix<puyocolor>& puyo, int score) {
    erase();

    drawRectangle(1, 0, puyo.rows() + 2, puyo.cols() * 2 + 2, COLOR_PAIR(0));

    // 盤面の1列を端末の2文字幅に割り当てて描画する。
    for (int r = 0; r < puyo.rows(); r++) {
        for (int c = 0; c < puyo.cols(); c++) {
            const short color = static_cast<short>(puyo(r, c));
            attrset(COLOR_PAIR(color));
            char symbol = '?';
            switch (color) {
                case DEFAULT: symbol = '.'; break;
                case RED: symbol = 'R'; break;
                case BLUE: symbol = 'B'; break;
                case GREEN: symbol = 'G'; break;
                case YELLOW: symbol = 'Y'; break;
                default: break;
            }
            mvaddch(r + 2, c * 2 + 2, symbol);
        }
    }

    char message[256];
    snprintf(message, sizeof(message), "Score: %d", score);
    mvaddstr(0, 1, message);
    refresh();
}

void displayPause(const Size& displaySize){
    erase();

    drawRectangle(1, 0, displaySize.getR() + 2, displaySize.getC() * 2 + 2, COLOR_PAIR(0));

    std::string title = "Pausing!";

    int centx = 2 * displaySize.center().getC() + 1;
    int centy = displaySize.center().getR() + 1;

    mvprintw(centy, centx - title.length() / 2, "%s", title.c_str());
    refresh();
}

void displayResult(const Size& displaySize, int score){
    erase();

    drawRectangle(1, 0, displaySize.getR() + 2, displaySize.getC() * 2 + 2, COLOR_PAIR(0));
    std::string title = "GameFinish!";

    int centx = 2 * displaySize.center().getC() + 1;
    int centy = displaySize.center().getR() + 1;

    mvprintw(centy, centx - title.length() / 2, "%s", title.c_str());
    mvprintw(centy + 1, centx - title.length() / 2, "score:%d", score);
    refresh();
}

// ==========================================
// プレイの進行と状態管理
// ==========================================

// ぷよの出現・操作・着地後の処理を状態に応じて進める。
class PuyoOperator {
    private:

        enum GamePlayState {
            Spawn,
            Control,
            Settle,
            Over
        };

        PuyoStuck stuck_;
        PuyoDrop drop_;
        Position dropPos_;

        GamePlayState state_;

        int waitCount_ = 6;
        int timer_;
        int score_ = 0;

        std::vector<Position> overlapped(const PuyoDrop& drop, const Position& dropPos) const {
            std::vector<Position> result;
            for (const auto& dropPuyo : drop.allPuyo()) {
                const Position position = dropPos + dropPuyo.first;
                if (position.isIn(stuck_.sizeObj()) && stuck_(position) != DEFAULT) {
                    result.push_back(position);
                }
            }
            return result;
        }

        std::vector<Position> outPuyo(const PuyoDrop& drop, const Position& dropPos) const {
            std::vector<Position> result;
            for (const auto& dropPuyo : drop.allPuyo()) {
                const Position position = dropPos + dropPuyo.first;
                if (!position.isIn(stuck_.sizeObj())) result.push_back(position);
            }
            return result;
        }

        // 色の付いたセルがすべて盤面内にあり、固定済みのぷよと重ならなければ置ける。
        bool canPlace(const PuyoDrop& drop, const Position& dropPos) const {
            return overlapped(drop, dropPos).empty() && outPuyo(drop, dropPos).empty();
        }

        bool moveDrop(const PosOffset& offset) {
            const Position candidate = dropPos_ + offset;
            if (!canPlace(drop_, candidate)) return false;
            dropPos_ = candidate;
            return true;
        }

        bool isDropped() const {
            return !canPlace(drop_, dropPos_ + PosOffset(1, 0));
        }

        void rotateDrop(){
            const PuyoDrop rotated = drop_.rotatedRightCandidate();

            // まずその場で回転できるなら、そのまま回転する。
            if (canPlace(rotated, dropPos_)) {
                drop_ = rotated;
                return;
            }

            // 回転後に盤面の外へ出る、または固定済みのぷよに重なるセルを調べる。
            // 軸のどちら側が塞がっているかを壁蹴り候補の絞り込みに使う。
            bool failedOnRight = false;
            bool failedOnLeft = false;

            for (const auto& cell : rotated.all()) {
                if (cell.second == DEFAULT) continue;

                const PosOffset local = cell.first - rotated.center();
                const Position position = dropPos_ + local;
                const bool failed =
                    !position.isIn(stuck_.sizeObj()) ||
                    stuck_(position) != DEFAULT;

                if (!failed) continue;

                if (local.getC() > 0) failedOnRight = true;
                if (local.getC() < 0) failedOnLeft = true;
            }

            // 3×3の行列の各座標を軸からの移動量に変換し、壁蹴り候補にする。
            std::vector<PosOffset> kicks;
            for (const auto& cell : drop_.all()) {
                const PosOffset kick = cell.first - drop_.center();

                // 下方向への壁蹴りはしない。
                if (kick.getR() > 0) continue;

                // 失敗原因が右なら右移動、左なら左移動を候補から外す。
                if (failedOnRight && kick.getC() > 0) continue;
                if (failedOnLeft && kick.getC() < 0) continue;

                // その場回転は既に失敗しているので (0, 0) は不要。
                if (kick.getR() == 0 && kick.getC() == 0) continue;

                kicks.push_back(kick);
            }

            // r が大きいもの（より下の配置）を優先し、
            // 同じ r なら移動距離が短いものを優先する。
            std::sort(kicks.begin(), kicks.end(),
                [](const PosOffset& a, const PosOffset& b) {
                    if (a.getR() != b.getR())
                        return a.getR() > b.getR();

                    const int aDistance =
                        a.getR() * a.getR() + a.getC() * a.getC();
                    const int bDistance =
                        b.getR() * b.getR() + b.getC() * b.getC();

                    return aDistance < bDistance;
                }
            );

            for (const PosOffset& kick : kicks) {
                const Position positionCandidate = dropPos_ + kick;

                if (canPlace(rotated, positionCandidate)) {
                    drop_ = rotated;
                    dropPos_ = positionCandidate;
                    return;
                }
            }

            // 移動できる候補がなければ回転をキャンセルする。
        }

        void merge() {
            stuck_ = merged();
        }

        // 描画用のコピーに落下中の組を重ねることで、固定済みの盤面を保つ。
        PuyoStuck merged() const {
            PuyoStuck result(stuck_);
            for (const auto& dropPuyo : drop_.allPuyo()) {
                const Position position = dropPos_ + dropPuyo.first;
                if (position.isIn(stuck_.sizeObj())) result(position) = dropPuyo.second;
            }
            return result;
        }

        void spawnDrop(){
            if(state_ != GamePlayState::Spawn) return;

            PuyoDrop drop;
            Position dropPos(0, (stuck_.cols() - 1) / 2);

            if (canPlace(drop, dropPos)){
                drop_ = drop;
                dropPos_ = dropPos;

                state_ = GamePlayState::Control;
            }else{
                state_ = GamePlayState::Over;
            }
        }

        void updateDrop(int ch){
            if(state_ != GamePlayState::Control) return;

            switch (ch) {
                case KEY_LEFT:
                case 'a':
                case 'A':
                    moveDrop(PosOffset(0, -1));
                    break;

                case KEY_RIGHT:
                case 'd':
                case 'D':
                    moveDrop(PosOffset(0, 1));
                    break;

                case KEY_UP:
                case 'w':
                case 'W':
                    rotateDrop();
                    break;

                case KEY_DOWN:
                case 's':
                case 'S':
                    moveDrop(PosOffset(1, 0));
                    break;

                default:
                    break;
            }

            // 6回の画面更新ごとに自動で1マス落とす。
            if (timer_ % waitCount_ == 0) {
                moveDrop(PosOffset(1, 0));
            }

            if(isDropped()){
                merge();
                state_ = GamePlayState::Settle;
            } 
        }

        void updateSettle(){
            if(state_ != GamePlayState::Settle) return;

            if (timer_ % waitCount_ != 0) return;
            // 落下が止まってから消去を判定する。消去が起きたら次の更新で連鎖を調べる。
            if(stuck_.breakDown()) return;
                
            int eracedNum = stuck_.eracePuyo();
            score_ += eracedNum;
            
            if(eracedNum == 0) state_ = GamePlayState::Spawn;
        }

        void display(){
            matrix<puyocolor> rendererd = (
                (state_ == GamePlayState::Control)?
                merged() :
                stuck_
            );
            displayGame(rendererd, score_);
        }

    public:

        void Update(int ch){
            switch(state_){
                case GamePlayState::Spawn:
                    spawnDrop();
                    break;
                case GamePlayState::Control:
                    updateDrop(ch);
                    break;
                case GamePlayState::Settle:
                    updateSettle();
                    break;
                case GamePlayState::Over:
                    break;
            }
            display();
            timer_++;
        }

        explicit PuyoOperator(const Size& boardSize = Size(12, 6), int waitCount = 6) :
            stuck_(boardSize),
            drop_(),
            dropPos_(0, (stuck_.cols() - 1) / 2),
            state_(GamePlayState::Spawn),
            timer_(0),
            score_(0),
            waitCount_(waitCount)
        {}

        bool getIsOver() {return state_ == GamePlayState::Over;}
        int getScore() {return score_;}

};

// 表示中の画面に応じてキー入力を処理し、画面を切り替える。
class GameController{

    private:
        const Size DISPLAYSIZE;

        enum Phase {
            TITLE,
            PLAYING,
            PAUSE,
            RESULT,
            END
        };

        Phase phase_;
        PuyoOperator puyoOp_;

        void UpdateTitle(int ch){
            if(phase_ != Phase::TITLE) return;

            switch(ch){
                // プレイスタート
                case ' ':
                    puyoOp_ = PuyoOperator(DISPLAYSIZE);
                    phase_ = Phase::PLAYING;
                    break;
                // プログラム実行終了
                case 'q':
                case 'Q':
                    phase_ = Phase::END;
                    break;
            }
        }

        void UpdatePlaying(int ch){
            if(phase_ != Phase::PLAYING) return;

            switch(ch){
                // ポーズ
                case 'q':
                case 'Q':
                    displayPause(DISPLAYSIZE);
                    phase_ = Phase::PAUSE;
                    break;
                // プレイ操作
                default:
                    puyoOp_.Update(ch);
                    break;
            }

            if(puyoOp_.getIsOver()) {
                displayResult(DISPLAYSIZE, puyoOp_.getScore());
                phase_ = Phase::RESULT;
            }
        }

        void UpdatePause(int ch){
            if(phase_ != Phase::PAUSE) return;

            switch(ch){
                // プレイ再開
                case ' ':
                    phase_ = Phase::PLAYING;
                    break;
                //リザルトへ
                case 'q':
                case 'Q':
                    displayResult(DISPLAYSIZE, puyoOp_.getScore());
                    phase_ = Phase::RESULT;
                    break;
                // リスタート
                case 'r':
                case 'R':
                    puyoOp_ = PuyoOperator(DISPLAYSIZE);
                    phase_ = Phase::PLAYING;
                    break;
            }
        }

        void UpdateResult(int ch){
            if(phase_ != Phase::RESULT) return;

            switch(ch){
                // リスタート
                case 'r':
                case 'R':
                    puyoOp_ = PuyoOperator(DISPLAYSIZE);
                    phase_ = Phase::PLAYING;
                    break;
                //タイトル画面へ
                case ' ':
                case 'q':
                case 'Q':
                    displayStart(DISPLAYSIZE);
                    phase_ = Phase::TITLE;
                    break;
            }
        }

    public:
        explicit GameController(const Size& displaysize):
            phase_(TITLE),
            puyoOp_(displaysize),
            DISPLAYSIZE(displaysize)
        {
            displayStart(displaysize);
        }

        void Update(int ch){
            switch (phase_){
                case Phase::TITLE:
                    UpdateTitle(ch);
                    break;

                case Phase::PLAYING:
                    UpdatePlaying(ch);
                    break;

                case Phase::PAUSE:
                    UpdatePause(ch);
                    break;

                case Phase::RESULT:
                    UpdateResult(ch);
                    break;

                default:
                    break;
            }
            
        }

        bool getIsEnd(){return phase_ == Phase::END;}
};

// ==========================================
// ゲームループとエントリーポイント
// ==========================================

// 操作中の画面更新間隔（マイクロ秒）。
#define GMCLOCK 50000

// タイトル画面で終了が選ばれるまで、入力の取得とゲームの更新を繰り返す。
void runGame(){
    Size displaySize(12,6);
    GameController gamecontroller(displaySize);
    int ch;
    while(!gamecontroller.getIsEnd()){
        ch = getch();
        gamecontroller.Update(ch);
        usleep(GMCLOCK);
    }
}

// 乱数と端末を初期化し、ゲーム終了時に端末の設定を戻す。
int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    initScreen();

    runGame();

    endwin();
    return 0;
}
