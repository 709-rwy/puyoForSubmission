#include <curses.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

#define GMCLOCK 50000

enum puyocolor {
    NONE = 0,
    RED,
    BLUE,
    GREEN,
    YELLOW
};

const std::vector<std::vector<short>> colorSets = {
    {static_cast<short>(NONE), COLOR_WHITE, COLOR_BLACK},
    {static_cast<short>(RED), COLOR_RED, COLOR_BLACK},
    {static_cast<short>(BLUE), COLOR_BLUE, COLOR_BLACK},
    {static_cast<short>(YELLOW), COLOR_YELLOW, COLOR_BLACK},
    {static_cast<short>(GREEN), COLOR_GREEN, COLOR_BLACK}
};

struct Size;
class PuyoGroup;

struct Vec2 {
    protected:
        int r;
        int c;

        Vec2(int r, int c) : r(r), c(c) {}

    public:
        int getR() const { return r; }
        int getC() const { return c; }
};

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

puyocolor randColor() {
    return static_cast<puyocolor>(std::rand() % 4 + 1);
}

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
                if ((*this)(i) != NONE) result++;
            }
            return result;
        }

        // 1フレーム分だけ浮いているぷよを下へ移動する。
        bool breakDown() {
            bool moved = false;
            for (int c = 0; c < cols(); c++) {
                for (int r = rows() - 2; r >= 0; r--) {
                    if ((*this)(r, c) != NONE && (*this)(r + 1, c) == NONE) {
                        (*this)(r + 1, c) = (*this)(r, c);
                        (*this)(r, c) = NONE;
                        moved = true;
                    }
                }
            }
            return moved;
        }

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

        int eracePuyo() {
            std::vector<PuyoGroup> groups = std::vector<PuyoGroup>();
            int eracedNum = 0;

            for(auto cell : all()){
                if(cell.second == NONE) goto nextCell;

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
                        (*this)(pos) = NONE;
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
                    if ((*this)(r, c) != NONE) {
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
                if (cell.second != NONE) result.insert(cell);
            }
            return result;
        }


};

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
            for (int i = 0; i < candidate.size(); i++) candidate(i) = NONE;

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
            for (int i = 0; i < candidate.size(); i++) candidate(i) = NONE;

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

        std::map<PosOffset, puyocolor> allPuyo() const {
            std::map<PosOffset, puyocolor> result;
            for (const auto& cell : all()) {
                if (cell.second != NONE) result.emplace(cell.first - center_, cell.second);
            }
            return result;
        }
};

void drawRectangle(int r1, int c1, int r2, int c2, int color);
void display(const matrix<puyocolor>& puyo, int score);
void initColorPairs();
void initScreen();

class PuyoOperator {
    private:
        PuyoStuck stuck_;

        std::vector<Position> overlapped(const PuyoDrop& drop, const Position& dropPos) const {
            std::vector<Position> result;
            for (const auto& dropPuyo : drop.allPuyo()) {
                const Position position = dropPos + dropPuyo.first;
                if (position.isIn(stuck_.sizeObj()) && stuck_(position) != NONE) {
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

        bool canPlace(const PuyoDrop& drop, const Position& dropPos) const {
            return overlapped(drop, dropPos).empty() && outPuyo(drop, dropPos).empty();
        }

        bool moveDrop(const PuyoDrop& drop, Position& dropPos, const PosOffset& offset) const {
            const Position candidate = dropPos + offset;
            if (!canPlace(drop, candidate)) return false;
            dropPos = candidate;
            return true;
        }

        bool isDropped(const PuyoDrop& drop, const Position& dropPos) const {
            return !canPlace(drop, dropPos + PosOffset(1, 0));
        }

        void rotateDrop(PuyoDrop& drop, Position& dropPos) const {
            const PuyoDrop rotated = drop.rotatedRightCandidate();

            // まずその場で回転できるなら、そのまま回転する。
            if (canPlace(rotated, dropPos)) {
                drop = rotated;
                return;
            }

            // その場での回転を妨げているぷよの、中心に対する左右位置を調べる。
            bool failedOnRight = false;
            bool failedOnLeft = false;

            for (const auto& cell : rotated.all()) {
                if (cell.second == NONE) continue;

                const PosOffset local = cell.first - rotated.center();
                const Position position = dropPos + local;
                const bool failed =
                    !position.isIn(stuck_.sizeObj()) ||
                    stuck_(position) != NONE;

                if (!failed) continue;

                if (local.getC() > 0) failedOnRight = true;
                if (local.getC() < 0) failedOnLeft = true;
            }

            // drop 内の各セルの Position - center を壁蹴り候補にする。
            std::vector<PosOffset> kicks;
            for (const auto& cell : drop.all()) {
                const PosOffset kick = cell.first - drop.center();

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
                const Position positionCandidate = dropPos + kick;

                if (canPlace(rotated, positionCandidate)) {
                    drop = rotated;
                    dropPos = positionCandidate;
                    return;
                }
            }

            // 移動できる候補がなければ回転をキャンセルする。
        }

        void merge(const PuyoDrop& drop, const Position& dropPos) {
            for (const auto& dropPuyo : drop.allPuyo()) {
                const Position position = dropPos + dropPuyo.first;
                if (position.isIn(stuck_.sizeObj())) stuck_(position) = dropPuyo.second;
            }
        }

        matrix<puyocolor> renderedMatrix(const PuyoDrop& drop, const Position& dropPos) const {
            matrix<puyocolor> result(stuck_);
            for (const auto& dropPuyo : drop.allPuyo()) {
                const Position position = dropPos + dropPuyo.first;
                if (position.isIn(stuck_.sizeObj())) result(position) = dropPuyo.second;
            }
            return result;
        }

    public:
        explicit PuyoOperator(const Size& boardSize = Size(12, 6)) : stuck_(boardSize) {}

        void gameOperate() {
            bool loop = true;
            int delay = 0;
            const int waitCount = 6;
            int score = 0;

            while (loop) {
                PuyoDrop drop;
                Position dropPos(0, (stuck_.cols() - 1) / 2);

                // 生成位置がすでに塞がれていればゲームオーバー扱い。
                if (!canPlace(drop, dropPos)) break;

                while (!isDropped(drop, dropPos)) {
                    const int ch = getch();
                    switch (ch) {
                        case KEY_LEFT:
                            moveDrop(drop, dropPos, PosOffset(0, -1));
                            break;

                        case KEY_RIGHT:
                            moveDrop(drop, dropPos, PosOffset(0, 1));
                            break;

                        case KEY_UP:
                            rotateDrop(drop, dropPos);
                            break;

                        case KEY_DOWN:
                            moveDrop(drop, dropPos, PosOffset(1, 0));
                            break;

                        case 'q':
                        case 'Q':
                            loop = false;
                            break;

                        default:
                            break;
                    }

                    if (!loop) break;

                    if (++delay % waitCount == 0) {
                        moveDrop(drop, dropPos, PosOffset(1, 0));
                    }

                    display(renderedMatrix(drop, dropPos), score);
                    usleep(GMCLOCK);
                }

                if (!loop) break;

                merge(drop, dropPos);
                while(true){
                    while (stuck_.breakDown()) {
                        display(stuck_, score);
                        usleep(GMCLOCK * waitCount);
                    }
                    int eracedNum = stuck_.eracePuyo();
                    if(eracedNum == 0) break; 
                    score += eracedNum;
                }

                display(stuck_, score);
            }
        }
};

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

void display(const matrix<puyocolor>& puyo, int score) {
    drawRectangle(1, 0, puyo.rows() + 2, puyo.cols() * 2 + 2, COLOR_PAIR(0));

    for (int r = 0; r < puyo.rows(); r++) {
        for (int c = 0; c < puyo.cols(); c++) {
            const short color = static_cast<short>(puyo(r, c));
            attrset(COLOR_PAIR(color));
            char symbol = '?';
            switch (color) {
                case NONE: symbol = '.'; break;
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
    keypad(stdscr, TRUE);
    timeout(0);
}

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    initScreen();

    PuyoOperator game(Size(12, 6));
    game.gameOperate();

    endwin();
    return 0;
}
