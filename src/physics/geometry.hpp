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

	TPoint operator*(const float factor) const {
		return { x * factor, y * factor };
	}

	TPoint operator+(const TPoint& other) const {
		return { x + other.x, y + other.y };
	}

	// z-составляющая векторного произведения двух векторов из плоскости (x,y)
	float crossComponent(const TPoint& other) const {
		return y * other.x - x * other.y;
	}

	float norm() const {
		return sqrt(x * x + y * y);
	}

	void normalize() {
		float n = norm();
		x /= n;
		y /= n;
	}
};

struct TLine {
	// point_on_line = p1 + (p2 - p1) * t, t -- параметр
	TPoint p1, p2;
	TPoint tangent, normal;

	TLine(TPoint _p1, TPoint _p2) : p1(_p1), p2(_p2) {      //    normal
		tangent = _p2 - _p1;                                //    ^
		tangent.normalize();                                //    |
		normal = { tangent.y, - tangent.x }; // левая нормаль     ----> tangent    НО! ось y направлена вниз!
	}

	TPoint intersect(TLine other) const {
		float num = (other.p1 - p1).crossComponent(other.tangent);
		float den = tangent.crossComponent(other.tangent);  // != 0
		float t = num / den;
		return p1 + tangent * t;
	}

	TLine shiftOut(const float distance) const {
		return TLine(p1 + normal * distance, p2 + normal * distance);
	}
};


struct TGeometry {
	std::vector<TPoint> coords;

	int get_next_idx(int i) const {
		return (i == coords.size() - 1) ? 0 : i + 1;
	}

	int get_prev_idx(int i) const {
		return (i == 0) ? coords.size() - 1 : i - 1;
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

	// Inflate geometry moving all edges outwards by a given distance
	std::vector<TPoint> getCoordsInflated(const float distance = 0.5) {
		std::vector<TLine> edges;
		std::vector<TPoint> coordsInflated;

		// Готовим грани
		for (int i{ 0 }; i < coords.size(); ++i) {
			const TPoint& beg = coords[i];
			const TPoint& end = coords[get_next_idx(i)];
			TLine edge = { beg, end };
			TLine edgeShifted = edge.shiftOut(distance);
			edges.push_back(edgeShifted);
		}

		// Готовим новую геометрию, пересекая сдвинутые грани
		for (int i{ 0 }; i < edges.size(); ++i) {
			const TLine first = edges[get_prev_idx(i)];
			const TLine second = edges[i];
			coordsInflated.push_back(first.intersect(second));
		}

		return coordsInflated;
	}
};