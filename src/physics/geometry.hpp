#pragma once

#include <vector>


struct TPoint {
	float x, y;
	bool isWallFollows = true;

	TPoint operator-(const TPoint& other) const {
		return { x - other.x, y - other.y };
	}

	float operator*(const TPoint& other) const {
		return x * other.x + y * other.y;
	}
};


struct TGeometry {
	std::vector<TPoint> coords;

	int get_next_idx(int i) const {
		return (i == coords.size() - 1) ? 0 : i + 1;
	}

	// Определяем, лежит ли точка pnt справа от стенки AB,
	// начинающейся в точке с индексом wallBegIdx
	// Используем свойство векторного произведения [AB, Apnt]
	bool isInside(const TPoint& pnt, const int wallBegIdx) const {

		const TPoint& beg = coords[wallBegIdx];
		const TPoint& end = coords[get_next_idx(wallBegIdx)];

		const TPoint be = end - beg;  // TODO: можно этот вектор запомнить для геометрии
		const TPoint bp = pnt - beg;
		const TPoint ep = pnt - end;

		bool isOnCorrectSide = (be.x * bp.y - be.y * bp.x) > 0; // в графике ось x направлена вправо, но ось y направлена вниз!
		bool isBeyondFace = (be * bp < 0.0) || (be * ep > 0.0); // атом должен лежать над ребром, иначе в невыпуклых геометриях будут проблемы

		return  isBeyondFace || isOnCorrectSide;
	}

	bool isInside(const TPoint& pnt) const {
		for (int i{ 0 }; i < coords.size(); ++i) {
			if (!isInside(pnt, i))
				return false;
		}
		return true;
	}

	// Начинается ли с данной точки стенка
	bool isWall(int wallBegIdx) {
		return coords[wallBegIdx].isWallFollows;
	}

	// Get face
	TPoint getFace(const int iBeg) const {
		const TPoint& beg = coords[iBeg];
		const TPoint& end = coords[get_next_idx(iBeg)];
		return end - beg;
	}
};