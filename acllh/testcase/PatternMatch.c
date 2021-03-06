#include "PatternMatch.h"
#include "ImgUtility.h"
#include "ImgProc.h"
#include "Motor.h"
#include "stdio.h"
#include "uart.h"
#include "gpio.h"

/*
 *
 */

int32_t ringDistance;
bool inRing;
bool ringEndDelay;

int16_t barrierType;
int32_t barrierDistance;
bool aroundBarrier;

static bool IsRing(void);
static bool IsRingEnd(void);
static int16_t WhichCurve(void);
static bool IsCrossRoad(void);
static int16_t WhichBarrier(void);
static int16_t CrossRoadCompensateOneSide(int16_t borders[IMG_ROW], int16_t slopes[IMG_ROW], int16_t zeros[IMG_ROW]);

static inline float Abs(float);
static inline float Min(float, float);
static inline float Max(float, float);
static inline bool OpstSign(int16_t, int16_t);
static inline bool InRange(int16_t value, int16_t lbound, int16_t hbound);

static int16_t black_pt_row;
static int16_t last_not_found_border_row;

bool OutOfRoadJudge() {
    int16_t cnt = 0;
    int16_t line = 0;
    for(int16_t row = 0; row < 10; ++row) {
        for(int16_t col = 60; col < IMG_COL - 60; ++col) {
            if(IsBlack(row, col)) {
                ++cnt;
            }
        }
        if(cnt >= 85) {
            ++line;
            if(line >= 4) {
                return true;
            }
        }
        cnt = 0;
    }
    return false;
}

int16_t GetRoadType() {
    // de-warning
    Min(0, 0);
    
    if(ringEndDelay) {
        if(ringDistance < 10000) {
            return RingEnd;
        } else {
            ringDistance = 0;
            ringEndDelay = false;
        }
    } else if(inRing) {
        if(ringDistance > 200000L) {
            ringDistance = 0;
            inRing = false;
        } else if(ringDistance > 25000 && IsRingEnd()) {
            ringDistance = 0;
            ringEndDelay = true;
            inRing = false;
            return RingEnd;
        }
    } else if(aroundBarrier) {
        if(barrierDistance > 5000) {
            barrierDistance = 0;
            aroundBarrier = false;
        } else {
            return barrierType;
        }
    }
    
    int16_t curve = WhichCurve();
    if(curve == Unknown) {
        int16_t cnt = 0;
        int16_t row;
        for(row = 5; row < IMG_ROW; ++row) {
            if((resultSet.rightBorder[row] - resultSet.leftBorder[row])
                - (resultSet.rightBorder[row - 5] - resultSet.leftBorder[row - 5]) > 6
                && resultSet.rightBorder[row] - resultSet.leftBorder[row] > 140) {
                ++cnt;
            }
            if(cnt > 3) {
                break;
            }
        }
        if(row == IMG_ROW) {
            return MAYBE_BARRIER;
        }
        bool hasLeftInflx = false, hasRightInflx = false;
        int16_t _row = row;
        for(; row >= _row - 10; --row) {
            if(OpstSign(resultSet.leftTrend[row], resultSet.leftTrend[row - 2])) {
                hasLeftInflx = true;
            }
            if(OpstSign(resultSet.rightTrend[row], resultSet.rightTrend[row - 2])) {
                hasRightInflx = true;
            }
            if(hasLeftInflx && hasRightInflx) {
                return IsRing() ? Ring : IsCrossRoad() ? CrossRoad : MAYBE_BARRIER;
            }
        }
        return MAYBE_BARRIER;
    } else {
        return curve;
    }
}

bool IsRing() {
    int16_t blackBlockRowsCnt = 0;
    int16_t col;
    int16_t whiteCol;
    int16_t width;
    for (int16_t row = IMG_ROW - 1; row >= 30; --row)
    {
        if(IsWhite(row, IMG_COL / 2))
        {
            continue;
        }
        for(col = IMG_COL / 2 - 1; IsBlack(row, col) && col >= 0; --col) { }
        if(col == -1 || col >= IMG_COL / 2 - 5)
        {
            continue;
        }
        for(whiteCol = col; IsWhite(row, whiteCol) && whiteCol >= 0; --whiteCol) { }
        width = col - whiteCol;
        for (col = IMG_COL / 2 + 1; IsBlack(row, col) && col < IMG_COL; ++col) { }
        if(InRange(col, IMG_COL / 2 + 4, IMG_COL))
        {
            for(whiteCol = col; IsWhite(row, whiteCol) && whiteCol < IMG_COL; ++whiteCol) { }
            if(Abs((whiteCol - col) - width) < 30) {
                ++blackBlockRowsCnt;
            }
        }
    }
    if(blackBlockRowsCnt > 6) {
        return inRing = true;
    }
    return false;
}

bool IsRingEnd() {
    if(resultSet.rightBorderNotFoundCnt < 15) {
        return false;
    }
    {
        int16_t row;
        int16_t cnt = 0;
        for(row = 0; row < IMG_ROW && resultSet.foundRightBorder[row]; ++row) { }
        for(; row < IMG_ROW && !resultSet.foundRightBorder[row]; ++row) { ++cnt; }
        last_not_found_border_row = row;
        if(cnt < 9) {
            return false;
        }
    }
    int16_t cnt = 0;
    int16_t row;
    for(row = 20; row < 40; ++row) {
        if(OpstSign(resultSet.leftTrend[row], resultSet.leftTrend[row - 5])) {
            break;
        }
    }
    if(row >= 40) {
        return false;
    }
    for(row = IMG_ROW - 1; row >= 35; --row) {
        if(IsWhite(row, resultSet.middleLine[row])) {
            ++cnt;
            if(cnt > 12) {
                return true;
            }
        }
    }
    return false;
}

int16_t WhichCurve() {
    int16_t row;
    bool leftCurve = false;
    bool rightCurve = false;
    int16_t cnt = 0;
    for (row = 5; row < IMG_ROW && IsWhite(row, resultSet.middleLine[row]); ++row) { }
    if(row < IMG_ROW && !InRange(resultSet.middleLine[row], IMG_COL / 2 - 22, IMG_COL / 2 + 22)) {
        black_pt_row = row;
        for(; row >= 0; --row) {
            if(!resultSet.foundLeftBorder[row]) {
                leftCurve = true;
                break;
            } else if(!resultSet.foundRightBorder[row]) {
                rightCurve = true;
                break;
            }
        }
        if(leftCurve) {
            for(; row >= 0; --row) {
                if(!resultSet.foundLeftBorder[row] && resultSet.foundRightBorder[row]) {
                    ++cnt;
                    if(cnt > 5) {
                        return LeftCurve;
                    }
                }
            }
        } else if(rightCurve) {
            for(; row >= 0; --row) {
                if(!resultSet.foundRightBorder[row] && resultSet.foundLeftBorder[row]) {
                    ++cnt;
                    if(cnt > 5) {
                        return RightCurve;
                    }
                }
            }
        }
    }
    return Unknown;
}

bool IsCrossRoad() {
    return resultSet.leftBorderNotFoundCnt > 2 && resultSet.rightBorderNotFoundCnt > 2
        && resultSet.leftBorderNotFoundCnt + resultSet.rightBorderNotFoundCnt > 10;
}

int16_t WhichBarrier() {
    int16_t inRow;
    int16_t outRow;
    int16_t row;
    int16_t _barrierType;
    for(row = 10; row < 45 && Abs(resultSet.middleLine[row] - resultSet.middleLine[row - 2]) <= 16; ++row) { }
    if(!InRange(resultSet.middleLine[row - 2], IMG_COL / 2 - 20, IMG_COL / 2 + 20)) {
        return Unknown;
    }
    
    _barrierType = resultSet.middleLine[row] - resultSet.middleLine[row - 2] > 0 ? LeftBarrier : RightBarrier;
    
    inRow = row;
    row += 2;
    for(; row < IMG_ROW && Abs(resultSet.middleLine[row] - resultSet.middleLine[row - 2]) <= 10; ++row) { }
    if((resultSet.middleLine[row] - resultSet.middleLine[row - 2] > 0 && _barrierType == LeftBarrier)
        || (resultSet.middleLine[row] - resultSet.middleLine[row - 2] < 0 && _barrierType == RightBarrier)) {
        return Unknown;
    }
    outRow = row;
    if(outRow - inRow > 5 && row < IMG_ROW && resultSet.middleLine[row] > IMG_COL / 2 - 20
        && resultSet.middleLine[row] < IMG_COL / 2 + 20) {
        aroundBarrier = true;
        return barrierType = _barrierType;
    } else {
        return Unknown;
    }
}

void RingCompensateGoLeft() {
    int row;
    for (row = IMG_ROW - 1;
        row >= 30 && IsWhite(row, IMG_COL / 2); --row) { }
    for (; row >= 10 && IsBlack(row, IMG_COL / 2); --row) { }
    int col;
    for (col = IMG_COL / 2; col >= 0 && IsBlack(row + 1, col); --col) { }
    for(int i = row; i > 0; --i)
    {
        resultSet.rightBorder[i] = col + (row + 1 - i) * (resultSet.rightBorder[0] - col) / row;
        MiddleLineUpdate(i);
    }
    int16_t borderSearchStart = col / 2;
    for (int i = row; i < IMG_ROW; ++i)
    {
        LeftBorderSearchFrom(i, borderSearchStart);
        RightBorderSearchFrom(i, borderSearchStart);
        MiddleLineUpdate(i);
        borderSearchStart = resultSet.middleLine[i];
    }
}

void RingCompensateGoRight() {
    int row;
    for (row = IMG_ROW - 1;
        row >= 30 && IsWhite(row, IMG_COL / 2); --row) { }
    for (; row >= 10 && IsBlack(row, IMG_COL / 2); --row) { }
    int col;
    for (col = IMG_COL / 2; col < IMG_COL && IsBlack(row + 1, col); ++col) { }
    for (int i = row; i > 0; --i)
    {
        resultSet.leftBorder[i] = col - (row + 1 - i) * (col - resultSet.leftBorder[0]) / row;
        MiddleLineUpdate(i);
    }
    int16_t borderSearchStart = col + (IMG_COL - col) / 2;
    for (int i = row; i < IMG_ROW; ++i)
    {
        LeftBorderSearchFrom(i, borderSearchStart);
        RightBorderSearchFrom(i, borderSearchStart);
        MiddleLineUpdate(i);
        borderSearchStart = resultSet.middleLine[i];
    }
}

void RingEndCompensateFromLeft() {
    int16_t row;
    int16_t col;
    for(row = last_not_found_border_row; row < IMG_ROW; ++row) {
        if(resultSet.rightBorder[row] < IMG_COL - 40) {
            col = resultSet.rightBorder[row];
            break;
        }
    }
    if(row < IMG_ROW) {
        --row;
        for(; row >= 0; --row) {
            resultSet.rightBorder[row] = col;
        }
        MiddleLineUpdateAll();
    }
}

void RingEndCompensateFromRight() {
    int16_t row;
    int16_t col;
    for(row = last_not_found_border_row; row < IMG_ROW; ++row) {
        if(resultSet.leftBorder[row] > 40) {
            col = resultSet.leftBorder[row];
            break;
        }
    }
    if(row < IMG_ROW) {
        --row;
        for(; row >= 0; --row) {
            resultSet.leftBorder[row] = col;
        }
        MiddleLineUpdateAll();
    }
}

void LeftCurveCompensate() {
    for (int row_ = IMG_ROW - 1; row_ > black_pt_row; --row_)
    {
        resultSet.middleLine[row_] = 0;
    }
    for (int cnt = 0; cnt < 12; ++cnt)
    {
        resultSet.middleLine[black_pt_row - cnt] = cnt * resultSet.middleLine[black_pt_row - 12] / 12;
    }
}

void RightCurveCompensate() {
    for (int row_ = IMG_ROW - 1; row_ > black_pt_row; --row_)
    {
        resultSet.middleLine[row_] = IMG_COL - 1;
    }
    for (int cnt = 0; cnt < 12; ++cnt)
    {
        resultSet.middleLine[black_pt_row - cnt] = IMG_COL - 1
            - cnt * (IMG_COL - 1 - resultSet.middleLine[black_pt_row - 12]) / 12;
    }
}

void CrossRoadCompensate() {
    int leftCompensateEnd = CrossRoadCompensateOneSide(resultSet.leftBorder, resultSet.leftSlope, resultSet.leftZero);
    int rightCompensateEnd = CrossRoadCompensateOneSide(resultSet.rightBorder, resultSet.rightSlope, resultSet.rightZero);
    
    int borderSearchStart;
    
    if (leftCompensateEnd < rightCompensateEnd)
    {
        borderSearchStart = (resultSet.leftBorder[leftCompensateEnd - 1] + resultSet.rightBorder[leftCompensateEnd - 1]) / 2;
        for (int row = leftCompensateEnd; row < rightCompensateEnd; ++row)
        {
            LeftBorderSearchFrom(row, borderSearchStart);
            borderSearchStart = (resultSet.rightBorder[row] + resultSet.leftBorder[row]) / 2;
        }
    }
    else if (leftCompensateEnd > rightCompensateEnd)
    {
        borderSearchStart = (resultSet.leftBorder[rightCompensateEnd - 1] + resultSet.rightBorder[rightCompensateEnd - 1]) / 2;
        for (int row = rightCompensateEnd; row < leftCompensateEnd; ++row)
        {
            RightBorderSearchFrom(row, borderSearchStart);
            borderSearchStart = (resultSet.rightBorder[row] + resultSet.leftBorder[row]) / 2;
        }
    }
    
    int compensateEnd = Max(leftCompensateEnd, rightCompensateEnd);

    borderSearchStart = (resultSet.leftBorder[compensateEnd - 1] + resultSet.rightBorder[compensateEnd - 1]) / 2;
    for (int row = compensateEnd; row < IMG_ROW; ++row)
    {
        LeftBorderSearchFrom(row, borderSearchStart);
        RightBorderSearchFrom(row, borderSearchStart);
        if (Abs((resultSet.rightBorder[row] + resultSet.leftBorder[row])
            - (resultSet.rightBorder[row - 1] + resultSet.leftBorder[row - 1])) < 10)
        {
            borderSearchStart = (resultSet.rightBorder[row] + resultSet.leftBorder[row]) / 2;
        }
    }
    
    MiddleLineUpdateAll();
}

int16_t CrossRoadCompensateOneSide(int16_t borders[IMG_ROW], int16_t slopes[IMG_ROW], int16_t zeros[IMG_ROW]) {
    int compensateEnd = IMG_ROW;
    int row = 6;
    while (row < IMG_ROW && borders[row] != 0
        && Abs(slopes[row] - slopes[row - 1]) < 3
        && !OpstSign(slopes[row], slopes[row - 1])) { ++row; }
    int16_t _row = row;
    for (; row < compensateEnd; ++row)
    {
        borders[row] = row * slopes[compensateEnd - 5] + zeros[compensateEnd - 5];
        if(IsBlack(row, borders[row]))
        {
            compensateEnd = row;
            break;
        }
    }
    return compensateEnd;
}

bool StartLineJudge(int16_t row) {
    int16_t toggleCnt = 0;
    int16_t patternRowCnt = 0;
    for(int16_t i = row; i >= row - 6; --i) {
        for(int16_t j = IMG_COL / 2; j >= 0; --j) {
            if(TstImgBufAsBitMap(i, j) != TstImgBufAsBitMap(i, j+1)) {
                if(toggleCnt > 14) {
                    toggleCnt = 0;
                    ++patternRowCnt;
                    if(patternRowCnt > 4) {
                        return true;
                    } else {
                        break;
                    }
                } else {
                    ++toggleCnt;
                }
            }
        }
    }
    return false;
}

bool StraightLineJudge(void) {
    int16_t middleAreaCnt = 0;
    for(int16_t i = 0; i < IMG_ROW; ++i) {
        if(InRange(resultSet.middleLine[i], IMG_COL / 2 - 10, IMG_COL / 2 + 10)) {
            middleAreaCnt++;
        }
    }
    return middleAreaCnt > 38;
}

inline float Abs(float input) {
    return input >= 0 ? input : -input;
}

inline float Min(float a, float b) {
    return a > b ? b : a;
}

inline float Max(float a, float b) {
    return a > b ? a : b;
}

inline bool OpstSign(int16_t a, int16_t b) {
    return (a & 0x8000) ^ (b & 0x8000);
}

inline bool InRange(int16_t value, int16_t lbound, int16_t hbound) {
    return value > lbound && value < hbound;
}
