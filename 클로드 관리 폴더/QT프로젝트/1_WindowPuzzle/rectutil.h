#pragma once
#include <QRect>
#include <QVector>

// a - b  (사각형에서 사각형을 뺀 나머지를 최대 4조각으로 반환)
// QRect의 right()/bottom()은 inclusive 좌표임에 주의.
inline QVector<QRect> subtractRect(const QRect &a, const QRect &b)
{
    const QRect i = a.intersected(b);
    if (i.isEmpty())
        return QVector<QRect>{a};

    QVector<QRect> out;
    if (i.top() > a.top())                                   // 위쪽 띠
        out.append(QRect(a.left(), a.top(), a.width(), i.top() - a.top()));
    if (i.bottom() < a.bottom())                             // 아래쪽 띠
        out.append(QRect(a.left(), i.bottom() + 1, a.width(), a.bottom() - i.bottom()));
    if (i.left() > a.left())                                 // 왼쪽 조각
        out.append(QRect(a.left(), i.top(), i.left() - a.left(), i.height()));
    if (i.right() < a.right())                               // 오른쪽 조각
        out.append(QRect(i.right() + 1, i.top(), a.right() - i.right(), i.height()));
    return out;
}

// box가 rects의 합집합에 "완전히" 덮이는가?
// = 이 게임에서 box가 갈 수 있는 공간인가?
inline bool fullyCovered(const QRect &box, const QVector<QRect> &rects)
{
    if (box.isEmpty())
        return true;

    QVector<QRect> remain{box};
    for (const QRect &r : rects) {
        QVector<QRect> next;
        for (const QRect &p : remain) {
            const QVector<QRect> pieces = subtractRect(p, r);
            for (const QRect &q : pieces)
                if (!q.isEmpty())
                    next.append(q);
        }
        remain = next;
        if (remain.isEmpty())
            return true;
        if (remain.size() > 512)   // 안전장치 (창이 아주 많아졌을 때)
            return false;
    }
    return remain.isEmpty();
}
